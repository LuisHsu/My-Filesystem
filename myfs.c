#include "myfs.h"

#define INODE_SIZE 317

static FILE *mountPoint = NULL;

int	myfs_create(const char *filesystemname, int max_size){
	if(!(mountPoint = fopen(filesystemname,"w+"))){
		return -1;
	}
	unsigned int inode_count,inode_section,actual_KB;
	if(max_size<2048){
		inode_section = ((unsigned int)((max_size<<11)/1341.0))*512*INODE_SIZE;
		actual_KB = (max_size<<10)-(inode_section>>10);
		inode_count = inode_section/INODE_SIZE;
	}else{
		inode_section = ((unsigned int)(max_size/1341.0))*1048576*INODE_SIZE;
		actual_KB = (max_size-(inode_section>>20))<<10;
		inode_count = inode_section/INODE_SIZE;
	}	
	printf("Actual_size: %u(KB),%.2f(MB)\n",actual_KB,actual_KB/1024.0);
	printf("Inode count: %u\nInode section size: %u(B), %.2f(MB)\n",inode_count,inode_section,inode_section/1048576.0);
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
		return 0;
	}
	return 1;
}
