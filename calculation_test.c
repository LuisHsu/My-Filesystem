#include "myfs_core.h"

#define INODE_SIZE 317

void decode(char *filename){
	Superblock block;
	
	FILE *fin=fopen(filename,"r");
	unsigned char *superPtr = (unsigned char *) &block;
	for(int i=0;i<20;++i){
		fscanf(fin,"%c",superPtr+i);
	}
	printf("Block count: %u\n",block.block_count);
	printf("Block unused: %u\n",block.block_unused);
	printf("Inode count: %u\n",block.inode_count);
	printf("Inode unused: %u\n",block.inode_unused);
	printf("Inode section size: %u\n",block.inode_section_size);
	fclose(fin);
}

int main(){
	printf("Input disk file name:");
	char filename[256];
	scanf("%s",filename);
	decode(filename);
	return 0;
}
