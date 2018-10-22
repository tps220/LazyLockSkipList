all: test

test: test.o SkipList.o
	gcc -O3 -lpthread -o test test.o SkipList.o

test.o: SkipList.o test.c
	gcc -O3 -lpthread -c test.c

SkipList.o: SkipList.c SkipList.h
	gcc -O3 -lpthread -c SkipList.c
