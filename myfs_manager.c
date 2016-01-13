#include <stdio.h>
#include <string.h>

#include "myfs.h"

void show_help();
int on_exit();
void trimFilename(char *filename);

void make_format(char *filename);
void destroy(char *filename);
void mountfs(char *filename);
void umountfs();
void create_file(char *filename);
void delete_file(char *filename);
void open_file(char *filename);
void close_file(char *fdStr);
void write_file(char *fdStr);
void read_file(char *fdStr);
void save_file(char *filename);

int main(int argc, const char *argv[])
{
	show_help();
	while (1) {
		char Command[30],filename[256];
		printf(">");
		scanf("%s",Command);
		memset(filename,0,256);
		unsigned char catch = 0, hasArg = 0, name_index = 0;
		while((catch=getchar())!='\n'){
			if(!hasArg){
				if(catch!=' '){
					hasArg = 1;
					filename[name_index++] = catch;
				}
			}else{
				filename[name_index++] = catch;
			}
		}
		if(!strcmp(Command,"exit")){
			if(!on_exit()){
				break;
			}
		}
		if((!strcmp(Command,"make"))||(!strcmp(Command,"format"))){
			make_format(filename);
		}
		if(!strcmp(Command,"destroy")){
			destroy(filename);
		}
		if(!strcmp(Command,"mount")){
			mountfs(filename);
		}
		if(!strcmp(Command,"umount")){
			umountfs();
		}
		if(!strcmp(Command,"create")){
			create_file(filename);
		}
		if(!strcmp(Command,"delete")){
			delete_file(filename);
		}
		if(!strcmp(Command,"open")){
			open_file(filename);
		}
		if(!strcmp(Command,"close")){
			close_file(filename);
		}
		if(!strcmp(Command,"help")){
			show_help();
		}
		if(!strcmp(Command,"write")){
			write_file(filename);
		}
		if(!strcmp(Command,"read")){
			read_file(filename);
		}
		if(!strcmp(Command,"save")){
			save_file(filename);
		}
	}
	return 0;
}

void show_help(){
	printf(
	"=== Myfs Manager ===\n"
	"Command List:\n"
	"make\tformat\tdestroy\tmount\n"
	"umount\texit\tcreate\tdelete\n"
	"open\twrite\tread\tsave\n"
	"help\n"
	);
}

void trimFilename(char *filename){
	int newline=0;
	for(int i=0;i<256;++i){
		if(!newline){
			if(filename[i]=='\n'){
				filename[i]='\0';
			}
		}else{
			filename[i]='\0';
		}	
	}	
}

int on_exit(){
	if(myfs_umount()>=0){
		return 0;		
	}else{
		printf("Couldn't exit! reason:\n%s\n",strerror(errno));
		return -1;
	}
}

void make_format(char *filename){
	int max_size;
	if(!strlen(filename)){
		while(1){
			printf("Input virtual disk filename\n-> ");
			if(fgets(filename,256,stdin)==NULL){
				printf("Input error!\n");
			}else{
				trimFilename(filename);
				break;
			}
		}
	}

	while(1){
		printf("Input filesystem max size(MiB)\n** notice: size must <= 1048576 MiB **\n-> ");
		scanf("%d",&max_size);
		if((max_size>1048576)||(max_size<1)){
			printf("Size over range!\n");
		}else{
			break;
		}
	}
	
	if(myfs_create(filename,max_size)){
		printf("Create/Format fail! Reason:\n%s\n",strerror(errno));
	}else{
		printf("Create/Format finished!\n");
		FILE *fin = fopen(filename,"r");
		union{
			unsigned char bytes[20];
			Superblock superblock;
		}buf;
		for(int i=0; i<20; ++i){
			fscanf(fin,"%c",&buf.bytes[i]);
		}
		fclose(fin);
		printf("%u blocks, %u inodes\n",buf.superblock.block_count,buf.superblock.inode_count);
	}
}

void destroy(char *filename){
	if(!strlen(filename)){
		printf("Input disk filename to destroy\n-> ");
		fgets(filename,256,stdin);
		trimFilename(filename);
	}
	switch(myfs_destroy(filename)){
		case -1:
			printf("Remove failed! Reason:\n%s\n",strerror(errno));
			break;
		case -2:
			printf("Unmount failed! Reason:\n%s\n",strerror(errno));
			break;
		default:
			printf("Destroy success!\n");
	}
}

void mountfs(char *filename){
	if(!strlen(filename)){
		printf("Input disk filename to mount:\n-> ");
		fgets(filename,256,stdin);
		trimFilename(filename);
	}
	switch(myfs_mount(filename)){
		case -1:
			printf("Can't open file! Reason:\n%s\n",strerror(errno));
			break;
		case -2:
			printf("Unmount failed! Reason:\n%s\n",strerror(errno));
			break;
		default:
			printf("%s mounted!\n",filename);
	}
}

void umountfs(){
	switch(myfs_umount()){
		case 1:
			printf("No mounted disk!\n");
			break;
		case -1:
			printf("File close failed! Reason:\n%s\n",strerror(errno));
			break;
		default:
			printf("Unmounted!\n");
	}
}

void create_file(char *filename){
	if(!strlen(filename)){
		printf("Input filename to create:\n-> ");
		fgets(filename,256,stdin);
		trimFilename(filename);
	}
	switch(myfs_file_create(filename)){
		case -1:
			printf("No mounted filesystem!\n");
			break;
		case -2:
			printf("No unused inode!\n");
			break;
		case -3:
			printf("Can't write to disk file! Reason:\n%s\n",strerror(errno));
			break;
		case -4:
			printf("File exists!\n");
			break;
		default:
			printf("Created!\n");
	}
}

void delete_file(char *filename){
	if(!strlen(filename)){
		printf("Input filename to delete:\n-> ");
		fgets(filename,256,stdin);
		trimFilename(filename);
	}
	switch(myfs_file_delete(filename)){
		case -1:
			printf("No mounted filesystem!\n");
			break;
		case -2:
			printf("There are no file in the disk!\n");
			break;
		case -3:
			printf("Can't write to disk file! Reason:\n%s\n",strerror(errno));
			break;
		case -4:
			printf("File not found!\n");
			break;
		default:
			printf("Deleted!\n");
	}
}

void open_file(char *filename){
	if(!strlen(filename)){
		printf("Input filename to open:\n-> ");
		fgets(filename,256,stdin);
		trimFilename(filename);
	}
	int fd;
	switch(fd = myfs_file_open(filename)){
		case -1:
			printf("No mounted filesystem!\n");
			break;
		case -2:
			printf("There are no file in the disk!\n");
			break;
		case -3:
			printf("Can't write to disk file! Reason:\n%s\n",strerror(errno));
			break;
		case -4:
			printf("File not found!\n");
			break;
		default:
			printf("Opened! File Descriptor: %d\n",fd);
	}
}

void close_file(char *fdStr){
	int fd;
	if(!strlen(fdStr)){
		printf("Input file descriptor to close:\n-> ");
		scanf("%d",&fd);
	}else{
		fd = atoi(fdStr);
	}
	
	switch(myfs_file_close(fd)){
		case -1:
			printf("No mounted filesystem!\n");
			break;
		case -2:
			printf("There are no file in the disk!\n");
			break;
		case -3:
			printf("Can't close file! Reason:\n%s\n",strerror(errno));
			break;
		case -4:
			printf("File descriptor not found!\n");
			break;
		default:
			printf("Closed!\n");
	}
}

void write_file(char *fdStr){
	int fd;
	if(!strlen(fdStr)){
		printf("Input file descriptor to write:\n-> ");
		scanf("%d",&fd);
	}else{
		fd = atoi(fdStr);
	}
	int data_size;
	printf("Input data size:\n-> ");
	scanf("%d",&data_size);
	while(getchar()!='\n');
	char buf[data_size+1];
	memset(buf,0,data_size+1);
	printf("Input data:\n-> ");
	fgets(buf,data_size+1,stdin);
	switch(myfs_file_write(fd,buf,data_size)){
		case -1:
			printf("No mounted filesystem!\n");
			break;
		case -2:
			printf("There are no file in the disk!\n");
			break;
		case -3:
			printf("Block allocate error!\n");
		break;
		case -4:
			printf("File descriptor not found!\n");
			break;
		default:
			printf("Written!\n");
	}
}

void read_file(char *fdStr){
	int fd;
	if(!strlen(fdStr)){
		printf("Input file descriptor to read:\n-> ");
		scanf("%d",&fd);
	}else{
		fd = atoi(fdStr);
	}
	int data_size;
	printf("Input read size:\n-> ");
	scanf("%d",&data_size);
	while(getchar()!='\n');
	char buf[data_size+1];
	memset(buf,0,data_size+1);
	switch(myfs_file_read(fd,buf,data_size)){
		case -1:
			printf("No mounted filesystem!\n");
			break;
		case -2:
			printf("There are no file in the disk!\n");
			break;
		case -3:
			printf("Block read error!\n");
		break;
		case -4:
			printf("File descriptor not found!\n");
			break;
		case -5:
			printf("Read size too big!\n");
			break;
		default:
			printf("== content ==\n");
			for(int i=0; i<data_size; ++i){
				printf("%c",buf[i]);
			}
			printf("\n== Successful read! ==\n");
	}
}

void save_file(char *filename){
	if(!strlen(filename)){
		printf("Input filename to save:\n-> ");
		fgets(filename,256,stdin);
		trimFilename(filename);
	}
	int error_code;
	if((error_code = myfs_file_create(filename)) != 0){
		printf("Unable to create file! [%d]\n",error_code);
		return;
	}
	int fd;
	if((fd = myfs_file_open(filename)) < 0){
		printf("Unable to open file! [%d]\n",fd);
		return;
	}
	FILE *fin;
	if(!(fin = fopen(filename,"rb"))){
		printf("Unable to open source file!\n");
		return;
	}
	char buf[1023];
	int data_size;
	while((data_size = fread(buf,1,1023,fin)) > 0){
		if((error_code = myfs_file_write(fd,buf,data_size)) < 0){
			printf("Write error! [%d]\n",error_code);
		}
	}
	fclose(fin);
	if((error_code = myfs_file_close(fd)) < 0){
		printf("Close error! [%d]\n",error_code);
	}
	printf("%s Saved!\n",filename);
}
