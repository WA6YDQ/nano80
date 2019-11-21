; bootloader for nano 80

STATUS	equ	2	; serial port status. >0 => byte avail
SIN	equ	1	; serial port in
LED	equ	0	; led display

	org	7fc0h	; put the bootloader in hi memory

	lxi h	0000h	; start of memory
	xra a		; clear the LED port
	out	LED
start	in	STATUS	; wait until data avail
	cpi	0
	jz	start

	in	SIN	; get a byte
	; got a byte. 1st 2 bytes are file size
	mov	b,a	; save high byte
	in	SIN
	mov	c,a	; save low byte

loop	in	SIN
	out	LED
	mov	m,a	; save it
	inx 	h	; next address
	dcx	b	; dec counter
	xra	a	; clear A
	cmp	b	; test MSB of BC
	jnz	loop	; wait for next char
	cmp	c	; test LSB
	jnz	loop	; loop until BC=0

	; done. read in all bytes
	xra a
	out	LED	; clear LED's
	hlt

