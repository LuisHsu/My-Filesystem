#include "myfs.h"

static FILE *mountPoint = NULL;
static char mountName[256] = "";
static union{
			unsigned char bytes[20];
			Superblock block;
		}u_Superblock;
		
static FileDiscriptor *fdHead = NULL;

int block_alloc(FILE *fptr){
	if(u_Superblock.block.block_count < 1){
		return 0;
	}
	unsigned long locSave = ftell(fptr);
	fseek(fptr,20+u_Superblock.block.inode_section_size,SEEK_SET);
	unsigned char dirty;
	unsigned int block_addr = 1;
	while(block_addr < u_Superblock.block.block_count){
		fscanf(fptr,"%c",&dirty);
		if(dirty!='d'){
			fseek(fptr,-1,SEEK_CUR);
			fprintf(fptr,"%c",'d');
			u_Superblock.block.block_unused -= 1;
			return block_addr;
		}
		fseek(fptr,1023,SEEK_CUR);
		block_addr++;
	}
	
	fseek(fptr,locSave,SEEK_SET);
	return 0;
}

int block_locate(unsigned long int filesize, Inode *inode, FILE *fptr){
	fseek(fptr,20+u_Superblock.block.inode_section_size,SEEK_SET);
	if(filesize<12276){
		int index = filesize/1023;
		if(inode->ptr_direct[index] == 0){ 
			inode->ptr_direct[index] = block_alloc(fptr);
			if(inode->ptr_direct[index] == 0){
				return -1;
			}
		}else{
			fseek(fptr,(inode->ptr_direct[index]-1)*1024+(filesize%1023),SEEK_CUR);
		}
	}
}

int	myfs_create(const char *filesystemname, int max_size){
	// Open file 
	if(!(mountPoint = fopen(filesystemname,"wb+"))){
		return -1;
	}
	// Calculate actual size of each section
	unsigned int inode_count,inode_section,block_count;
	if(max_size<2048){
		inode_section = ((unsigned int)((max_size<<11)/(1024.0+INODE_SIZE)))*512*INODE_SIZE;
		block_count = (max_size<<10)-(inode_section>>10)-1;
		inode_count = inode_section/INODE_SIZE;
	}else{
		inode_section = ((unsigned int)(max_size/(1024.0+INODE_SIZE)))*1048576*INODE_SIZE;
		block_count = ((max_size-(inode_section>>20))<<10)-1;
		inode_count = inode_section/INODE_SIZE;
	}
	// Formatting disk file
	for(int i=0;i<max_size;++i){
		for(unsigned int j=0;j<1048576;++j){
			fprintf(mountPoint,"%c",0);	
		}
	}
	// Write Superblock
	u_Superblock.block.block_count = block_count;
	u_Superblock.block.block_unused= block_count;
	u_Superblock.block.inode_section_size = inode_section;
	u_Superblock.block.inode_count = 0;
	u_Superblock.block.inode_count += inode_count;
	u_Superblock.block.inode_unused= 0;
	u_Superblock.block.inode_unused+= inode_count;
	fseek(mountPoint,0,SEEK_SET);
	for (int i = 0; i < 20; i++) {
		fprintf(mountPoint, "%c",u_Superblock.bytes[i]);
	}
	// Unmount
	myfs_umount();
	return 0;
}

int myfs_destroy(const char *filesystemname){
	if(!strcmp(mountName,filesystemname)){
		if(myfs_umount() < 0){
			return -2;	
		}
	}
	if(remove(filesystemname)){
		return -1;
	}
	return 0;
}

int myfs_mount(const char *filesystemname){
	if(myfs_umount() < 0){
		return -2;
	}
	if(!(mountPoint = fopen(filesystemname,"rb+"))){
		return -1;
	}
	strcpy(mountName,filesystemname);
	// Read Superblock
	for(int i=0; i<20; ++i){
		fscanf(mountPoint,"%c",&u_Superblock.bytes[i]);
	}
	fseek(mountPoint,0,SEEK_SET);
	return 0;
}

int myfs_umount(){
	if (mountPoint != NULL) {
		FileDiscriptor *cur = fdHead;
		while(cur!=NULL){
			FileDiscriptor *tmp = cur;
			cur = cur->next;
			fclose(tmp->fptr);
			free(tmp);
		}
		fdHead = NULL;
		if(fclose(mountPoint)==EOF){
			return -1;
		}
		mountPoint = NULL;
		strcpy(mountName,"");
		return 0;
	}
	return 1;
}

int myfs_file_open(const char *filename){
	if(mountPoint == NULL){
		return -1;
	}
	if(u_Superblock.block.inode_unused == u_Superblock.block.inode_count){
		return -2;
	}
	// Create new fd mode
	FileDiscriptor *newFD = (FileDiscriptor *)malloc(sizeof(FileDiscriptor));
	if(!(newFD->fptr = fopen(mountName,"rb+"))){
		free(newFD);
		return -3;
	}
	
	// Find inode
	int found = 0;
	fseek(newFD->fptr,20,SEEK_SET);
	while(ftell(newFD->fptr)<(20+u_Superblock.block.inode_section_size)){
		char buf[256];
		fgets(buf,256,newFD->fptr);
		if(!strcmp(buf,filename)){
			found = 1;
			break;
		}
		fseek(newFD->fptr,INODE_SIZE - 255,SEEK_CUR);
	}
	if(!found){
		fclose(newFD->fptr);
		free(newFD);
		return -4;
	}
	fseek(newFD->fptr,-255,SEEK_CUR);
	newFD->inode_location = ftell(newFD->fptr);
	
	// Insert file descriptor
	FileDiscriptor *cur = fdHead;
	if((cur == NULL)||(cur->fd != 0)){
		newFD->fd = 0;
		newFD->next = fdHead;
		fdHead = newFD;
		return 0;
	}
	while(cur->next != NULL){
		if(cur->next->fd != ((cur->fd) + 1)){
			newFD->fd = (cur->fd) + 1;
			newFD->next = cur->next;
			cur->next = newFD;
			return newFD->fd;
		}
		cur = cur -> next;
	}
	newFD->fd = (cur->fd) + 1;
	newFD->next = cur->next;
	cur->next = newFD;
	return newFD->fd;
}

int myfs_file_close(int fd){
	if(mountPoint == NULL){
		return -1;
	}
	if(u_Superblock.block.inode_unused == u_Superblock.block.inode_count){
		return -2;
	}
	// Find fd
	FileDiscriptor *cur = fdHead, *pre = fdHead;
	while(1){
		if(cur == NULL){
			return -4;
		}
		if(cur->fd == fd){
			break;
		}
		pre = cur;
		cur = cur->next;
	}
	// Delete fd
	if(fclose(cur->fptr)==EOF){
		return -3;
	}
	if(pre == cur){
		fdHead = cur->next;
	}else{
		pre->next = cur->next;
	}
	free(cur);	
	return 0;
}

int myfs_file_create(const char *filename){
	if(mountPoint == NULL){
		return -1;
	}
	if(!u_Superblock.block.inode_unused){
		return -2;
	}
	union{
		unsigned char bytes[INODE_SIZE];
		Inode inode;
	}u_inode;
	memset(u_inode.bytes,0,INODE_SIZE);
	strncpy(u_inode.inode.filename,filename,strlen(filename));
	u_inode.inode.filesize_L = 0;
	u_inode.inode.filesize_H = 0;
	
	// Open disk file
	FILE *fptr;
	if(!(fptr = fopen(mountName,"rb+"))){
		return -3;
	}
	fseek(fptr,20,SEEK_SET);
	// Find empty inode
	int location = 0;
	while(ftell(fptr)<(20+u_Superblock.block.inode_section_size)){
		char buf[256];
		fgets(buf,256,fptr);
		if(!strlen(buf)){
			if(!location){
				location = ftell(fptr)-255;
			}
		}
		if(!strcmp(buf,filename)){
			fclose(fptr);
			return -4;
		}
		fseek(fptr,INODE_SIZE - 255,SEEK_CUR);
	}
	fseek(fptr,location,SEEK_SET);
	
	// Write superblock
	u_Superblock.block.inode_unused -= 1;
	for (int i = 0; i < 20; i++) {
		fprintf(mountPoint, "%c",u_Superblock.bytes[i]);
	}
	fseek(mountPoint,0,SEEK_SET);
	
	// Write inode
	for(int i=0; i<INODE_SIZE; ++i){
		fprintf(fptr,"%c",u_inode.bytes[i]);
	}
	fclose(fptr);
	return 0;
}

int myfs_file_delete(const char *filename){
	if(mountPoint == NULL){
		return -1;
	}
	if(u_Superblock.block.inode_unused == u_Superblock.block.inode_count){
		return -2;
	}
	// Open disk file
	FILE *fptr;
	if(!(fptr = fopen(mountName,"rb+"))){
		return -3;
	}
	fseek(fptr,20,SEEK_SET);
	// Find inode
	int found = 0;
	while(ftell(fptr)<(20+u_Superblock.block.inode_section_size)){
		char buf[256];
		fgets(buf,256,fptr);
		if(!strcmp(buf,filename)){
			found = 1;
			break;
		}
		fseek(fptr,INODE_SIZE - 255,SEEK_CUR);
	}
	fseek(fptr,-255,SEEK_CUR);
	
	if(!found){
		return -4;
	}
	
	// Write superblock
	u_Superblock.block.inode_unused += 1;
	for (int i = 0; i < 20; i++) {
		fprintf(mountPoint, "%c",u_Superblock.bytes[i]);
	}
	fseek(mountPoint,0,SEEK_SET);
	
	// Write inode
	for(int i=0; i<INODE_SIZE; ++i){
		fprintf(fptr,"%c",0);
	}
	fclose(fptr);
	return 0;
}

int myfs_file_read(int fd, char *buf, int count){
	/* TODO */	
}

int myfs_file_write(int fd, char *buf, int count){
	if(mountPoint == NULL){
		return -1;
	}
	if(u_Superblock.block.inode_unused == u_Superblock.block.inode_count){
		return -2;
	}
	// Find fd
	FileDiscriptor *cur = fdHead, *pre = fdHead;
	while(1){
		if(cur == NULL){
			return -4;
		}
		if(cur->fd == fd){
			break;
		}
		pre = cur;
		cur = cur->next;
	}
	// Read inode
	fseek(cur->fptr, cur->inode_location, SEEK_SET);
	union{
		unsigned char bytes[INODE_SIZE];
		Inode inode;
	}u_inode;
	for(int i=0; i<INODE_SIZE; ++i){
		fscanf(cur->fptr,"%c",&u_inode.bytes[i]);
	}
	fflush(cur->fptr);
	unsigned long int filesize = (((unsigned long int)u_inode.inode.filesize_H)<<32)+u_inode.inode.filesize_L;
	
	// Write data
	if(block_locate(filesize, &(u_inode.inode), cur->fptr) == -1){
		return -3;
	}

	int state = 0;
	for(int i=0, block_capacity=filesize % 1024; i < count; ++i){
		if(block_capacity > 1022){
			fflush(cur->fptr);
			if(block_locate(filesize, &(u_inode.inode), cur->fptr) == -1){
				state = -5;
				break;
			}
		}
		fprintf(cur->fptr,"%c",buf[i]);
		if(++filesize > 17029540340){
			state = -6;
			break;
		}
		block_capacity=(block_capacity+1)%1024;
	}
	fflush(cur->fptr);
	// Write inode
	u_inode.inode.filesize_H = (unsigned char) ((filesize >> 32) & 0xff);
	u_inode.inode.filesize_L = filesize & 0xFFFFFFFF;
	fseek(cur->fptr, cur->inode_location, SEEK_SET);
	for(int i=0; i<INODE_SIZE; ++i){
		fprintf(cur->fptr,"%c",u_inode.bytes[i]);
	}
	fflush(cur->fptr);
	
	// Write superblock
	fseek(cur->fptr,0, SEEK_SET);
	for(int i=0; i<20; ++i){
		fprintf(cur->fptr,"%c",u_Superblock.bytes[i]);
	}
	fflush(cur->fptr);
	
	return state;
}
