Using the asm80 assembler:

--------------------------

asm80 [source.asm]
Will assmeble the 8080 source code and create 2 files:
source.prn	Suitable for printing
source.com	Typical CP/M object code

--------------------------

Nmenomics are standard Intel (from the MCS-85 book)
ie: mov a,b   jmp   rz   nop   lxi sp   lxi h

--------------------------

Numbers can be: 
decimal (100, 12, 65535) 
hex (0x1, 0x12, 0x123, 0x1234)
hex (01h, 12h, 1234h)

---------------------------

Labels can be any word/number combination as long
as they're not reserved words.
ie. start, loop123, FileStart_1

Also, labels can have a colon (:) as the last character
ie: start:

This allows older .asm files to be assembled.

----------------------------

Reserved words are:
db, DB, .db, .DB
ds, .ds, DS, .DS
dw, DW, .dw, .DW
org, ORG, .org, .ORG
equ, .equ, EQU, .EQU

endasm, ENDASM will stop assembly.

------------------------

Define Byte: the db keyword tells the assembler that the following 
"quoted string", numeric value (1234, 0x1) or equate (CR, LF) 
is to be used to fill memory. Putting .db after a label is optional.
ie. 
message db	"This is a line of Text"
	.db	CR
	.DB	LF

Note the next line has no label, but the first character is 
a .db or varient.

The form:
message db	"This is a line of Text" .db CR .db LF .db EOL

can also be used. There must be another .db (or varient) after the
double qoutes at the end of the first line.


The db keyword can also be used to insert numeric values 
in the code:

start	mvi a, 0x20
	.db 0x3e  .db 0x20

is equivalent.

-----------------------

Define Space: the ds keyword tells the assembler to assign nn 0's
after the ds keyword.
ie.
.DS 0x20
STACK

Will assign 0x20 0's after the .DS
The address of the label STACK will start 0x20 bytes after the .DS 0x20
(Note that 0x20 is 32 decimal).

	
-----------------------

Comments begin with a semi-colon (;) and end at a newline (\n)
Comments can be anywhere in a line. Assembly for the line
stops at a semi-colon and starts at the beginning of the next line.

-------------------------

Here is an example program (note different variations of the same keyword):

START	.equ	0
CR	equ	13
LF	.EQU	0xa
EOL	.equ	0xfe

org	START			
; set start address (note org is in label column)

	jmp	begin


	org	0x100		
; set another start address (now org is in the next tabbed address)
; The assembler doesn't care where the org goes

begin	mvi a	0x1		; [TAB] between a and 0x1, space between mvi and a
	mvi a	0x23
	mvi b	100
	lxi h	0x1
	lxi b	0x21
	lxi d	0x1234
	.db	0x3e	.db	0xff
	lxi sp	STACK
	hlt

message: 	db	"This is a line of Text" .db CR .db LF DB EOL
message2	db	"Next line"
		.db	CR
		DB	LF
		.DB	EOL


.DS	0x20
STACK	


-----------------------

