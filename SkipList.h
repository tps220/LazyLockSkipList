#include <pthread.h>
#include <stdlib.h>

#define INT_MIN -2147483648
#define INT_MAX 2147483647

struct Node {
  int val; //stores the value at the node
  int topLevel; //stores the height of the tower
  struct Node** next; //stores the next pointer for each level in the tower
  volatile int markedToDelete; //stores whether marked for deletion
  volatile int fullylinked; //stores whether fullylinked or in the middle of insertion
  pthread_mutex_t lock; //node-specific mutex
};
struct Node* constructNode(int val, int topLevel);
struct Node* constructLinkedNode(int val, int topLevel, struct Node* next);
void destructNode(struct Node* node);

struct SkipList {
  struct Node* head; //stores the head sentinel node
  int maxLevel; //stores the max number of levels, declared at runtime by user
};
struct SkipList* constructSkipList(int maxLevel);
struct SkipList* destructSkipList(struct SkipList* skipList);
size_t getSize(struct SkipList* skipList);
int search(struct SkipList* skipList, int val, struct Node** predecessors, struct Node** successors);
int find(struct SkipList* skipList, int val);
void unlockLevels(struct Node** nodes, int topLevel);
int insert(struct SkipList* skipList, int val);
int validDeletion(struct Node* deletion, int idx);
int removeNode(struct SkipList* skipList, int val);
