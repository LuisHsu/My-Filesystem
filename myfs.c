#include "myfs.h"

static FILE *mountPoint = NULL;
static char mountName[256] = "";
static union{
			char bytes[20];
			Superblock block;
		}u_Superblock;
		
static FileDiscriptor *fdHead = NULL;

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
			char nch = 0;
			fwrite(&nch,1,1,mountPoint);
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
		fwrite(&u_Superblock.bytes[i],1,1,mountPoint);
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
		fread(&u_Superblock.bytes[i],1,1,mountPoint);
	}
	fflush(mountPoint);
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
	newFD->location = 0;
	
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
		char bytes[INODE_SIZE];
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
	fflush(mountPoint);
	fseek(mountPoint,0,SEEK_SET);
	
	// Write inode
	for(int i=0; i<INODE_SIZE; ++i){
		fprintf(fptr,"%c",u_inode.bytes[i]);
	}
	fclose(fptr);
	return 0;
}

void locate_level_delete(FILE *fptr, unsigned int addr, int level){
	if(addr == 0){
		return;
	}
	fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
	fseek(fptr, (addr-1)*1024, SEEK_CUR);
	
	// Read Block
	union{
		char bytes[1024];
		PtrBlock block;
	}u_PtrBlock;
	fprintf(fptr,"%c",0);
	fflush(fptr);
	fseek(fptr, -1, SEEK_CUR);
	for(int i=0;i<1024;++i){

		fread(&(u_PtrBlock.bytes[i]),1,1,fptr);
	}
	fflush(fptr);
	fseek(fptr, -1024, SEEK_CUR);
	
	
	if(level == 1){	
		for(int i=0; i<255; ++i){
			if(u_PtrBlock.block.entry[i] == 0){
				continue;
			}
			fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
			fseek(fptr, ((u_PtrBlock.block.entry[i])-1)*1024, SEEK_CUR);
			// Write Block
			fprintf(fptr,"%c",0);
			fflush(fptr);
		}
	}else{
		for(int i=0; i<255; ++i){
			locate_level_delete(fptr, u_PtrBlock.block.entry[i], level-1);
		}
	}
}

void block_delete_all(FILE *fptr, Inode *inode){
	for(int i=0; i<12; ++i){
		if(inode->ptr_direct[i]!=0){
			fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
			fseek(fptr, ((inode->ptr_direct[i])-1)*1024, SEEK_CUR);
			fprintf(fptr,"%c",0);
			fflush(fptr);
		}
	}
	if(inode->ptr_level_1!=0){
		locate_level_delete(fptr,inode->ptr_level_1,1);
	}
	if(inode->ptr_level_2!=0){
		locate_level_delete(fptr,inode->ptr_level_2,2);
	}
	if(inode->ptr_level_3!=0){
		locate_level_delete(fptr,inode->ptr_level_3,3);
	}
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
	unsigned long int inode_loc = ftell(fptr);
	
	// Read inode
	union{
		char bytes[INODE_SIZE];
		Inode inode;
	}u_inode;
	for(int i=0; i<INODE_SIZE; ++i){
		fread(&u_inode.bytes[i],1,1,fptr);
	}
	fflush(fptr);
	
	// Release block
	block_delete_all(fptr,&(u_inode.inode));
		
	// Write superblock
	u_Superblock.block.inode_unused += 1;
	for (int i = 0; i < 20; i++) {
		fprintf(mountPoint, "%c",u_Superblock.bytes[i]);
	}
	fflush(mountPoint);
	fseek(mountPoint,0,SEEK_SET);
	
	// Write inode
	fseek(fptr,inode_loc,SEEK_SET);
	for(int i=0; i<INODE_SIZE; ++i){
		fprintf(fptr,"%c",0);
	}
	fclose(fptr);
	return 0;
}

int locate_level_read(unsigned long int entry_id, FILE *fptr, unsigned int *addr, int level){
	if(*addr == 0){
		return -1;
	}
	fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
	fseek(fptr, ((*addr)-1)*1024, SEEK_CUR);
	// Read Block
	union{
		char bytes[1024];
		PtrBlock block;
	}u_PtrBlock;
	for(int i=0;i<1024;++i){
		fread(&(u_PtrBlock.bytes[i]),1,1,fptr);
	}
	fflush(fptr);
	fseek(fptr, -1024, SEEK_CUR);
	
	if(level == 1){	
		if(u_PtrBlock.block.entry[entry_id % 255] == 0){
			return -1;
		}
		fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
		fseek(fptr, ((u_PtrBlock.block.entry[entry_id % 255])-1)*1024, SEEK_CUR);
	}else{
		locate_level_read(entry_id, fptr, &(u_PtrBlock.block.entry[entry_id/(unsigned long int)pow(255,level-1)]), level-1);
	}
	return 0;
}

Block *block_locate_read(unsigned long int fileloc, FILE *fptr, Inode *inode){
	if(fileloc < 12276){
		if(inode->ptr_direct[fileloc/1023] == 0){
			return NULL;
		}
		fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
		fseek(fptr, ((inode->ptr_direct[fileloc/1023])-1)*1024, SEEK_CUR);
	}else if(fileloc < 273141){
		if(locate_level_read((fileloc-12276)/1023, fptr, &(inode->ptr_level_1), 1) == -1){
			return NULL;
		}
	}else if(fileloc < 1474866144){
		if(locate_level_read((fileloc-273141)/1023, fptr, &(inode->ptr_level_2), 2) == -1){
			return NULL;
		}
	}else if(fileloc < 17029540341){
		if(locate_level_read((fileloc-1474866144)/1023, fptr, &(inode->ptr_level_3), 3) == -1){
			return NULL;
		}
	}else{
		return NULL;
	}
		
	// Read Block
	Block *ret = (Block *)malloc(sizeof(Block));
	fread(&(ret->dirty),1,1,fptr);
	for(int i=0;i<1023;++i){
		fread(&(ret->bytes[i]),1,1,fptr);
	}
	fflush(fptr);
	fseek(fptr, -1024, SEEK_CUR);
	return ret;
}

int file_read(unsigned long int *fileloc, char *buf, int size, Inode *inode, FILE *fptr){
	int buf_index = 0;
	while(buf_index < size){
		// Locate
		Block *t_block = block_locate_read(*fileloc, fptr, inode);
		if(t_block == NULL){
			return -1;
		}
		// Fill buffer
		int offset = ((*fileloc) % 1023);
		int bound = ((size-buf_index+offset)>1023) ? 1023 : (size-buf_index+offset);
		for(int i = offset; i < bound; ++i){
			 buf[buf_index++] = t_block->bytes[i];
		}

		// Update fileloc
		*fileloc += bound - offset;
	}

	return 0;
}

int myfs_file_read(int fd, char *buf, int count){
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
		char bytes[INODE_SIZE];
		Inode inode;
	}u_inode;
	for(int i=0; i<INODE_SIZE; ++i){
		fread(&u_inode.bytes[i],1,1,cur->fptr);
	}
	fflush(cur->fptr);
	unsigned long int filesize = ((((unsigned long int)u_inode.inode.filesize_H)<<32) & 0x300000000)+u_inode.inode.filesize_L;;
	if(filesize < (cur->location + count)){
		return -5;
	}
	// Read data
	if(file_read(&(cur->location),buf, count, &(u_inode.inode), cur->fptr) == -1){
		return -3;
	}
	return 0;
}

unsigned int find_empty_block(FILE *fptr){
	unsigned long int loc = ftell(fptr);
	fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
	unsigned int block_avail = 1;
	while(block_avail < u_Superblock.block.block_count){
		char dirty;
		fread(&dirty,1,1,fptr);
		fseek(fptr, -1, SEEK_CUR);
		if(dirty != 'd'){
			return block_avail;
		}
		fseek(fptr, 1024, SEEK_CUR);
		block_avail += 1;
	}
	return 0;
}

int locate_level_write(unsigned long int entry_id, FILE *fptr, unsigned int *addr, int level){
	if(*addr == 0){
		*addr = find_empty_block(fptr);
		if(*addr == 0){
			return -1;
		}
	}
	fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
	fseek(fptr, ((*addr)-1)*1024, SEEK_CUR);
	// Read Block
	union{
		char bytes[1024];
		PtrBlock block;
	}u_PtrBlock;
	char dch = 'd';
	fwrite(&dch,1,1,fptr);
	fflush(fptr);
	fseek(fptr, -1, SEEK_CUR);
	for(int i=0;i<1024;++i){
		fread(&(u_PtrBlock.bytes[i]),1,1,fptr);
	}
	fflush(fptr);
	fseek(fptr, -1024, SEEK_CUR);
	
	
	if(level == 1){	
		unsigned long int ptrLoc = ftell(fptr);
		if(u_PtrBlock.block.entry[entry_id % 255] == 0){
			u_PtrBlock.block.entry[entry_id % 255] = find_empty_block(fptr);
			if(u_PtrBlock.block.entry[entry_id % 255] == 0){
				return -1;
			}
			
		}
		fseek(fptr,ptrLoc,SEEK_SET);
		// Write Block
		for(int i=0; i<1024; ++i){
			fwrite(&u_PtrBlock.bytes[i],1,1,fptr);
		}
		fflush(fptr);
		fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
		fseek(fptr, ((u_PtrBlock.block.entry[entry_id % 255])-1)*1024, SEEK_CUR);
	}else{
		unsigned long int b_loc = ftell(fptr), rec_loc;
		locate_level_write(entry_id, fptr, &(u_PtrBlock.block.entry[entry_id/(unsigned long int)pow(255,level-1)]), level-1);
		rec_loc = ftell(fptr);
		fseek(fptr, b_loc, SEEK_SET);
		for(int i=0;i<1024;++i){
			fwrite(&u_PtrBlock.bytes[i],1,1,fptr);
		}
		fflush(fptr);
		fseek(fptr, rec_loc, SEEK_SET);
	}
	return 0;
}

Block *block_locate_write(unsigned long int fileloc, FILE *fptr, Inode *inode){
	if(fileloc < 12276){
		if(inode->ptr_direct[fileloc/1023] == 0){
			inode->ptr_direct[fileloc/1023] = find_empty_block(fptr);
			if(inode->ptr_direct[fileloc/1023] == 0){
				return NULL;
			}
		}
		fseek(fptr, 20+u_Superblock.block.inode_section_size, SEEK_SET);
		fseek(fptr, ((inode->ptr_direct[fileloc/1023])-1)*1024, SEEK_CUR);
	}else if(fileloc < 273141){
		if(locate_level_write((fileloc-12276)/1023, fptr, &(inode->ptr_level_1), 1) == -1){
			return NULL;
		}
	}else if(fileloc < 1474866144){
		if(locate_level_write((fileloc-273141)/1023, fptr, &(inode->ptr_level_2), 2) == -1){
			return NULL;
		}
	}else if(fileloc < 17029540341){
		if(locate_level_write((fileloc-1474866144)/1023, fptr, &(inode->ptr_level_3), 3) == -1){
			return NULL;
		}
	}else{
		return NULL;
	}
		
	// Read Block
	Block *ret = (Block *)malloc(sizeof(Block));
	fread(&(ret->dirty),1,1,fptr);
	for(int i=0;i<1023;++i){
		fread(&(ret->bytes[i]),1,1,fptr);
	}
	fflush(fptr);
	fseek(fptr, -1024, SEEK_CUR);
	return ret;
}

int file_write(unsigned long int *fileloc, char *buf, int size, Inode *inode, FILE *fptr){
	int buf_index = 0;
	while(buf_index < size){
		// Locate
		Block *t_block = block_locate_write(*fileloc, fptr, inode);
		if(t_block == NULL){
			return -1;
		}
		// Fill block
		t_block->dirty = 'd';
		int offset = ((*fileloc) % 1023);
		int bound = ((size-buf_index+offset)>1023) ? 1023 : (size-buf_index+offset);
		for(int i = offset; i < bound; ++i){
			t_block->bytes[i] = buf[buf_index++];
		}
		// Write block
		fwrite(&t_block->dirty,1,1,fptr);
		for(int i=0; i<1023; ++i){
			fwrite(&t_block->bytes[i],1,1,fptr);
		}
		fflush(fptr);
		// Update fileloc
		*fileloc += bound - offset;
	}
	unsigned long int filesize = ((((unsigned long int)inode->filesize_H)<<32) & 0x300000000)+inode->filesize_L;
	if(*fileloc > filesize){
		filesize = *fileloc;
		inode->filesize_H = (char) ((filesize >> 32) & 0xff);
		inode->filesize_L = filesize & 0xFFFFFFFF;
	}
	return 0;
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
		char bytes[INODE_SIZE];
		Inode inode;
	}u_inode;
	for(int i=0; i<INODE_SIZE; ++i){
		fread(&u_inode.bytes[i],1,1,cur->fptr);
	}
	fflush(cur->fptr);
		
	// Write data
	if(file_write(&(cur->location), buf, count, &(u_inode.inode), cur->fptr) == -1){
		return -3;
	}

	// Write inode
	fseek(cur->fptr, cur->inode_location, SEEK_SET);
	for(int i=0; i<INODE_SIZE; ++i){
		fwrite(&u_inode.bytes[i],1,1,cur->fptr);
	}
	fflush(cur->fptr);
	
	// Write superblock
	fseek(cur->fptr,0, SEEK_SET);
	for(int i=0; i<20; ++i){
		fwrite(&u_Superblock.bytes[i],1,1,cur->fptr);
	}
	fflush(cur->fptr);
	
	return 0;
}

FileStatus *myfs_file_list(unsigned int *count){
	if(mountPoint == NULL){
		return NULL;
	}
	if(u_Superblock.block.inode_unused == u_Superblock.block.inode_count){
		*count = 0;
		return NULL;
	}
	*count = u_Superblock.block.inode_count - u_Superblock.block.inode_unused;
	// Create FileStatus array
	FileStatus *ret = (FileStatus *)malloc(sizeof(FileStatus)*(u_Superblock.block.inode_count - u_Superblock.block.inode_unused));
	
	// Scan inode
	FILE *fptr;
	if(!(fptr = fopen(mountName,"rb"))){
		return NULL;
	}
	fseek(fptr,20,SEEK_SET);
	unsigned int fstatus_index = 0;
	while(ftell(fptr)<(20+u_Superblock.block.inode_section_size)){
		union{
			char bytes[INODE_SIZE];
			Inode inode;
		}u_inode;
		for(int i=0; i<INODE_SIZE; ++i){
			fread(&(u_inode.bytes[i]),1,1,fptr);
		}
		fflush(fptr);
		if(strlen(u_inode.inode.filename)){
			strcpy(ret[fstatus_index].filename,u_inode.inode.filename);
			ret[fstatus_index].filesize = ((((unsigned long int)u_inode.inode.filesize_H)<<32) & 0x300000000)+u_inode.inode.filesize_L;
			++fstatus_index;
		}
	}
	return ret;
}

int myfs_file_seek(int fd, long int offset, long int origin){
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
		char bytes[INODE_SIZE];
		Inode inode;
	}u_inode;
	for(int i=0; i<INODE_SIZE; ++i){
		fread(&u_inode.bytes[i],1,1,cur->fptr);
	}
	fflush(cur->fptr);
	unsigned long int filesize = ((((unsigned long int)u_inode.inode.filesize_H)<<32) & 0xffffffff)+u_inode.inode.filesize_L;
	
	// Macro
	if(origin == MY_SEEK_CUR){
		origin = cur->location;
	}else if(origin == MY_SEEK_SET){
		origin = 0;
	}else if(origin == MY_SEEK_END){
		origin = filesize-1;
	}
	
	// Change offset
	if(((origin + offset) < 0)||((origin + offset) >= filesize)){
		return -3;
	}else{
		cur->location = origin + offset;
	}
	return 0;
}
