#include <stdio.h>
#include <string.h>

#include "myfs.h"

void show_help();

void create_format();
int on_exit();

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
		if((!strcmp(Command,"create"))||(!strcmp(Command,"format"))){
			while(getchar()!='\n');
			create_format();
		}
	}
	return 0;
}

void show_help(){
	printf(
	"=== Myfs Manager ===\n"
	"Command List:\n"
	"create\tformat\texit\n"
	);
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
			break;
		}
	}

	while(1){
		printf("Input filesystem max size(MB)\n** notice: size must <= 1048576 MB **\n-> ");
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
	}
}
