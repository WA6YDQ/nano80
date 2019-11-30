; diag.asm	test 8080 opcodes

; nothing special about this - just a brute force 
; op code tester for the 8080.

; This test runs TESTCOUNT times, printing a dot (.) for each pass.
; Any fails will stop the test. At the end of a successful run,
; a success message will be displayed and the program will stop.

; (C) k theis 2019 MIT license applies


; program defines - change as needed for your use
STACK	equ	7f00h
STACK2	equ	6543h
SOUT	equ	1
EOL	equ	feh
CR	equ	0dh
LF	equ	0ah
TESTCOUNT equ	40

org	0


start	lxi sp	STACK
	mvi a	TESTCOUNT
	sta	CNTR
	jmp	diag

error	; show error msg and halt
	lxi h	errmsg
	call	print
	lxi h	failedcode
	call	print
	lda	ERRCODE		; retrive the number of the failed routine
	out	SOUT		; show code
	mvi a	CR		; and send a CR/LF
	out	SOUT
	mvi a	LF
	out	SOUT
	hlt

ok	; show pass msg and halt
	lxi h	okmsg
	call	print
ok1	jmp	ok1

print	; send message to serial port
	mov a,m
	cpi EOL
	rz
	out	SOUT
	inx h
	jmp	print


okmsg	db	CR
	db	LF
	db	"All tests Passed" 
	db	CR
	db	LF
	db	EOL

errmsg	db	"Tests Failed"
	db	CR
	db	LF 
	db	EOL

failedcode	db	"Routine that failed: "
		db	EOL


diag	; start of tests

;	----- Data Transfer Group -----
		
	mvi a	'A'
	sta	ERRCODE		; ERROR CODE A
	
	mvi a	55h	; test mov, cmp		NOTE: No flags affected in this group
	mov b,a
	mov c,b
	mov d,c
	mov e,d
	mov l,e
	mov h,l
	cmp b
	jnz	error
	cmp c
	jnz	error
	cmp d
	jnz	error
	cmp e
	jnz	error
	cmp h
	jnz	error
	cmp l	
	jnz	error
	lxi h	TEMPH
	mov m,a
	cmp m
	jnz	error
	

	mvi a	'B'
	sta	ERRCODE		; ERROR CODE B
	;
	mvi a	55h		; mvi 
	mvi b	55h
	mvi c	55h
	mvi d	55h
	mvi e	55h
	mvi h	55h
	mvi l	55h
	cmp b
	jnz	error
	cmp c
	jnz	error
	cmp d
	jnz	error
	cmp e
	jnz	error
	cmp h
	jnz	error
	cmp l
	jnz	error
	lxi h	TEMPH
	mvi m	55h
	cmp m
	jnz	error
	mov m,a
	cmp m
	jnz	error

	mvi a	'C'
	sta	ERRCODE		; ERROR CODE C 
	
	mvi a	aah		; memory 
	mov b,a
	mov m,b
	cmp m
	jnz	error

	inr a
	mov c,a
	mov m,c
	cmp m
	jnz	error

	inr a
	mov d,a
	mov m,d
	cmp m
	jnz	error

	inr a
	mov e,a
	mov m,e
	cmp m
	jnz	error

	mvi a	55h
	mov m,a
	mov b,m
	cmp b
	jnz	error

	mov c,m
	cmp c
	jnz	error
	
	mov d,m
	cmp d
	jnz	error

	mov e,m
	cmp e
	jnz	error

	mvi a	'D'
	sta	ERRCODE		; ERROR CODE D

	mvi a	55h		; lhld
	sta	TMPL
	mvi a	aah
	sta	TMPH
	lhld	TMPL
	cmp h
	jnz	error
	mvi a	55h
	cmp l
	jnz	error

	mvi a	'E'
	sta	ERRCODE		; ERROR CODE E

	xra a			; lhld, shld
	mov h,a
	mov l,a
	shld	TMPL
	mvi h	1
	mvi l	2
	lhld	TMPL
	cmp h
	jnz	error
	cmp h
	jnz	error
	cmp l
	jnz	error

	mvi a	'F'
	sta	ERRCODE		; ERROR CODE F

	lxi h	TEMPH		; ldax, stax, xchg
	mvi m	55h
	xchg
	ldax d
	cpi	55h
	jnz	error
	lxi h	TEMPH
	mvi m	aah
	mov b,h
	mov c,l
	mov d,h
	mov e,l
	ldax b
	cpi	aah
	jnz	error
	ldax d
	cpi	aah
	jnz	error


	
; ----- Arithmetic Group -----

	mvi a	'G'
	sta	ERRCODE		; ERROR CODE G

	mvi a 	55h
	mvi b	55h
	mvi c	55h
	mvi d	55h
	mvi e	55h
	mvi h	55h
	mvi l	55h	
;
	add b		; test add
	jp	error	; Plus: should be minus
	jc	error	; Carry : clear
	jz	error	; Zero: clear
	jpo	error	; 55h, parity is even (P=1)
	cpi 0aah	
	jnz	error	; Z: set

	mvi a	0	; test positive
	add b
	jm	error
	jc	error
	jz	error

	mvi a	abh	; overflow test
	add b		; should be 00 in A w/CY set
	jpe	error	; parity odd 
	jnc	error
	jnz	error
	jm	error	
	; flags should work, no need to test flags again for add

	mvi a	55h
	add c
	cpi aah
	jnz	error

	mvi a	55h
	add d
	cpi aah
	jnz	error

	mvi a	55h
	add e
	cpi aah
	jnz	error

	mvi a	55h
	add h
	cpi aah
	jnz	error

	mvi a	55h
	add l
	cpi aah
	jnz	error

	; test add m
	lxi h	TEMPH
	mvi m	55h
	mvi a	55h
	add m
	cpi	aah
	jnz	error

	mvi a	'H'
	sta	ERRCODE		; ERROR CODE H

	; test adi
	mvi a	55h
	adi	55h
	jc	error
	jz	error
	jp	error
	jpo	error

	cpi	aah
	jnz	error

	mvi a	'I'
	sta	ERRCODE		; ERROR CODE I

	; test adc
	mvi a	55h
	mvi b	55h
	stc
	adc b
	jc	error
	jp	error
	jz	error
	jpe	error	

	cpi	abh
	jnz	error

	mvi a	55h
	mvi c	55h
	stc
	adc c
	cpi	abh
	jnz	error

	mvi a	55h
	mvi d	55h
	stc
	adc d
	cpi	abh
	jnz	error

	mvi a	55h
	mvi e	55h
	stc	
	adc e
	cpi	abh
	jnz	error

	mvi a	55h
	mvi h	55h
	stc
	adc h
	cpi	abh
	jnz	error

	mvi a	55h
	mvi l	55h
	stc
	adc l
	cpi	abh
	jnz	error	

	mvi a	'J'
	sta	ERRCODE		; ERROR CODE J

	; test adc m
	lxi h	TEMPH
	mvi m, 	55h
	mvi a	55h
	stc
	adc m
	jc	error	; CY=0
	jp	error	; S=1
	jpe	error	; P=0
	jz	error	; Z=0

	cpi	abh
	jnz	error

	mvi a	'K'
	sta	ERRCODE		; ERROR CODE K

	; test aci 
	mvi a	55h
	stc
	aci	55h
	jz	error	; Z=0
	jp	error	; S=1
	jpe	error	; P=0
	jc	error	; CY=0

	cpi	abh
	jnz	error

	mvi a	'L'
	sta	ERRCODE		; ERROR CODE L

	; test sub
	mvi a	55h
	mvi b	1
	sub b
	jc	error	; CY=0
	jz	error	; Z=0
	jm	error	; S=0
	jpe	error	; P=0

	cpi	54h
	jnz	error

	mov c,b
	sub c
	jc	error
	jz	error
	jm	error
	jpo	error

	cpi	53h
	jnz	error

	mov d,b
	sub d
	cpi	52h
	jnz	error

	mov e,b
	sub e
	cpi	51h
	jnz	error
	
	mov h,b
	sub h
	cpi	50h
	jnz	error

	mov l,b
	sub l
	cpi	4fh
	jnz	error

	; test sub w/underflow
	xra a
	mvi b 	1
	sub	b	; A=ffh
	jnc	error	; CY=1
	jp	error	; S=1
	jpo	error	; P=1
	jz	error	; Z=0
	cpi	ffh
	jnz	error

	mvi a	'M'
	sta	ERRCODE		; ERROR CODE M

	; test sub m
	lxi h	TEMPH
	mvi m	1
	mvi a	55h
	sub m
	jz	error	; Z=0
	jc	error	; CY=0
	jm	error	; S=0
	jpe	error	; P=0

	cpi	54h
	jnz	error

	mvi a	'N'
	sta	ERRCODE		; ERROR CODE N

	; test sui
	mvi a	55h
	sui	1
	jz	error	; Z=0
	jc	error	; CY=0
	jm	error	; S=0
	jpe	error	; P=0
	cpi	54h
	jnz	error

	; sui underflow
	xra a
	sui 	1
	jnc	error	; CY=1
	jp	error	; S=1
	jpo	error	; P=1
	cpi	ffh
	jnz	error	; A=ffh

	mvi a	'O'
	sta	ERRCODE		; ERROR CODE O

	; test sbb
	mvi a	55h
	mvi b	1
	mov c,b
	mov d,b
	mov e,b
	mov h,b
	mov l,b
	stc
	sbb b
	jz	error	; Z=0
	jc	error	; CY=0
	jm	error	; S=0
	jpo	error	; P=1

	cpi	53h
	jnz	error

	stc
	sbb c
	cpi	51h
	jnz	error

	stc
	sbb d
	cpi	4fh
	jnz	error

	stc
	sbb e
	cpi	4dh
	jnz	error

	stc
	sbb h
	cpi	4bh
	jnz	error

	stc
	sbb l
	cpi	49h
	jnz	error	

	mvi a	'P'
	sta	ERRCODE		; ERROR CODE P

	; test sbb m
	lxi h	TEMPH
	mvi m	1
	mvi a	55h
	stc
	sbb m
	jz	error	; Z=0
	jc	error	; CY=0
	jm	error	; S=0
	jpo	error	; P=1

	cpi	53h
	jnz	error

	mvi a	'Q'
	sta	ERRCODE		; ERROR CODE Q
	
	; test sbi 
	mvi a	55h
	stc
	sbi	1
	jz	error	; Z=0
	jc	error	; CY=0
	jm	error	; S=0
	jpo	error	; P=1

	cpi	53h
	jnz	error

	mvi a	'R'
	sta	ERRCODE		; ERROR CODE R

	; test for underflow
	xra a		; A=0
	sbi	1	; A=ffh
	jnc	error	; CY=1
	jpo	error	; P=1
	jz	error	; Z=0
	jp	error	; S=1


	; test inr

	; basic tests (no flags)
	mvi a	1
	mov b,a
	mov c,b
	mov d,c
	mov e,d
	mov h,e
	mov l,h
	inr a
	cpi	2
	jnz	error
	inr b
	cmp b
	jnz	error
	inr c
	cmp c
	jnz	error
	inr d
	cmp d
	jnz	error
	inr e
	cmp e
	jnz	error
	inr h
	cmp h
	jnz	error
	inr l
	cmp l
	jnz	error

	; test flags
	xra a
	inr a		; A=1
	jz	error	; Z=0
	jm	error	; S=0
	jpe	error	; P=0
	cpi	1
	jnz	error

	mvi a	ffh
	inr a		; A=0
	jnz	error	; Z=1
	jpe	error	; P=0 (*)
	jm	error	; S=0
	cpi	0
	jnz	error
	


	mvi a	'S'
	sta	ERRCODE		; ERROR CODE S

	; test inr M
	mvi a	2
	lxi h	TEMPH
	mvi m	1
	inr m
	cmp m
	jnz	error

	mvi a	'T'
	sta	ERRCODE		; ERROR CODE T

	; test dcr
	mvi a	1
	dcr a
	jnz	error
	mvi b	1
	dcr b
	jnz	error
	mvi c	1
	dcr c
	jnz	error
	mvi d	1
	dcr d
	jnz	error
	mvi e	1
	dcr e
	jnz	error
	mvi h	1
	dcr h
	jnz	error
	mvi l	1
	dcr l
	jnz	error

	; test flags
	xra a
	dcr a	; A=ffh
	jz	error	; Z=0
	jp	error	; S=1
	jpo	error	; P=1
	cpi	ffh
	jnz	error

	
	mvi a	'U'
	sta	ERRCODE		; ERROR CODE U

	; test dcr M
	lxi h	TEMPH
	mvi m	1
	dcr m
	jnz	error	; Z=0
	jpe	error	; P=0
	jm	error	; S=0

	mvi a	'V'
	sta	ERRCODE		; ERROR CODE V

	; test INX rp
	lxi b	ffffh
	inx b
	xra a
	cmp b
	jnz	error
	cmp c
	jnz	error

	lxi d	ffffh
	inx d
	xra a
	cmp d
	jnz	error
	cmp e
	jnz	error

	lxi h	ffffh
	inx h
	xra a
	cmp h
	jnz	error
	cmp l
	jnz	error

	mvi a	'W'
	sta	ERRCODE

	; test DCX rp
	lxi b	0000h
	dcx b
	mov a,b
	cpi	ffh
	jnz	error
	mov a,c
	cpi	ffh
	jnz	error

	lxi d	0000h
	dcx d
	mov a,d
	cpi	ffh
	jnz	error
	mov a,e
	cpi	ffh
	jnz	error

	lxi h	0000h
	dcx h
	mov a,h
	cpi	ffh
	jnz	error
	mov a,l
	cpi	ffh
	jnz	error

	mvi a	'X'
	sta	ERRCODE		; ERROR CODE X
	
	; test DAD rp
	lxi h	5555h
	lxi b	5555h
	dad b
	mov a,h
	cpi	aah
	jnz	error
	mov a,l
	cpi	aah
	jnz	error

	lxi d	1111h
	dad d
	mov a,h
	cpi	bbh
	jnz	error
	mov a,l
	cpi	bbh
	jnz	error	

	lxi h	0101h
	dad h
	mov a,h
	cpi	02h
	jnz	error
	mov a,l
	cpi	02h
	jnz	error

	mvi a	'Y'
	sta	ERRCODE		; ERROR CODE Y

	; test daa
	; I don't understands DAA enough to test it yet

	mvi a	'Z'
	sta	ERRCODE		; ERROR CODE Z

	; test ANA r
	mvi a	a5h
	push psw
	mvi b	6ch
	mov c,b
	mov d,b
	mov e,b
	mov h,b
	mov l,b
	ana b
	jc	error		; CY cleared after ana
	cpi	24h
	jnz	error
		
	pop	psw
	push 	psw
	ana c
	jc	error
	cpi	24h
	jnz	error

	pop	psw
	push	psw
	ana d
	jc	error
	cpi	24h
	jnz	error

	pop 	psw
	push 	psw
	ana e
	jc	error
	cpi	24h
	jnz	error

	pop	psw
	ana h
	jc	error
	cpi	24h
	jnz	error

	; test flags
	xra a
	jnz	error
	
	mvi a	f0h
	mvi b	0fh
	ana b
	jnz	error	; Z=1

	mvi a	f0h
	mvi b	ffh
	ana b
	jp	error	; S=1
	jpo	error	; P=1
	jz	error	; Z=0
	
	mvi a	ffh
	mvi b	ffh
	ana b
	jp	error	; S=1
	jpo	error	; P=1
	cpi	ffh
	jnz	error
	

	mvi a	05h
	mvi c	05h
	ana c
	jz	error	; Z=0
	jpo	error	; P=1
	jc	error	; CY=0 (always)
	jm	error	; S=0

	mvi a	'a'
	sta	ERRCODE		; ERROR CODE a

	; test ANA m
	lxi h	TEMPH
	mvi a 	a5h
	mvi m	6ch
	ana m
	jc	error
	jz	error
	jm	error
	jpo	error
	cpi 	24h
	jnz	error

	mvi a	'b'
	sta	ERRCODE		; ERROR CODE b
	
	; test ANI
	mvi a	a5h
	ani	6ch
	jc	error
	jz	error
	jpo	error
	jm	error
	cpi	24h
	jnz	error

	mvi a	'c'
	sta	ERRCODE		;; ERROR CODE c

	; test xra
	mvi a	cch
	push	psw
	mvi b	66h
	mov c,b
	mov d,b
	mov e,b
	mov h,b
	mov l,b
	xra a
	jc	error		; tested xra earlier
	cpi	0
	jnz	error
	pop	psw
	push	psw
	xra 	b
	jc	error
	cpi	aah
	jnz	error
	pop 	psw
	push	psw
	xra	c
	jc	error
	cpi	aah
	jnz	error
	pop	psw
	push	psw
	xra	c
	jc	error
	cpi	aah
	jnz	error
	pop	psw
	push	psw
	xra	d
	jc	error
	cpi	aah
	jnz	error
	pop	psw
	push 	psw
	xra	e
	jc	error
	cpi	aah
	jnz	error
	pop	psw
	push	psw
	xra	h
	jc	error
	cpi	aah
	jnz	error
	pop	psw
	xra	l
	jc	error
	cpi	aah
	jnz	error

	mvi a	'd'	
	sta	ERRCODE		; ERROR CODE d

	; test xra m
	lxi h	TEMPH
	mvi m	66h
	mvi a	cch
	xra m
	jp	error
	jpo	error
	jz	error
	jc	error
	cpi	aah
	jnz	error

	mvi a	'e'
	sta	ERRCODE		; ERROR CODE e

	; test XRI
	mvi a	cch
	xri	66h
	jc	error
	jz	error
	jpo	error
	jp	error
	cpi	aah
	jnz	error

	mvi a	'f'
	sta	ERRCODE		; ERROR CODE f

	; test ora
	mvi a	f0h
	push psw
	mvi b	0fh
	mov c,b
	mov d,b
	mov e,b
	mov h,b
	mov l,b
	ora b
	jc 	error	; CY=0
	jz	error	; Z=0
	jpo	error	; P=1
	jp	error	; S=1
	inr a	
	jnz	error
	pop	psw
	push	psw
	ora c
	jc	error
	inr a
	jnz	error
	pop	psw
	push	psw
	ora d
	jc 	error
	inr a
	jnz	error
	pop	psw
	push	psw
	ora e
	jc	error
	inr a
	jnz	error
	pop	psw
	push	psw
	ora h
	jc	error
	inr a
	jnz	error
	pop	psw
	ora	l
	jc	error
	cpi	ffh
	jnz	error

	mvi a	'g'
	sta	ERRCODE		; ERROR CODE g

	; test ORA m
	mvi a	f0h
	lxi h	TEMPH
	mvi m	0fh
	ora m
	jc	error
	jz	error
	jp	error
	jpo	error
	cpi	ffh
	jnz	error

	mvi a	'h'
	sta	ERRCODE		; ERROR CODE h

	; test ORI
	mvi a	f0h
	ori	0fh
	jc	error
	jz	error
	jpo	error
	jp 	error
	cpi	ffh
	jnz	error

	mvi a	'i'
	sta	ERRCODE		; ERROR CODE i

	; test cmp
	mvi a	08h
	mvi b	07h
	cmp b
	jz	error
	jc	error
	mvi b	08h
	cmp b
	jnz	error
	jc	error
	mvi b	09h
	cmp b
	jz	error
	jnc	error	; at this point we've tested the functionality
			; of the cmp opcodes. The rest is just gravy
	mvi a	'j'
	sta	ERRCODE		; ERROR CODE j

	; test cmp m
	mvi a 	08h
	lxi h	TEMPH
	mvi m	07h
	cmp m
	jz 	error
	jc	error
	inr m
	cmp m
	jnz	error
	jc 	error
	inr m
	cmp m
	jz	error
	jnc	error


	mvi a	'k'
	sta	ERRCODE		; ERROR CODE k

	; test cpi
	mvi a	08h
	cpi	07h
	jz	error
	jc	error
	cpi	08h
	jnz	error
	jc	error
	cpi	09h
	jz	error
	jnc	error

	mvi a	'l'
	sta	ERRCODE		; ERROR CODE l

	; test RLC
	mvi a	1
	rlc
	jc	error	; only CY is affected
	
	mvi a	80h
	rlc
	jnc	error		; CY=1	

	mvi a	'm'
	sta	ERRCODE		; ERROR CODE = m

	; test RRC
	mvi a 80h
	rrc
	jc	error

	mvi a 	1
	rrc
	jnc	error
	
	mvi a	'n'
	sta	ERRCODE		; ERROR CODE = n
	
	; test RAL
	ana a		; clr cy
	mvi a	80h
	ral
	jnc	error

	ana a		; clr cy
	mvi a 55h
	ral
	cpi	aah
	jnz	error
	
	mvi a	'o'
	sta	ERRCODE		; ERROR CODE = o

	ana a		; clr CY
	mvi a	80h
	rar
	cpi	40h
	jnz	error

	ana a
	mvi a	1
	rar
	jnc	error	; CY=1
	jz	error	; Z=0

	mvi a	1
	stc
	rar
	jz	error
	jnc	error
	cpi	80h
	jnz	error
	
	ana a	; clear CY
	mvi a	aah
	rar
	cpi	55h
	jnz	error

	mvi a	'p'
	sta	ERRCODE		; ERROR CODE p

	; test jmp and varients
	xra a		; Z=1
	jz	loop1
	jmp	error
loop1	
	ana a		; CY=0
	jnc	loop2
	jmp	error
loop2
	ana a		; CY=0	
	mvi a 1
	adi 2		; P=1
	jpe	loop3
	jmp	error
loop3	
	jp	loop4	; S=0
	jmp	error
loop4
	adi	80h	; S=1
	jm	loop5
	jmp	error
loop5
	mvi a	1
	sui	1	; Z=1
	jz	loop6
	jmp	error
loop6

	mvi a	'q'
	sta	ERRCODE		; ERROR CODE q

	; test call and varients

	xra a		; Z=1
	cz	ctest1
	jnz	error	; returned w/Z=1

	ana a		; CY=0
	cnc	ctest2
	jnc	error

	mvi a	1
	adi	1	; S=0
	cp	ctest3
	jp	error
		
	mvi a	1
	adi	1	; P=0
	cpo	ctest4

	jpo	error
	
	mvi a	'r'
	sta	ERRCODE		; ERROR CODE r
	lxi h	pctest
	pchl			; test pchl
	jmp	error

pctest
	mvi a	's'
	sta	ERRCODE		; ERROR CODE s
	
	; test stack
	lxi h	1234h
	push h
	lxi h	0000h
	pop h
	mvi a	12h
	cmp h
	jnz	error
	mvi a	34h
	cmp l
	jnz	error

	lxi b	1234h
	push b
	lxi b	0000h
	pop b
	mvi a	12h
	cmp b
	jnz	error
	mvi a	34h
	cmp c
	jnz	error

	lxi d	1234h
	push d
	lxi d	0000h
	pop d
	mvi a	12h
	cmp d
	jnz	error
	mvi a	34h
	cmp e
	jnz	error

	mvi a	't'
	sta	ERRCODE		; ERROR CODE t
	
	; test psw
	mvi a	aah	
	adi	1
	push	psw
	xra a
	pop	psw
	jz	error
	jp	error
	jc	error
	jpe	error
	cpi	abh
	jnz	error

	mvi a	'u'
	sta	ERRCODE		; ERROR CODE u

	; test xthl
	lxi b	1234h
	push b
	lxi h	a55ah
	xthl
	mvi a	34h
	cmp	l
	jnz	error
	mvi a	12h
	cmp h
	jnz	error

	pop h
	mvi a	5ah
	cmp l
	jnz	error
	mvi a	a5h
	cmp h
	jnz	error

	mvi a	'v'
	sta	ERRCODE		; ERROR CODE v

	; test sphl
	lxi h	STACK2
	sphl
	lxi h	0000h
	dad sp
	mvi a	65h
	cmp h
	jnz	error
	mvi a	43h
	cmp l
	jnz	error
	lxi sp	STACK	; restore the stack	

	; test DAA
	mvi a	83h
	mvi b	54h
	add b
	daa
	jnc	error	; result is 1 37 decimal (CY=1, 37 in A)
	cpi	37h	; NOTE: cpi clears CY
	jnz	error
	
	mvi a	38h
	mvi b	45h
	add b
	daa
	jc	error	; result 0 83h
	cpi	83h
	jnz	error


	mvi a	38h
	mvi b	41h
	add b
	daa		; result 0 79h
	jc	error
	cpi	79h
	jnz	error


	mvi a	88h
	mvi b	44h
	add b
	daa		; result 1 32h
	jnc	error
	cpi	32h
	jnz	error



	; all tests passed - show a '.' for each loop (set in diag)
	mvi a	'.'
	out	SOUT
	lda	CNTR
	dcr a
	sta	CNTR
	jz	ok		; counter at 0, all passed. Show & stop.

	; now keep doing
	jmp	diag	; loop thru again

	; --- END ---


;---- SUBROUTINES ---

ctest1	; from cz
	xra a		; Z=1
	rz	
	jmp	error

ctest2	; from cnc
	stc
	rc
	jmp	error

ctest3	; from cp S=0
	adi	80h	; S=1
	rm		; return minus
	jmp	error

ctest4	; from cpo P=0
	adi 1		; P=1
	rpe
	jmp	error


TEMPH	DS	2		; temp storage
TEMPL	DS	2		; temp storage
ERRCODE	DS	1		; holds the error code if a test failed
TMPL	DS	1		; for lhld, shld
TMPH	DS	1
CNTR	DS	1		; loop counter
