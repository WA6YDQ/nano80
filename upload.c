/* 
 * upload.c  uploader for nano80
 * 
 * k theis 11/2019
 *
 * This is a simple serial terminal program used to upload .hex files
 * from a remote computer to the nano80. It sends .hex files over
 * a serial connection (default is /dev/USB0) at a defined speed (9600 here).
 *
 * The sequence is:
 * Place the nano80 in run mode (run/halt switch to run) and press/hold the LOAD
 * button while resetting the nano80 (NOT the arduino nano).
 *
 * The message "booting" will show up on the LCD. Now upload the file:
 * ./upload file.hex 
 * where file.hex is your file to transfer to the nano80.
 * The lights on the nano80 will blink and the display will show the final
 * address in hex when the upload has finished. After a second, the message 
 * 'Done' will be displayed on the LCD. Then the LCD will clear and the 
 * program will start to run.
 *
 *
 * derived from https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c 
*/

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


/* available baud rates */
/* B115200, B230400, B9600, B19200, B38400, B57600, B1200, B2400, B4800, B2400, B1200, B300 */

int main(int argc, char **argv) {

    char *portname = "/dev/ttyUSB0";
    int fd;
    int wlen;
    int n;
    char line[80];	
    FILE *infile;	// name of file to upload
     
    if (argc != 2) {	// show usage and exit
	printf("Usage: %s [file to upload]\n",argv[0]);
	exit(0);
    }

    /* open file for reading */
    infile = fopen(argv[1],"r");
    if (infile == NULL) {
	    fprintf(stderr,"Cannot open %s for read. Stopping.\n",argv[1]);
	    exit(1);
    }

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    /*baudrate 9600, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(fd, B9600);


    printf("Ready to send\n");
    /* send file line by line to the serial port */
    while (1) {
	fgets(line,80,infile);
	if (feof(infile)) break;
	printf("%s",line);	
	wlen = write(fd,line,strlen(line));
	if (wlen != strlen(line)) {
		printf("Write Error - %d %d\n",wlen,errno);
	}
	usleep(20000);	// 20 msec delay after each line allows the nano80 to save the data to FRAM
	tcdrain(fd);	// delay, flush output
    }
    
    
    /* done */
    exit(0);

}
