#include <stdio.h>
#include <math.h>
#include <errno.h>

typedef struct {
	unsigned int block_count:30;
	unsigned int block_unused:30;
	unsigned int inode_count:24;
	unsigned int inode_unused:24;
	unsigned int inode_section_size:20;
}Superblock;
