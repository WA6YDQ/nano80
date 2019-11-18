; cylon - bounce led's back and forth
; k theis 11/2019

outport	equ 	0
	org	0

	lxi 	sp STACK
	xra 	a
	out 	outport
	inr 	a
	out 	outport
loop1	rlc
	out 	outport
	call	delay
	cpi	80h
	jnz	loop1

loop2	rrc
	out	outport
	call	delay
	cpi	1
	jnz	loop2
	jmp	loop1

delay	lxi b	0120h	; 0120h for FRAM, 04ffh for internal RAM
d1	dcr 	c	; holds 20h
	jnz	d1
	dcr	b	; holds 01h
	jnz	d1
	ret

DS 20h
STACK	; stack pointer w/20h bytes (16 levels deep)
