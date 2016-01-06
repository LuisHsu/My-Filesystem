#include "myfs.h"

#define INODE_SIZE 317

static FILE *mountPoint = NULL;

int	myfs_create(const char *filesystemname, int max_size){
	// Open file 
	if(!(mountPoint = fopen(filesystemname,"w+"))){
		return -1;
	}
	// Calculate actual size of each section
	unsigned int inode_count,inode_section,block_count;
	if(max_size<2048){
		inode_section = ((unsigned int)((max_size<<11)/1341.0))*512*INODE_SIZE;
		block_count = (max_size<<10)-(inode_section>>10);
		inode_count = inode_section/INODE_SIZE;
	}else{
		inode_section = ((unsigned int)(max_size/1341.0))*1048576*INODE_SIZE;
		block_count = (max_size-(inode_section>>20))<<10;
		inode_count = inode_section/INODE_SIZE;
	}
	
	// Write Superblock
	union{
		unsigned char bytes[20];
		Superblock block;
	}u_Superblock;
	u_Superblock.block.block_count = block_count;
	u_Superblock.block.block_unused= block_count;
	u_Superblock.block.inode_section_size = inode_section;
	u_Superblock.block.inode_count = 0;
	u_Superblock.block.inode_count += inode_count;
	u_Superblock.block.inode_unused= 0;
	u_Superblock.block.inode_unused+= inode_count;

	for (int i = 0; i < 20; i++) {
		fprintf(mountPoint, "%c",u_Superblock.bytes[i]);
	}
	
	myfs_umount();
	return 0;
}

int myfs_destroy(const char *filesystemname){
	/* TODO */
}

int myfs_mount(const char *filesystemname){
	/* TODO */
}

int myfs_umount(){
	if (mountPoint != NULL) {
		if(fclose(mountPoint)==EOF){
			return -1;
		}
		mountPoint = NULL;
		return 0;
	}
	return 1;
}
