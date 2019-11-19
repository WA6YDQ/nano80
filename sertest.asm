; sertest.asm
; test serial read/write of nano80
; k theis 2019

; this program reads from the serial port. When a character
; is available, is returns it on the serial port ans sends it to 
;the LED display port as well.


	org	0
STATUS	equ	2	; serial status 0=not ready, n=# of chars
SIN	equ	1	; serial in
SOUT	equ	1	; serial out	
LED	equ	0	; LED output port

start	in	STATUS
	cpi	0
	jz	start

	mov	c,a	; save char count to receive
loop	in	SIN
	out	SOUT
	out	LED
	dcr	c
	jnz	loop
	
	jmp	start	; continue forever

