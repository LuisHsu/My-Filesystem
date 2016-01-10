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
