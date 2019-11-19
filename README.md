# nano80
Arduino nano 8080 emulator: a single board computer with FRAM, a front panel and LCD display.

This is an 8080 simulator/emulator for an Arduino Nano. It uses about 12K of memory and
appx 600 bytes of ram (dynamic memory).

This is a front panel programmable computer, something like the single board trainers used in the 
70's and 80's in colleges. Using a few switches and push buttons, along with some led's and a 
1x8 lcd display, you can program the memory, step thru and view memory and run 8080 programs.

See the schematic for wiring. 

I use a 32Kx8 FRAM for main memory. This slows it down a lot, but it gives you lots of room
to play around. If you want more speed, change the fram.[read8/write8] to mem[] and set up
about 1K of arduino ram. Doing this gives a speed improvement of about 4.5 times.

The serial port is defined so that your 8080 has a serial port in addition to 8 led's and 8 
toggle switches for input/output.

I'm sure there are bugs. Please let me know so I can fix them.

Also here is asm80.c
This is a command line 8080 assembler I wrote some time ago. Tested under Linux, compiled with gcc.
To compile: cc -o asm80 asm80.c
