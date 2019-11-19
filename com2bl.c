/* com2bl.c - convert 8080 .com file to something the boot loader 
 * can send. Specifically, add the file length as the first 2 bytes to the file.
 *
 * k theis 2019 
*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	FILE *infile, *outfile;
	int cnt;
	char chr;

	if (argc == 1) {	// usage
		printf("Usage: %s [infile] [outfile]\n",argv[0]);
		printf("convert 8080 .com file to a file suitable for the bootloader of nano80\n");
		printf("\n");
		exit(0);
	}

	infile = fopen(argv[1],"r");
	if (infile == NULL) {
		fprintf(stderr,"Cannot open source file %s\n",argv[1]);
		exit(1);
	}

	outfile = fopen(argv[2],"w");
	if (outfile == NULL) {
		fprintf(stderr,"Cannot create destination file %s\n",argv[2]);
		exit(1);
	}

	cnt = 0;
	while (feof(infile) == 0) {
		fgetc(infile);
		cnt++;
	}
	rewind(infile);
	cnt--;

	printf("Filesize is of %s is %d bytes\n",argv[1],cnt);
	fprintf(outfile,"%c",((cnt & 0xff00) >> 8));	// write MSB of filesize
	fprintf(outfile,"%c",cnt & 0xff);		// write LSB of filesize

	while(feof(infile) == 0) {
		chr = fgetc(infile);
		if (feof(infile)) break;
		fprintf(outfile,"%c",chr);	// write file
	}

	printf("File written\n");

	fclose(infile);
	fclose(outfile);
	exit(0);
}

