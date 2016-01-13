#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define INODE_SIZE 324

typedef struct {
	unsigned int block_count:32;
	unsigned int block_unused:32;
	unsigned int inode_section_size:32;
	int inode_count:24;
	int inode_unused:24;
}Superblock;

typedef struct {
	char filename[256]; 
	// filename null means the inode is free
	unsigned int ptr_direct[12];
	unsigned int ptr_level_1;
	unsigned int ptr_level_2;
	unsigned int ptr_level_3;
	// NOTICE: Value of ptr_X is start from 1
	unsigned int filesize_L;
	char filesize_H;
}Inode;

typedef struct {
	char dirty;
	unsigned int entry[255];
}PtrBlock;

typedef struct {
	char dirty;
	char bytes[1023];
}Block;

typedef struct FD{
	int fd;
	int inode_location;
	unsigned long int location;
	FILE *fptr;
	struct FD *next;
}FileDiscriptor;

typedef struct {
	char filename[256];
	unsigned long int filesize;
}FileStatus;
