nano80 - An Arduino powered 8080 single board computer.


This is an 8080 simulator/emulator for an Arduino Nano. It uses about 
12K of memory and appx 600 bytes of ram (dynamic memory).

This is a front panel programmable computer, something like the 
single board trainers used in the 70's and 80's in colleges. 
Using a few switches and push buttons, along with some led's 
and a 1x8 lcd display, you can program the memory, step thru 
and view memory and run 8080 programs. 

See the schematic for wiring and the program #defines for details.

I use a 32Kx8 FRAM for main memory. This slows it down a lot, but 
it gives you lots of room to play around. If you want more speed, 
change the fram.[read8/write8] to mem[] and set up about 1K of 
arduino ram.

Note: think of this as an 8080 running at about 1mhz with extremly long 
memory wait states. It takes 14 seconds to read/write 32K bytes to the FRAM.
That's about 2 bytes/msec. Going to SPI instead of I2C will resolve this 
but I use those lines for other things. Also, the biggest SPI chip I saw was 
8K. Not enough.

The serial port is defined so that your 8080 has a serial port in 
addition to 8 led's and 8 toggle switches for input/output. Default speed
is set (in #define) for 9600. NOTE: because of FRAM access, you are limited to
reading/writing as many bytes as the serial buffer allows. Varies, but about
128 bytes. Any more and you will see data loss.

I'm sure there are bugs. Please let me know so I can fix them.

------------------------------

Also here is asm80.c

This is a command line 8080 assembler I wrote some time ago. Tested 
under Linux, compiled with gcc. 

To compile: cc -o asm80 asm80.c

There are some .asm examples for using the assembler as well as a README file.

------------------------------------

There are some 8080 source code files for testing the system. 

sertest.asm 

is a simple test of the serial port. 

cylon.asm 

is a flashy lights program.

There is a program called dump.c This is a hex dump program 
I wrote to view object code files. 

Compile with cc -o dump dump.c
  
  
MIT license applies to all code here unless described otherwise.

--Kurt
