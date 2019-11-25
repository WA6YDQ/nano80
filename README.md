nano80 - An Arduino powered 8080 single board computer.


This is an 8080 simulator/emulator for an Arduino Nano. It uses about 
12K of memory and appx 600 bytes of ram (dynamic memory).

This is a front panel programmable computer, something like the 
single board trainers used in the 70's and 80's in colleges. 
Using a few switches and push buttons, along with some led's 
and a 1x8 lcd display, you can program the memory, step thru 
and view memory and run 8080 programs. There is a bootloader 
that allows loading an 8080 object coded program over the serial 
port as well as some development programs written in c.

See the schematic for wiring.

I use a 32Kx8 FRAM for main memory. This slows it down a lot, but 
it gives you lots of room to play around. If you want more speed, 
change the fram.[read8/write8] to mem[] and set up about 1K of 
arduino ram. Doing this gives a speed improvement of about 4.5 times.

The serial port is defined so that your 8080 has a serial port in 
addition to 8 led's and 8 toggle switches for input/output.

The input/output ports are defined as follows:
IN 0x00 is the toggle switches
IN 0x01 is the serial port using the arduino
IN 0x02 returns the number of available bytes on the serial port

OUT 0x00 is the 8 LED's 
OUT 0x01 is the serial port using the arduino

------------------------------

I'm sure there are bugs. Please let me know so I can fix them.

------------------------------

There is a boot loader in software to load intel .hex files 
into the nano80 memory. Loading more than a few bytes via switches
and lights get old fast. The boot loader lets you assemble large 
programs and upload then. See README.bootloader for use.

There are two programs used to create and send the .hexfiles:
ihex.c and upload.c

ihex converts the .com output of the assembler into intel .hex files
upload is a serial based uploader that allows for the slow memory 
access of the FRAM memory chip.

to compile these 2 programs:
cc -o ihex ihex.c
cc -o upload upload.c

-----------------------------------

Also here is asm80.c

This is a command line 8080 assembler I wrote some time ago. Tested 
under Linux, compiled with gcc. To compile: 

cc -o asm80 asm80.c

asm80 uses standard intel 8080 nmemonics. It is NOT a macro assembler.

-----------------------------------

There are some 8080 source code files for testing the system. 
sertest.asm is a simple test of the serial port. 
cylon.asm is a flashy lights program.

-----------------------------------

There is a program called dump.c This is a hex dump program 
I wrote to view object code files. 

Compile with:

cc -o dump dump.c

-----------------------------------

MIT license applies to all files herein unless otherwise noted.

--Kurt
