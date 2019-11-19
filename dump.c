/* 

dump.c - dump a hex file to screen 
kurt theis <ktheis@landfall.net>
nov 18 06

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

int errno;

int main(int argv, char *argc[]){

FILE *infile;
short int ch;
short int ascii[16];
int ct=0;
int addr=0;
int n;
int FLAG=0;

	infile = fopen(argc[1],"r");
	if (infile == NULL){
		 perror("fopen");
		 exit(-1);
	}
	for (n=0; n<16; n++) ascii[n]=0;	/* initialize */
	ct = 0;
	printf("%4.4X  ",addr);
	while(1){
		if (!FLAG)
			ch = fgetc(infile);
		ascii[ct++] = ch;
		if (feof(infile)) FLAG=1;
		if ((ch < 0) || (ch > 255)) ch = 0;
		printf("%2.2X ",ch);
		if (ct == 16){
			printf("   ");
			for (n=0; n<16; n++)
				if (isprint(ascii[n])) printf("%c",ascii[n]); else printf(".");	
			if (FLAG) break;
			addr += 16;
			printf("\n%4.4X  ",addr);
			ct = 0;
		}
	}
	fclose(infile);
	printf("\n");
}

	
