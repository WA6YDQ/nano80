/*
 * ihex - convert .com output of asm80 into intel HEX (.hex) file
 * (also create a binary version, .bhex, used for testing)
 *
 * (C) k theis 11/2019
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXLEN 80

int main(int argc, char **argv) {
	FILE *infile, *outfile, *hexfile;
	char val;
	char infilename[MAXLEN]={};
        char outfilename[MAXLEN]={}; 
	char hexfilename[MAXLEN]={};
	int cc, cnt, adr, dat, n, hi, lo = 0;
	int chksum = 0;

	if (argc != 2) {	// show usage
		printf("Usage: %s [.com file]\n",argv[1]);
		printf("Convert .com file to intel .hex file\n");
		exit(0);
	}

	infile = fopen(argv[1],"r");
	if (infile == NULL) {
		printf("error opening %s\n",argv[1]);
		exit(1);
	}
	
	/* derive outfile name */
	if (strlen(argv[1]) > MAXLEN) {
			printf("Filename too long (%s)\n",argv[1]);
			exit(1);
	}
	strcpy(infilename,argv[1]);
	for (n=strlen(infilename); n > 0; n--) {
		if (infilename[n] == '.') break;	// position = n
	}
	if (!n) {
		strcpy(outfilename,infilename);
		strcpy(hexfilename,infilename);
	}
	if  (n) {
		strncpy(outfilename,infilename,n);
		strncpy(hexfilename,infilename,n);
	}
	strcat(outfilename,".hex");	// encoded as ascii
	strcat(hexfilename,".bhex");	// encoded as binary

	outfile = fopen(outfilename,"w");
	if (outfile == NULL) {
		printf("Cannot create file %s\n",outfilename);
		exit(1);
	}

	hexfile = fopen(hexfilename,"w");
	if (hexfile == NULL) {
		printf("Cannot create file %s\n",hexfilename);
		exit(1);
	}

	while (1) {	// get file size
		fgetc(infile);
		if (feof(infile)) break;
		cnt++;
	}
	// cnt is filesize
	//printf("filesize is %d\n",cnt);
	rewind(infile);
	while (cnt-adr>0) {
		fprintf(hexfile,":");	// start of record
		fprintf(outfile,":");
		//printf("first cnt-adr=%d\n",cnt-adr);
		if (cnt-adr >= 16) 
			cc = 16;
		else
			cc = cnt-adr;
		chksum = cc;			// start checksum for this line
		val = cc;
		fprintf(hexfile,"%c",val);	// write byte count to binary file
		fprintf(outfile,"%2.2X",val);	// and ascii file
		hi = (adr&0xff00) >> 8;
		lo = adr&0x00ff;	// calculate hi/lo address bytes
		chksum += hi;
		chksum += lo;
		fprintf(hexfile,"%c",hi);	// output address hi and lo to binary file
		fprintf(hexfile,"%c",lo);
		fprintf(outfile,"%2.2X",hi);	// and ascii file
		fprintf(outfile,"%2.2X",lo);
		val = 0;	// normal record type
		fprintf(hexfile,"%c",val);	// binary file
		fprintf(outfile,"%2.2X",val);
		/* now send data bytes */
		for (n=0; n!=cc; n++) {
			val = fgetc(infile);
			fprintf(hexfile,"%c",val);	// binary file
			fprintf(outfile,"%2.2X",val);	// ascii file
			chksum += val;
			adr++;		// make sure address keeps up with bytes send
		}
		val = ((uint8_t)~chksum)+1;	// make two's compliment
		//val = 0xaa;	// dummy checksum
		fprintf(hexfile,"%c",val);	// binary file
		fprintf(outfile,"%2.2X",val);
		fprintf(outfile,"\n");		// need \n here
		if (cc >= 16) continue;
		//printf("last cnt-adr=%d\n",cnt-adr);
		/* now send last record  :00000001FF */
		fprintf(hexfile,":");		// binary file
		fprintf(outfile,":");		// ascii file
		val = 0;
		fprintf(hexfile,"%c",val);	// bytecount
		fprintf(outfile,"%2.2X",val);
		fprintf(hexfile,"%c",val);	// adr hi
		fprintf(outfile,"%2.2X",val);
		fprintf(hexfile,"%c",val);	// adr lo
		fprintf(outfile,"%2.2X",val);
		val = 1;
		fprintf(hexfile,"%c",val);	// end char
		fprintf(outfile,"%2.2X",val);
		val = 0xff;
		fprintf(hexfile,"%c",val);	// end checksum
		fprintf(outfile,"%2.2X\n",val);
		break;
	}
	// done
	fflush(outfile); fflush(hexfile);
	fclose(outfile); fclose(hexfile);
	fclose(infile);

	exit(0);
}
	
