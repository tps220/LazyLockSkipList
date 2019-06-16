#include "SkipList.h"
#include "Utilities.h"
//constructor that initializes data fields of the node and its lock,
//but does not link the node to other towers
struct Node* constructNode(int val, int topLevel) {
  struct Node* node = (struct Node*)malloc(sizeof(struct Node));
  node -> next = (struct Node**)malloc(topLevel * sizeof(struct Node*));
  node -> val = val;
  node -> topLevel = topLevel;
  node -> markedToDelete = 0;
  node -> fullylinked = 0;
  pthread_mutex_init(&node -> lock, NULL);
  return node;
}

//constructor that initializes data fields of the node and its lock, as well as
//pointing each level of the tower to the provided next node
struct Node* constructLinkedNode(int val, int topLevel, struct Node* next) {
    struct Node* node = constructNode(val, topLevel);
    for (int i = 0; i < topLevel; i++) {
      node -> next[i] = next;
    }
    return node;
}

void destructNode(struct Node* node) {
  pthread_mutex_destroy(&node -> lock);
  free(node -> next);
  free(node);
}

//constructor for skip list that initializes sentinel nodes
struct SkipList* constructSkipList(int maxLevel) {
  struct SkipList* skipList = (struct SkipList*)malloc(sizeof(struct SkipList));
  struct Node* tail = constructLinkedNode(INT_MAX, maxLevel, NULL);
  struct Node* head = constructLinkedNode(INT_MIN, maxLevel, tail);
  tail -> fullylinked = 1;
  head -> fullylinked = 1;
  skipList -> head = head;
  skipList -> maxLevel = maxLevel;
  return skipList;
}

//destructor for skip list that frees all data and locks
struct SkipList* destructSkipList(struct SkipList* skipList) {
  struct Node* runner = skipList -> head;
  while (runner != NULL) {
    struct Node* temp = runner -> next[0];
    destructNode(runner);
    runner = temp;
  }
  free(skipList);
  return NULL;
}

//gets size of skip list, not concurrent
size_t getSize(struct SkipList* skipList) {
  int size = -1;
  struct Node* runner = skipList -> head -> next[0];
  while (runner -> next[0] != NULL) {
    if (runner -> fullylinked && !runner -> markedToDelete) {
      size++;
    }
    runner = runner -> next[0];
  }
  return size;
}

//Lazily searches the skip list, not regarding locks
//stores the predeccor and successor nodes during a traversal of the skip list.
//if the value is found, then returns the first index in the successor where it was found
//if the value is not found, returns -1
int search(struct SkipList* skipList, int val, struct Node** predecessors, struct Node** successors) {
  struct Node* previous = skipList -> head;
  int idx = -1;
  for (int i = skipList -> maxLevel - 1; i >= 0; i--) {
    struct Node* current = previous -> next[i];
    while (current -> val < val) {
      previous = current;
      current = previous -> next[i];
    }
    if (idx == -1 && val == current -> val) {
        
      idx = i;
    }
    predecessors[i] = previous;
    successors[i] = current;
  }
  return idx;
}

//return true if the value was found and is valid
//returns false if the value was not found, or the the value was
//found and its invalid (in an intermediate state and not linearizable)
int find(struct SkipList* skipList, int val) {
  struct Node *predecessors[skipList -> maxLevel], *successors[skipList -> maxLevel];
  int idx = search(skipList, val, predecessors, successors);
  return idx != -1 && successors[idx] -> fullylinked && successors[idx] -> markedToDelete == 0;
}

//unlocks all the levels that were locked previously
void unlockLevels(struct Node** nodes, int topLevel) {
  struct Node* previous = NULL;
  for (int i = 0; i <= topLevel; i++) {
    if (nodes[i] != previous) {
      pthread_mutex_unlock(&nodes[i] -> lock);
    }
    previous = nodes[i];
  }
}

//Inserts a value into the skip list
int insert(struct SkipList* skipList, int val) {
  int topLevel = getRandomLevel(skipList -> maxLevel);
  struct Node *predecessors[skipList -> maxLevel], *successors[skipList -> maxLevel];
  int finished = 0;

  while (!finished) {
    //store the result of a traveral through the skip list while searching for a value
    int idx = search(skipList, val, predecessors, successors);
    //if already inside the tree
    if (idx != -1) {
      struct Node* foundObj = successors[idx];
      //if it isnt markedToDelete for removal, wait until it has been fully linked and return false
      if (foundObj -> markedToDelete == 0) {
        while (foundObj -> fullylinked == 0) {} //wait for linearization point
        return 0;
      }
      //if it was deleted, and hasn't been removed yet, we restart
      continue;
    }
    //if it hasn't been found in the tree, attempt to lock all the necessary
    //levels and then insert the new tower of nodes
    int highest_locked = -1;
    struct Node* previous = NULL, *runner = NULL, *prev_pred = NULL;
    int valid = 1;
    for (int i = 0; i < topLevel && valid; i++) {
      previous = predecessors[i];
      runner = successors[i];
      if (previous != prev_pred) {
        pthread_mutex_lock(&previous -> lock);
        highest_locked = i;
        prev_pred = previous;
      }
      //ensures that all values that have been seen are valid
      valid = previous -> markedToDelete == 0 &&
              runner -> markedToDelete == 0 &&
              (volatile struct Node*)previous -> next[i] == (volatile struct Node*)runner;
    }
    //if we broke out of the loop because one of the nodes wasn't valid, then we unlock
    //all the levels and attempt again
    if (valid == 0) {
      unlockLevels(predecessors, highest_locked);
      continue;
    }
    //otherwise, all nodes in our critical sections have been locked and we can
    //insert into the list
    struct Node* insertion = constructNode(val, topLevel);
    for (int i = 0; i < topLevel; i++) {
      insertion -> next[i] = successors[i];
      predecessors[i] -> next[i] = insertion;
    }
    insertion -> fullylinked = 1;
    unlockLevels(predecessors, highest_locked);
    return 1;
  }
  return 0;
}

int validDeletion(struct Node* deletion, int idx) {
  return deletion -> topLevel - 1 == idx && deletion -> fullylinked && deletion -> markedToDelete == 0;
}

int removeNode(struct SkipList* skipList, int val) {
  int markedToDelete = 0;
  int topLevel = -1;
  struct Node *predecessors[skipList -> maxLevel], *successors[skipList -> maxLevel];
  struct Node* deletion = NULL;
  int finished = 0;
  while (!finished) {
    //store the result of a traveral through the skip list while searching for a value
    int idx = search(skipList, val, predecessors, successors);
    //if we're marked to delete or we found a valid value to delete
    if (markedToDelete || (idx != -1 && validDeletion(successors[idx], idx))) {
      //if not already marked to delete, take control of the lock and mark it to delete
      if (markedToDelete == 0) {
        deletion = successors[idx];
        topLevel = deletion -> topLevel;
        pthread_mutex_lock(&deletion -> lock);
        if (deletion -> markedToDelete) {
          pthread_mutex_unlock(&deletion -> lock);
          return 0;
        }
        deletion -> markedToDelete = 1;
        markedToDelete = 1;
      }

      //attempt to gain control of all locks
      int highest_locked = -1;
      int valid = 1;
      struct Node *previous, *runner, *prev_pred = NULL;
      for (int i = 0; i < topLevel && valid; i++) {
        previous = predecessors[i];
        runner = successors[i];
        if (previous != prev_pred) {
          pthread_mutex_lock(&previous -> lock);
          highest_locked = i;
          prev_pred = previous;
        }
        valid = previous -> markedToDelete == 0 &&
                (volatile struct Node*)previous -> next[i] == (volatile struct Node*)runner;
      }

      //if we broke out of the loop because one of the nodes wasn't valid, then we unlock
      //all the levels and attempt again
      if (valid == 0) {
        unlockLevels(predecessors, highest_locked);
        continue;
      }
      //otherwise, all nodes in our critical sections have been locked and we can
      //delete the proper nodes in the list
      for (int i = topLevel - 1; i >= 0; i--) {
        predecessors[i] -> next[i] = deletion -> next[i];
      }
      pthread_mutex_unlock(&deletion -> lock);
      unlockLevels(predecessors, highest_locked);
      return 1;
    }
    //otherwise we didn't find a node with the value and nothing was markedToDelete for deletion,
    //and therefore we return false
    else {
      return 0;
    }
  }
  return 0;
}
