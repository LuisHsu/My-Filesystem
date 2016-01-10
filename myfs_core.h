#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#define INODE_SIZE 320

typedef struct {
	unsigned int block_count:32;
	unsigned int block_unused:32;
	unsigned int inode_section_size:32;
	int inode_count:24;
	int inode_unused:24;
}Superblock;

typedef struct {
	unsigned char filename[256]; 
	// filename null means the inode is free
	unsigned int filesize;
	unsigned int ptr_direct[12];
	unsigned int ptr_level_1;
	unsigned int ptr_level_2;
	unsigned int ptr_level_3;
}Inode;

typedef struct {
	unsigned int level:2;
	unsigned int entry_count:10;
	unsigned int reserved:20;

}PtrBlock;

typedef struct {
	unsigned char dirty;
	unsigned char bytes[1023];
}Block;

