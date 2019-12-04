/*
 
---------------------------------------- 
asm80.c	8080 assembler
version 4.2
07-13-2008	<theis.kurt@gmail.com> 	


Revised 12/2019 - allow loading . before certain key words 
(.db, .equ etc)

Revised 9/2/2008 - better error checking



This has more error checking, all the 
directives work as they should, and
labels can be used for almost anything.

I'm releasing this as GNU GPL 3 license. 
If you find this useful, send me a note. 
Also if you find any errors, please
let me know.

To compile this use:
cc -o asm80 asm80.c
on (hopefully) any c compiler.

This was done on a Linux system, but 
should work just fine on anything else.

To assemble a program:
asm80 file.asm
Will produce 2 files:
file.prn (print out file)
file.com (com format)

-----------------------------------------

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define DEBUG 0			/* 1=debugging turned on, 0=off */
#define MAXTOKEN 100000		/* no more than 100K tokens */
#define MAXLABEL 10000		/* no more than 10K labels */
#define NOP_CODE 0		/* code for nop op-code for 8080 */
#define MAXLINE	 74		/* max line length of quoted string */

int tokenCount = 0;		/* count the number of tokens in input file */
int address = 0;		/* running count used in PASS 2 and 3 */
int *label_address;		/* assign address to a label */
int label_count = 0;		/* count the number of labels found/used */
int codenum = 0;		/* opcode value used in PASS 3 */
int idx = 0;			/* index counter used in PASS 3 */
int *memory;			/* array holding all assembled data */
int addr, addr1, addr2;		/* used to determine 3 byte opcode labels */
int NCFLAG = 0;			/* command line option - remove colon from label (see below comments) */
int FLAG = 0;			/* Gen Purpose FLAG */
int maxAddress = 0;		/* address at end of PASS 2 - used to set memory in PASS 3 */
int ERRFLAG = 0;		/* set if fatal errors found during assembly */

char sourceFile[80];		/* name of source code filename */
char printFile[80];		/* name of print filename */
char objFile[80];		/* name of object filename */
char token[MAXTOKEN][120];	/* used in PASS 1 */
char ptoken[MAXTOKEN][120];	/* processed tokens */
char singleWord[120];		/* used in PASS 1 */
char singleByte;		/* used in PASS 1 */
char label[MAXLABEL][80];	/* hold labels - max length 80 characters */


/* multi byte op codes are like mov a,b, jmp xxx, pop bc, etc */
/* single byte op codes are like nop, stc, ret */ 
/* these are multi-byte op codes (used in isMbyteOpCode) */
char mbopcode[20][5] = {"mov","mvi","ldax","stax","lxi","push","pop", \
			"add","sub","inr","dcr","cmp","ana","ora","xra","adc", \
			"sbb","dad","inx","dcx"};

/* assign list of opcodes to variables */

int opcode_bytes[256] = { \
1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,3,1,1,1,1,2, \
1,1,1,1,1,1,1,2,1,1,3,3,1,1,1,2,1,1,1,3,1,1,1,2, \
1,1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,1,1,1,1,1,1,1, \
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
1,1,1,3,3,3,1,2,1,1,1,3,1,3,3,2,1,1,1,3,2,3,1,2, \
1,1,1,3,2,3,1,2,1,1,1,3,1,3,1,2,1,1,1,3,1,3,1,2, \
1,1,1,3,1,3,1,2,1,1,1,3,1,3,1,2,1 };

char opcodes[256][10] = { \
"nop", "lxi b", "stax b", "inx b", "inr b", "dcr b", \
"mvi b", "rlc", "__8", "dad b", "ldax b", "dcx b", \
"inr c", "dcr c", "mvi c", "rrc", "_10", "lxi d", \
"stax d", "inx d", "inr d", "dcr d", "mvi d", "ral", \
"_18", "dad d", "ldax d", "dcx d", "inr e", "dcr e", \
"mvi e", "rar", "_20", "lxi h", "shld", "inx h", \
"inr h", "dcr h", "mvi h", "daa", "_28", "dad h", \
"lhld", "dcx h", "inr l", "dcr l", "mvi l", "cma", \
"_30", "lxi sp", "sta", "inx sp", "inr m", "dcr m", \
"mvi m", "stc", "_38", "dad sp", "lda", "dcx sp", \
"inr a", "dcr a", "mvi a", "cmc", "mov b,b", "mov b,c", \
"mov b,d", "mov b,e", "mov b,h", "mov b,l", "mov b,m", "mov b,a", \
"mov c,b", "mov c,c", "mov c,d", "mov c,e", "mov c,h", "mov c,l", \
"mov c,m", "mov c,a", "mov d,b", "mov d,c", "mov d,d", "mov d,e", \
"mov d,h", "mov d,l", "mov d,m", "mov d,a", "mov e,b", "mov e,c", \
"mov e,d", "mov e,e", "mov e,h", "mov e,l", "mov e,m", "mov e,a", \
"mov h,b", "mov h,c", "mov h,d", "mov h,e", "mov h,h", "mov h,l", \
"mov h,m", "mov h,a", "mov l,b", "mov l,c", "mov l,d", "mov l,e", \
"mov l,h", "mov l,l", "mov l,m", "mov l,a", "mov m,b", "mov m,c", \
"mov m,d", "mov m,e", "mov m,h", "mov m,l", "hlt", "mov m,a", \
"mov a,b", "mov a,c", "mov a,d", "mov a,e", "mov a,h", "mov a,l", \
"mov a,m", "mov a,a", "add b", "add c", "add d", "add e", \
"add h", "add l", "add m", "add a", "adc b", "adc c", \
"adc d", "adc e", "adc h", "adc l", "adc m", "adc a", \
"sub b", "sub c", "sub d", "sub e", "sub h", "sub l", \
"sub m", "sub a", "sbb b", "sbb c", "sbb d", "sbb e", \
"sbb h", "sbb l", "sbb m", "sbb a", "ana b", "ana c", \
"ana d", "ana e", "ana h", "ana l", "ana m", "ana a", \
"xra b", "xra c", "xra d", "xra e", "xra h", "xra l", \
"xra m", "xra a", "ora b", "ora c", "ora d", "ora e", \
"ora h", "ora l", "ora m", "ora a", "cmp b", "cmp c", \
"cmp d", "cmp e", "cmp h", "cmp l", "cmp m", "cmp a", \
"rnz", "pop b", "jnz", "jmp", "cnz", "push b", \
"adi", "rst 0", "rz", "ret", "jz", "_cb", \
"cz", "call", "aci", "rst 1", "rnc", "pop d", \
"jnc", "out", "cnc", "push d", "sui", "rst 2", \
"rc", "_d9", "jc", "in", "cc", "_dd", \
"sbi", "rst 3", "rpo", "pop h", "jpo", "xthl", \
"cpo", "push h", "ani", "rst 4", "rpe", "pchl", \
"jpe", "xchg", "cpe", "_ed", "xri", "rst 5", \
"rp", "pop psw", "jp", "di", "cp", "push psw", \
"ori", "rst 6", "rm", "sphl", "jm", "ei", \
"cm", "_fd", "cpi", "rst 7" };



FILE *sourcefile, *objfile, *prnfile;

/* ---------------------------------------------- */
/* ------------- Code Starts Here --------------- */
/* ---------------------------------------------- */


/*
*	isReserved() -  check if passed value is a reserved
*			word or character. Typically an opcode
*			or some such.
*
*/
int isReserved(char *token){	/* return 1 if match to reserved work, 0 if not */
int n;
const char reserved[21][8] = {"db",".db","DB",".DB", \
    		   "ds",".ds","DS",".DS", \
	           "dw",".dw","DW",".DW", \
     		   "org",".org","ORG",".ORG", \
		   "equ",".equ","EQU",".EQU"};

	for (n=0; n<255; n++){
		if (strcmp(token,opcodes[n])==0)	// test all opcodes
			return 1;
	}

	for (n=0; n<20; n++) {
		if (strcmp(token,reserved[n])==0)	// other reserved words
		    	return 1;
	}

	return 0;	/* everything else is OK */
}




/*
*	isOpcode() - check if passed value is a valid
*	opcode in 8080 format. If not, return -1
*	else return the size of the opcode, 1, 2 or 3
*/

int isOpcode(char *token){	/* check if opcode - return byte size or -1 */
int n;
	for (n=0; n<256; n++)
		if (strcmp(token,opcodes[n])==0)
			return opcode_bytes[n];
	return -1;	/* token not an opcode */
}





/*
*	isLabel() - check is passed token is a valid label
*	Return -1 if token is not in the pre-defined list
*	of labels, or if it is return the address or number
*	of the label references
*/

int isLabel(char *token){	/* check if token is a label - return -1 or address */
int n;
	for (n=0; n<label_count; n++){
		if (strcmp(label[n],token)==0)
			return label_address[n];
	}
	return -1;	/* token not a label */
}




/*
*	getOpCode() - find value of opcode from supplied
*	token. Should never see an error since program 
*	calls isOpcode() before calling this. Return
*	single byte value of op code.
*/

int getOpCode(char *token){	/* find opcode, return value */
int n;
	for (n=0; n<256; n++)
		if (strcmp(token,opcodes[n])==0)
			return n;
	return -1;	/* token not an opcode */
}





/*
*	hex2dec() and getAddr()
*
*	getAddr() returns the int value of a passed token
*	the token is a string that represents either a decimal
*	or hex number in the range from 0 to 65535
*	Return value is -1 if value is out of range.
*	
*	hex2dec() is called by getAddr()
*/

int hex2dec(char h){            /* convert single hex digit to decimal */
int byte;
        if(isalpha(h)){     /* 1st digit */
                byte = toupper(h);
                byte = byte - 55;
        }
        if (isdigit(h)){
                byte = h - 48;
        }
        return(byte);
}

int getAddr(char *addr){	/* get address of passed variable - numbers can be 1234, 1234h, 0x1234 */
int n;
char value[10];

	if (strlen(addr)==0)	/* discard bad token (last token will be null in pass 2) */
		return -1;


	/* for 0xnn type of hex numbers */
	if (addr[1] == 'x') {
		n=strtol(addr,NULL,0);
		return n;
	}

	/* for 10h, 1234h type of hex numbers */
	if ((addr[strlen(addr)-1] == 'h') || (addr[strlen(addr)-1] == 'H')){ /* hex number */
		if (strlen(addr)==3){	/* ex 45H */
			n = hex2dec(addr[1]);
			n += hex2dec(addr[0]) * 16;
			if ((n < 0) || (n > 255))	/* error checking */
				return -1;
			return n;
		}
		if (strlen(addr)==4){	/* ex 100H */
			n = hex2dec(addr[2]);
			n += hex2dec(addr[1]) * 16;
			n += hex2dec(addr[0]) * 256;
			if ((n < 0) || (n > 4095))	/* error checking */
				return -1;
			return n;
		}
		if (strlen(addr)==5){	/* ex 2345H */
			n = hex2dec(addr[3]);
			n += hex2dec(addr[2]) * 16;
			n += hex2dec(addr[1]) * 256;
			n += hex2dec(addr[0]) * 4096;
			if ((n < 0) || (n > 65535))	/* error checking */
				return -1;
			return n;
		}
		return -1;	/* wrong size */
	}
	if ((strlen(addr)>1) && (atoi(addr)==0))	/* bad number */
		return -1;

	if ((strlen(addr)==1) && (isdigit(addr[0])==0))	/* not a number */
		return -1;

	/* address is decimal */
	n = atoi(addr);
	if ((n < 0) || (n > 65535))		/* error checking */
		return -1;
	return n;
}




//	isMbyteOpCode()
//	check if passed token is a multi-byte op code 
//	return 0 if not, 1 if it is 

int isMbyteOpCode(char *opcode){
	int n;
	for (n=0; n<20; n++){
		if (strcmp(opcode,mbopcode[n])==0)
			return 1;
	}
	return 0;	/* not a multi-byte op code */
}




//	usage() - display usage and exit

int usage(void){
	printf("\n\nasm80 v4.0 - an 8080 assembler");
	printf("\nCompiled %s %s\n",__TIME__,__DATE__);
	printf("\nUsage: asm80 [-nc] filename");
	printf("\nThe -nc option removes trailing colons (:)");
	printf("\nfrom labels (some CP/M era assemblers used this).");
	printf("\nfilename example: file.asm ");
	printf("\nOutput is file.prn (print file)");
	printf("\nand file.hex (compiled object file).");
	printf("\nSee source code or README for more info.\n\n");
	exit(-1);
}


/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ----------------------- main routine --------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int main(int argc, char *argv[]){

int i, n, ct;		/* used for token, ptoken */
int l, MEMFLAG = 0;

	/* these are big, need to be on the heap. The rest can stay on the stack */
	memory = malloc(65536*sizeof(int));
	label_address = malloc(MAXLABEL+1*(sizeof(int)));
	if (memory == NULL) MEMFLAG = 1;
	if (label_address == NULL) MEMFLAG = 1;	
	if (MEMFLAG) {
	    	fprintf(stderr,"Memory allocation error\n");
		exit(1);
	}

	for (n=0;n<65536;n++)
		memory[n] = NOP_CODE;	/* fill array with code for NOP */

	label_count = 0;		/* initialize */

	NCFLAG = 0;			/* command line option to remove colons */

	if (argc == 1)
		usage();		/* show usage and exit */

	if (argc == 2)
		strcpy(sourceFile,argv[1]);	/* get source file name */


	/* check command line options */
	if ((argc == 3) && (strcmp(argv[1],"-nc")==0)){
		NCFLAG = 1;		/* turn on no colon option */
		strcpy(sourceFile,argv[2]);	/* get source file name */
	}
	NCFLAG = 1;	// no reason not to have it as default	

	sourcefile = fopen(sourceFile,"r");
	if (sourcefile == NULL){	/* check for valid source file */
		printf("\nError - cannot open %s\n",sourceFile);
		exit(-1);
	}

	/* derive prnfile (print file) filename */
	for (n = strlen(sourceFile); n >= 0; n--){		/* find last . in filename */
		if (sourceFile[n] == '.')
			break;			/* position is n */
	}
	if (n == 0)
                strcpy(printFile,sourceFile);
        if (n != 0)
                strncpy(printFile,sourceFile,n);
        strcat(printFile,".prn");

	/* derive objfile (object code file) filename */
	for (n = strlen(sourceFile); n >= 0; n--){	/* find last . in filename */
		if (sourceFile[n] == '.')
			break;			/* position is n */
	}
	if (n == 0)
		strcpy(objFile,sourceFile);
	if (n != 0)
		strncpy(objFile,sourceFile,n);
	strcat(objFile,".com");


	/* ----------------------------------------------- */
	/* PASS 1 - read in the source file, create tokens */
	/* ----------------------------------------------- */

	ERRFLAG = 0;
	tokenCount = 0;

	while(feof(sourcefile) == 0){	/* read in each token till EOF */
		fscanf(sourcefile,"%s",singleWord);	/* get a single token */

		if (feof(sourcefile) != 0)	/* stop at end of file */
			break;

		if ((strcmp(singleWord,"ENDASM")==0) || (strcmp(singleWord,"endasm")==0))
			break;

		if (singleWord[0]==';'){	/* found a comment - skip till EOL */
			while((singleByte = fgetc(sourcefile)) != '\n');
			continue;
		}

		/* if singleWord is bound by double quotes, save as single token */
		if ((singleWord[0] == '"') && (singleWord[(strlen(singleWord)-1)] == '"')) {
			strcpy(token[tokenCount++],singleWord);
			continue;
		}

		/* if multi-words bound by double quotes, save whole thing */
		if (singleWord[0] == '\"') {
			strcpy(token[tokenCount],singleWord);
			i = strlen(singleWord);
			while((token[tokenCount][i++] = fgetc(sourcefile)) != '\"') { }
			if (strlen(token[tokenCount]) > MAXLINE){
				printf("\nError - Line too long. Missing double quote?\nNear %s",token[tokenCount]);
				ERRFLAG = 1;
			}	
			tokenCount++;
			continue;
		}


	
		strcpy(token[tokenCount++],singleWord);		/* save token */
	}
	fclose(sourcefile);
	tokenCount--;		/* set to correct number */

	if (tokenCount < 1){
		printf("\nWarning - File empty.\n");
		ERRFLAG = 1;
	}


	/* finished reading in all tokens - now parse them */


	ct = 0;	/* ptoken count */
	for (n=0; n<=tokenCount; n++){			/* cycle thru tokens found */
		if (isMbyteOpCode(token[n])){		/* current token is multibyte */
			strcpy(ptoken[ct],token[n]);	/* save first part of op code */
			strcat(ptoken[ct]," ");		/* space between parts */
			strcat(ptoken[ct++],token[++n]);	/* rest of op code */
			if (ptoken[ct-1][(strlen(ptoken[ct-1])-1)]==',')   /* remove trailing comma */
				ptoken[ct-1][(strlen(ptoken[ct-1])-1)] = '\0';
			/* if token isn't an opcode, bypass this routine */
			/* this allows a partial op code to be a label ie. dcr, mov etc */
			if (isOpcode(ptoken[ct-1]) != -1)
				continue;			/* next token */
			else {
				n--;
				ct--;
			}
		}
	
		/* check for a single space inside single quotes */	
		if ((token[n][0]=='\'') && (token[n+1][0]=='\'')){ 
			strcpy(ptoken[ct++],"' '");
			n+=1;
			continue;
		}


		/*
		* This is a command line option:
		* if the last character of a label 
		* is a colon (:), and the option is 
		* enabled, then remove the colon. 
		* Several CP/M era assemblers 
		* had a requirement to have a colon 
		* at the end of all labels. 
		* while this one doesn't, to be 
		* compatable we have this option 
		* in the command line (-nc no colon)
		*/

		if (NCFLAG){	/* remove trailing colon from label */
			if (token[n][(strlen(token[n])-1)] == ':'){
				token[n][(strlen(token[n])-1)] = '\0';
			}
		}


		/* nothing special from here on, save token in ptoken (processed token) */
		strcpy(ptoken[ct++],token[n]);		/* token now in ptoken */
	}


	if (ERRFLAG){
		printf("\nFound fatal error in PASS 1. Stopping.\n\n");
		exit(-1);
	}

	

/* -------------------------------------------------- */
/* ------------------ PASS 2 ------------------------ */
/* -------------------------------------------------- */

	/* read thru all tokens, set EQU's, create addresses, assign addr to labels */

	address = 0;		/* set initial address */
	n = 0;
	ERRFLAG = 0;

	while (n <= ct){		/* cycle thru all tokens */


		/* check for org or ORG */
		if ((strcmp(ptoken[n],"org")==0) || (strcmp(ptoken[n],"ORG")==0) || \
			(strcmp(ptoken[n],".org")==0) || (strcmp(ptoken[n],".ORG")==0)) {
			
		    	/* check next byte - reserved word? */
			if (isReserved(ptoken[n+1])){
				printf("\nError - token after %s is a reserved word: %s %s",ptoken[n],ptoken[n],ptoken[n+1]);
				ERRFLAG = 1;
			}
			/* check if next byte is a label */
			if (isLabel(ptoken[n+1]) != -1){ 	/* is label */
				address = isLabel(ptoken[n+1]);
				n += 2;
				continue;
			}
			/* else check if next token is a numeric address */
			if (getAddr(ptoken[n+1]) != -1){
				address = getAddr(ptoken[n+1]);	/* next byte is address */
				n += 2;
				continue;
			}

			/* only labels or numbers can follow an ORG statement */
	
			printf("\nError - PASS 2 - ORG directive missing address.");
			printf("\nToken number %d: %s %s",n,ptoken[n],ptoken[n+1]);
			ERRFLAG = 1;
			n += 1; 	/* point to next token */
			continue;
		}


		/* check for opcodes */
		if ((i = isOpcode(ptoken[n])) > 0){	/* if opcode, i returns size */
			address += i;			/* length of opcodes change addresses */
			if (i == 1)
				n++;
			if (i > 1){
				if (isReserved(ptoken[n+1])==1){
					printf("\nError - token after opcode %s is reserved word (%s)",\
					ptoken[n],ptoken[n+1]);
					ERRFLAG = 1;
				}		
				n += 2;
			}
			continue;
		}

		/*
		*  check for DS (Define Storage) directive 
		*  ptoken[n] is DS or ds  
		*  ptoken[n+1] is value of storage space 
		*/

		if ((strcmp(ptoken[n],"ds")==0) || (strcmp(ptoken[n],"DS")==0) || \
			(strcmp(ptoken[n],".ds")==0) || (strcmp(ptoken[n],".DS")==0)) {
			/* check for reserved words */
			if (isReserved(ptoken[n+1])){
				printf("\nError - token after DS is a reserved word: %s %s",ptoken[n],ptoken[n+1]);
				ERRFLAG = 1;
			}

			/* check for label here */
			if (isLabel(ptoken[n+1]) != -1){
				i = isLabel(ptoken[n+1]);
				address += i;
				n += 2;
				continue;
			}

			/* check for numeric constant */
			i = getAddr(ptoken[n+1]);
			if (i == -1){		/* error getting DS address */
				printf("\nError - token/label after DS directive non-numeric");
				printf("\nLine: %s %s",ptoken[n],ptoken[n+1]);
				ERRFLAG = 1;
			}
			if (i == 0){		/* probable error in DS address */
				printf("\nWarning - value of token/label after DS directive is 0");
				printf("\nLine: %s %s",ptoken[n],ptoken[n+1]);
			}
			address += i;		/* address incremented by DS byte 2 */
			n += 2;
			continue;
		}


		/*
		*  check for DB (define byte) directive
		*  ptoken[n] is .DB or DB or db or .db, [n+1] is data
		*/

		if ((strcmp(ptoken[n],"db")==0) || (strcmp(ptoken[n],"DB")==0) || \
			(strcmp(ptoken[n],".db")==0) || (strcmp(ptoken[n],".DB")==0)){
			/* check for reserved words */
			if (isReserved(ptoken[n+1])){
				ERRFLAG = 1;
				printf("\nError - token after %s is a reserved word: %s %s",ptoken[n],ptoken[n],ptoken[n+1]);
			}
			/* check for number after DB */
			if (getAddr(ptoken[n+1])>=0){
				i = getAddr(ptoken[n+1]);
				if (i < 256){
					address += 1;
					n += 2;
					continue;
				}
			}
			/* check for label after db directive */
			if ( (isLabel(ptoken[n+1])) != -1){
				i = isLabel(ptoken[n+1]);
				if ((i != -1) && (i < 256)){
					address += 1;
					n += 2;
					continue;
				}
			}
			/* else assume token after db is quoted string */		
			i = strlen(ptoken[n+1]);
			address += (i-2);	/* ignore leading/trailing quotes */
			n += 2;			
			continue;
		}


		/* check for equ/EQU labels */
		/* ptoken[n] should be a label, [n+1] the equ statement, and [n+2] the data */

		if ((strcmp(ptoken[n+1],"equ")==0)|| (strcmp(ptoken[n+1],"EQU")==0) || \
			(strcmp(ptoken[n+1],".equ")==0) || (strcmp(ptoken[n+1],".EQU")==0)){
			/* check for a reserved word */
			if (isReserved(ptoken[n+2])){
				ERRFLAG = 1;
				printf("\nError - token after EQU is a reserved word: %s %s %s"\
				,ptoken[n],ptoken[n+1],ptoken[n+2]);
			}
		
			/* check if label is a partial of an opcode (ie: mov, dcr, etc) */
			/* not fatal, but probably not wanted */
			if (isMbyteOpCode(ptoken[n]))
				printf("\nWarning - label %s is partial opcode. ",ptoken[n]);
	
			/* check if label already used */
			if (i = (isLabel(ptoken[n])) != -1){
				printf("\nError - token %s previously defined. Existing value is %d (%2.2XH)",\
				ptoken[n],label_address[i],label_address[i]);
				ERRFLAG = 1;
			}


			/* single quoted char ie '?' */
			if ((strlen(ptoken[n+2])==3) && \
				(ptoken[n+2][0]=='\'' && ptoken[n+2][2]=='\'')){
			        /* token after equ is single char */
				i = ptoken[n+2][1];
				strcpy(label[label_count],ptoken[n]);
				label_address[label_count++] = i;
				n += 3;
				continue;
			}	

			/* a number */
			i = getAddr(ptoken[n+2]);
			if (i != -1){
			        /* token after equ is number */
				strcpy(label[label_count],ptoken[n]);
				label_address[label_count++] = i;
				n += 3;
				continue;
			}

			/* a pre-defined label */
			i = isLabel(ptoken[n+2]);	/* i is label address */
			if (i != -1){	/* char after equ is pre-defined label */
				strcpy(label[label_count],ptoken[n]);
				label_address[label_count++] = i;
				n += 3;
				continue;
			}


			/* label after equ not a single char, label or number - show error */
			printf("\nError - In Pass 2: token after equ (%s %s %s) not valid. \
			\n",ptoken[n],ptoken[n+1],ptoken[n+2]);
			exit(-1);

		}


		/* everything else should be a label used for addressing */

		/* but first check for reserved chars in a label */
		/* not allowed: {,\} */

		if (isReserved(ptoken[n])){
			printf("\nError - token %s is a reserved word.",ptoken[n]);
			ERRFLAG = 1;
		}

		FLAG = 0;
		for (idx=0;idx<=strlen(ptoken[n]);idx++){
			if (ptoken[n][idx] == ',')
				FLAG = 1;
			if (ptoken[n][idx] == '\\')
				FLAG = 1; 
		}
		if (FLAG){
			printf("\nError - reserved char in label { %s } not allowed.",ptoken[n]);
			printf("\nExiting.\n\n");
			exit(-1);
		}

		/* next, check for reserved words as a label */

		if ((strcmp(ptoken[n],"equ")==0) || (strcmp(ptoken[n],"EQU")==0)){
			printf("\nError - reserved word %s used as a label.",ptoken[n]);
			printf("\nExiting.\n\n");
			ERRFLAG = 1;
		}

		/* it's bad form to use a number as a label - check for these */

		if (getAddr(ptoken[n]) != -1){
			printf("\nWarning - label %s is numeric. This may not be what you wanted.",\
			ptoken[n]);
		}

		/* did we already use this label? */
		if (isLabel(ptoken[n])!=-1){
			printf("\nError - duplicate label %s",ptoken[n]);
			ERRFLAG = 1;
		} 


		strcpy(label[label_count],ptoken[n]);		/* save label in array */
		label_address[label_count++] = address;		/* save label address */
		n++;						/* point to next token */
		continue;

		/* ------------------------ */

		n++;	/* should never come here - everything should have been checked */
		printf("\nFell Thru - error.\n");
		printf("\nYou shouldn't be here. Please report this.");
		continue;
	}


	/* If ERRFLAG is set, we really can't go on. Stop here. */
	if (ERRFLAG){
		printf("\nToo many errors to continue. Stopping.\n\n");
		exit(-1);
	}
	

	
	/* label_count is one more than actual - fix */
	label_count--;	



	/* set max address */
	maxAddress = address;


	/* ------------------------------------------------- */
	/* -------------------- PASS 3 --------------------- */
	/* ------------------------------------------------- */

	address = 0;
	n = 0;
	ERRFLAG = 0;

	prnfile = fopen(printFile,"w");
	if (prnfile == NULL){
		printf("\nError - cannot create .prn file");
		exit(-1);
	}

	fprintf(prnfile,"\nReading from source file %s\n",sourceFile);
	fprintf(prnfile,"\nNumber of tokens found in file is %d\n",ct-1);

	while ( n < ct ){	/* cycle thru all tokens */	


                /* check for org or ORG */
                if ((strcmp(ptoken[n],"org")==0) || (strcmp(ptoken[n],"ORG")==0) || \
			(strcmp(ptoken[n],".org")==0) || (strcmp(ptoken[n],".ORG")==0)){
			if (isLabel(ptoken[n+1]) != -1){		/* next token is a label */
				address = isLabel(ptoken[n+1]);
				fprintf(prnfile,"\n\n%s  (%s)\t%4.4X\n",ptoken[n],ptoken[n+1],address);
				n += 2;
				continue;
			}

			/* else check for numeric address in next token */
			if (getAddr(ptoken[n+1]) != -1){
                        	address = getAddr(ptoken[n+1]); /* next byte is address */
                        	/* address now changed */
				fprintf(prnfile,"\n\n%s\t%4.4X\n",ptoken[n],address);
                        	n += 2; /* skip past address, go to next real token */
                        	continue;
			}
			/* next token not valid? */
			printf("\nPass 3 - token after org not valid.");
			printf("\nLine: %s %s",ptoken[n],ptoken[n+1]);
			ERRFLAG = 1;
                }


		/* check if token is an opcode */
		i = 0;		/* set initial value */
		i = isOpcode(ptoken[n]);	/* returns -1,1,2,3 */

		if (i == 1){	/* single byte opcode */
			codenum = getOpCode(ptoken[n]);
			fprintf(prnfile,"\n%4.4X %2.2X\t\t%s",\
			address,codenum,ptoken[n]);
			memory[address++] = codenum;
			n++;		/* point to next token */
			continue;
		}

		/* check 2nd/3rd byte of multi-byte opcode for reserved word */
		if (((i == 2) || (i == 3)) && (isReserved(ptoken[n+1]))){
			ERRFLAG = 1;
			printf("\nError - 2nd or 3rd byte of multi-byte opcode is a reserved word.");
			printf("\n%s %s",ptoken[n],ptoken[n+1]);
		}


		if ((i == 2) && (isLabel(ptoken[n+1])!=-1 )){  /* 2 byte opcode w/label */
			codenum = getOpCode(ptoken[n]);
			idx = isLabel(ptoken[n+1]);
			if (idx > 255){
				printf("\nError - 2nd byte of 2 byte opcode is > 255.");
				printf("\nRange is 0-255. Value is %d.",idx);
				printf("\nLine is %s %s",ptoken[n],ptoken[n+1]);
				ERRFLAG = 1;	
			}
			fprintf(prnfile,"\n%4.4X %2.2X %2.2X\t%s %s", \
			address,codenum,idx,ptoken[n],ptoken[n+1]);
			memory[address++] = codenum;	/* save opcode */
			memory[address++] = idx;	
			n += 2;
			continue;
		}		

		/* token after opcode is single quoted char */
		if ((i==2) && (ptoken[n+1][0]=='\'') && (ptoken[n+1][2]=='\'')){
			if (strlen(ptoken[n+1]) != 3){		/* error - should be length 3 ('x') */
				printf("\nError - token wrong length %s %s",ptoken[n],ptoken[n+1]);
				ERRFLAG = 1;
			}
			codenum = getOpCode(ptoken[n]);
			fprintf(prnfile,"\n%4.4X %2.2X %2.2X\t%s %s",\
			address,codenum,(int)ptoken[n+1][1],ptoken[n],ptoken[n+1]);
			memory[address++] = codenum;
			memory[address++] = (char)ptoken[n+1][1];
			n += 2;
			continue;
		}

		/* token after opcode is number */
		if ((i==2) && (getAddr(ptoken[n+1])>=0)){
			codenum = getOpCode(ptoken[n]);
			idx = getAddr(ptoken[n+1]);
			if (idx > 255){
                                printf("\nError - 2nd byte of 2 byte opcode is > 255.");
                                printf("\nRange is 0-255. Value is %d.",idx);
                                printf("\nLine is %s %s",ptoken[n],ptoken[n+1]);
                               	ERRFLAG = 1; 
                        }
			fprintf(prnfile,"\n%4.4X %2.2X %2.2X\t%s %s",\
			address,codenum,idx,ptoken[n],ptoken[n+1]);
			memory[address++] = codenum;
			memory[address++] = idx;
			n += 2;
			continue;
		}

		/* exausted all 2 byte opcode possibilities - show 2 byte failure */
		if (i == 2){
			printf("\nError - 2nd byte of 2 byte opcode invalid.");
			printf("\n%s %s -  no match found for %s",ptoken[n],ptoken[n+1],ptoken[n+1]);
			ERRFLAG = 1;
		}


		/* now check for 3 byte opcodes */

                /* 3 byte opcode w/label*/
                if (  (i == 3) && ( (isLabel(ptoken[n+1])!=-1) )  ){
                        codenum = getOpCode(ptoken[n]);
                        addr = isLabel(ptoken[n+1]);
			if ((addr > 65535) || (addr < 0)){
				printf("\nError - value of label %s after 3 byte opcode < 0 or > 65535",ptoken[n+1]);
				printf("\nValid range is 0-65535. ");
				printf("\nValue found is %d - line is %s %s %s",ptoken[n],ptoken[n+1],ptoken[n+2]);
				ERRFLAG = 1;
			}
                        addr2 = (int)addr/256;
                        addr1 = addr - (addr2 * 256);
                        fprintf(prnfile,"\n%4.4X %2.2X %2.2X %2.2X\t%s %s (%2.2X%2.2XH)",\
                        address,codenum,addr1,addr2,ptoken[n],ptoken[n+1],addr2,addr1);
                        memory[address++] = codenum;
                        memory[address++] = addr1;
                        memory[address++] = addr2;
                        n += 2;
                        continue;
                }


		/* 3 byte opcode w/number */
		if ((i == 3) && (getAddr(ptoken[n+1])>=0)) {
			codenum = getOpCode(ptoken[n]);
			addr = getAddr(ptoken[n+1]);
			if ((addr > 65535) || (addr < 0)){
                                printf("\nError - value after 3 byte opcode < 0 or > 65535");
                                printf("\nValid range is 0-65535. ");
                                printf("\nValue found is %d - line is %s %s %s",ptoken[n],ptoken[n+1],ptoken[n+2]);
				ERRFLAG = 1;
                        }
			addr2 = (int)addr/256;
			addr1 = addr - (addr2 * 256);
			fprintf(prnfile,"\n%4.4X %2.2X %2.2X %2.2X\t%s %s",\
			address,codenum,addr1,addr2,ptoken[n],ptoken[n+1]);
			memory[address++] = codenum;
			memory[address++] = addr1;
			memory[address++] = addr2;
			n += 2;
			continue;
		}

		
		/* exausted all 3 byte options - show error */
		if (i == 3){
			printf("\nError - Pass 3:Address after 3 byte opcode invalid.");
                        printf("\n%s %s -  no match found for %s",ptoken[n],ptoken[n+1],ptoken[n+1]);
			ERRFLAG = 1;
                }



		if ((strcmp(ptoken[n+1],"equ")==0) || (strcmp(ptoken[n+1],"EQU")==0) || \
			(strcmp(ptoken[n+1],".equ")==0) || (strcmp(ptoken[n+1],".EQU")==0)){
			/* line is equate - ignore */
			fprintf(prnfile,"\n%s\t%s\t%s",ptoken[n],ptoken[n+1],ptoken[n+2]);
			n += 3;
			continue;
		}
	
		
		/* check for DS/ds directive */
		if ((strcmp(ptoken[n],"DS")==0) || (strcmp(ptoken[n],"ds")==0) || \
			(strcmp(ptoken[n],".DS")==0) || (strcmp(ptoken[n],".ds")==0)){
			idx = getAddr(ptoken[n+1]);
			if (idx == -1){		/* error */
				printf("\nError - value after DS not a number");
				// printf("\nLine: %s %s %s",ptoken[n],ptoken[n+1],ptoken[n+2]);
				printf("\nLine: %s %s",ptoken[n],ptoken[n+1]);
				ERRFLAG = 1;
			}
			fprintf(prnfile,"\n%4.4X\t%s %s",address,ptoken[n],ptoken[n+1]);
			address += idx;
			n += 2;
			continue;
		}

		/* check for DB/db directive */
		if ((strcmp(ptoken[n],"db")==0) || (strcmp(ptoken[n],"DB")==0) || \
			(strcmp(ptoken[n],".db")==0) || (strcmp(ptoken[n],".DB")==0)){
			/* check for number after DB */
			i = getAddr(ptoken[n+1]);
			if (i >= 0){
				fprintf(prnfile,"\n%4.4X  %2.2X\t%s  %s",address,i,ptoken[n],ptoken[n+1]);
				memory[address++] = i;
				n += 2;
				continue;
			}
			/* check for label after db directive */
                        if ((isLabel(ptoken[n+1]) != -1)){
                                i = isLabel(ptoken[n+1]);
                                if ((i != -1) && (i < 256)){
					fprintf(prnfile,"\n%4.4X  %2.2X\t%s  %s",address,i,ptoken[n],ptoken[n+1]);
					memory[address++] = i;	/* save byte */
                                        n += 2;
                                        continue;
                                }
                        }

			/* use quoted text */
			idx = strlen(ptoken[n+1]);	/* get string length */
			fprintf(prnfile,"\n%4.4X\t\t%s  \"",address,ptoken[n]);
			for (i=1; i<idx-1; i++){
				fprintf(prnfile,"%c",ptoken[n+1][i]);
				memory[address++] = (int)ptoken[n+1][i];
			}
			fprintf(prnfile,"\"");
			n += 2;
			continue;
		}

	
		/* check for label - print address, continue */
		if (isLabel(ptoken[n])>=0){
			fprintf(prnfile,"\n\n%4.4X\t%s",address,ptoken[n]);
			n++;
			continue;
		}

	
		n++;
		continue;

	}

	/* check if last address is overflowed */
	if (address > 65535){
		printf("\nError - address overflow. Address is %d (%4.4XH)",address,address);
	}

	/* show labels used */
	fprintf(prnfile,"\n\nLabels Used: %d\n",label_count);
	for (n=0; n<label_count; n++){
		fprintf(prnfile,"\n%s     \t",label[n]);
		fprintf(prnfile,"%4.4XH",label_address[n]);
	}

	/* finished with PASS 3 */

	/* error check - if no assembled code, maxAddress will be 0. The next line will show */
	/* -1 as the maxAddress which is wrong. */
	if (maxAddress == 0)
		maxAddress = 1;

	fprintf(prnfile,"\n");
	fprintf(prnfile,"\nLast address used is %4.4XH (%d decimal)\n",maxAddress-1,maxAddress-1);
	fprintf(prnfile,"\nEnd of assembly.\n\n");


	/* create object file and populate with memory[] data */

	if (ERRFLAG){	/* some fatal error found - do not create object file */
		printf("\nFound fatal errors. No object file created.\n\n");
		exit(-1);
	}

	objfile = fopen(objFile,"w");
	if (objfile == NULL){
		printf("\nError - cannot create object file.");
		printf("\nExiting.\n");
		exit(-1);
	}

	for (n=0; n<=maxAddress; n++)
		fprintf(objfile,"%c",memory[n]);
	fclose(objfile);
	fclose(prnfile);


	printf("\nAssembly Complete.\n");
	exit(0);
}
	
