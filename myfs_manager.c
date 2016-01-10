#include <stdio.h>
#include <string.h>

#include "myfs.h"

void show_help();
int on_exit();
void trimFilename(char *filename);

void create_format();
void destroy();
void mountfs();
void umountfs();
void create_file();

int main(int argc, const char *argv[])
{
	show_help();
	while (1) {
		char Command[30];
		printf(">");
		scanf("%s",Command);
		
		if(!strcmp(Command,"exit")){
			if(!on_exit()){
				break;
			}
		}
		if((!strcmp(Command,"make"))||(!strcmp(Command,"format"))){
			while(getchar()!='\n');
			create_format();
		}
		if(!strcmp(Command,"destroy")){
			while(getchar()!='\n');
			destroy();
		}
		if(!strcmp(Command,"mount")){
			while(getchar()!='\n');
			mountfs();
		}
		if(!strcmp(Command,"umount")){
			while(getchar()!='\n');
			umountfs();
		}
		if(!strcmp(Command,"create")){
			while(getchar()!='\n');
			create_file();
		}
	}
	return 0;
}

void show_help(){
	printf(
	"=== Myfs Manager ===\n"
	"Command List:\n"
	"make\tformat\tdestroy\n"
	"mount\tumount\texit\n"
	"create\n"
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

void create_format(){
	char filename[256];
	int max_size;

	while(1){
		printf("Input virtual disk filename\n-> ");
		if(fgets(filename,256,stdin)==NULL){
			printf("Input error!\n");
		}else{
			trimFilename(filename);
			break;
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

void destroy(){
	char filename[256];
	printf("Input disk filename to destroy\n-> ");
	fgets(filename,256,stdin);
	trimFilename(filename);
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

void mountfs(){
	char filename[256];
	printf("Input disk filename to mount:\n-> ");
	fgets(filename,256,stdin);
	trimFilename(filename);
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

void create_file(){
	char filename[256];
	printf("Input filename to create:\n-> ");
	fgets(filename,256,stdin);
	trimFilename(filename);
	switch(myfs_file_create(filename)){
		case -1:
			printf("No mounted filesystem!\n");
			break;
		case -2:
			printf("Inode full!\n");
			break;
		case -3:
			printf("Can't write to disk file! Reason:\n%s\n",strerror(errno));
			break;
		default:
			printf("Created!\n");
	}
}
