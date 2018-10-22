all: test

test: test.o SkipList.o
	gcc -o test test.o SkipList.o

test.o: SkipList.o test.c
	gcc -c test.c

SkipList.o: SkipList.c SkipList.h
	gcc -c SkipList.c
