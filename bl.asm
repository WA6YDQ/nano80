; bootloader for nano 80

STATUS	equ	2	; serial port status. >1=byte avail
SIN	equ	1	; serial port in
LED	equ	0	; led display

	org	7fc0h	; put the bootloader in hi memory
			; then load C3 C0 7F to address 0000 and press reset

	lxi h	0000h	; start of memory
	xra a		; clear the LED port
	out	LED

loop1	in	STATUS
	cpi	0
	jz	loop1

	in	SIN	; get a byte
	; got a byte. 1st 2 bytes are file size
	mov	b,a	; save high byte
	in	SIN
	mov	c,a	; save low byte

loop2	in	SIN
	out	LED
	mov	m,a	; save to memory
	inx 	h	; next address
	dcx	b	; dec counter
	xra	a	; clear A
	cmp	c	; see if BC at 0
	jnz	loop2	; wait for next char
	cmp	b
	jnz	loop2	; loop until BC=0

	; done. read in all bytes
	xra a
	out	LED	; clear LED's
	hlt

