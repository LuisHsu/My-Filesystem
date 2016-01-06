#include <stdio.h>
#include <math.h>

#define INODE_SIZE 317

FILE *fout = NULL;

void count_size(int B){
	unsigned int inode_count,inode_section,block_count;
	if(B<2048){
		inode_section = ((unsigned int)((B<<11)/1341.0))*512*INODE_SIZE;
		block_count = (B<<10)-(inode_section>>10);
		inode_count = inode_section/INODE_SIZE;
	}else{
		inode_section = ((unsigned int)(B/1341.0))*1048576*INODE_SIZE;
		block_count = (B-(inode_section>>20))<<10;
		inode_count = inode_section/INODE_SIZE;
	}	
	fprintf(fout,"%u,%u,",B,block_count);
	fprintf(fout,"%u,%u\n",inode_count,inode_section);
}

int main(){
	fout=fopen("calc.csv","w");
	for(int B=1;B<=1048576;++B){
		count_size(B);
	}
	fclose(fout);
	return 0;
}
