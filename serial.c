#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "serial.h"

/***********************************************************************
  Open modem device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
  // baud, parity data, stopbits
***********************************************************************/
int serial_init(char *devicename,int flags,int *resfd)
{
  int fd;
  struct termios newtio;

  fd = open(devicename, O_RDWR | O_NOCTTY);// | O_NONBLOCK);// | O_NDELAY);
  if (fd<0) return -1;
  //tcgetattr(fd,&oldtio); /* save current serial port settings */
  bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
        
  newtio.c_cflag = flags; //B9600 | CS8 | CLOCAL | CREAD; // IRDETO
  //newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD | CSTOPB | PARENB | PARODD; // VIACCESS
  newtio.c_iflag = 0;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
    
  newtio.c_cc[VMIN]=0;
  newtio.c_cc[VTIME]=2;
  /* 
    now clean the modem line and activate the settings for the port
  */
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);
  *resfd = fd;
}

/******************************************************************************
restore the old port settings
******************************************************************************/
void serial_done(int fd)
{
  //tcsetattr(fd,TCSANOW,&oldtio);
  close(fd);
}

int serial_read(int fd, char *buf, int len)
{
  return read(fd, buf, len);
}

int serial_write(int fd, char *buf, int len)
{
  return write(fd, buf, len);
}

int serial_setctrl(int fd, int ctrl)
{
  int tmp;
  ioctl(fd, TIOCMGET, &tmp);
  tmp &= ~(TIOCM_RTS | TIOCM_CTS | TIOCM_DTR);
  tmp |= ctrl; //TIOCM_RTS | TIOCM_CTS | TIOCM_DTR;
  return (ioctl(fd, TIOCMSET, &tmp));
}

void serial_purge(int fd)
{
  tcflush(fd,TCIFLUSH);
  tcflush(fd,TCOFLUSH);
}

void serial_clrrts(int fd)
{
  int mcs;
  /* clear RTS */
  ioctl(fd, TIOCMGET, &mcs);
  mcs^=TIOCM_RTS;
  ioctl(fd, TIOCMBIC, &mcs);
}

void serial_setrts(int fd)
{
  int arg=TIOCM_RTS; 
  ioctl(fd, TIOCMBIS, &arg);
}

/*
* Set or Clear RTS modem control line
*
* Note: TIOCMSET and TIOCMGET are POSIX
*
* the same things:
*
* TIOCMODS and TIOCMODG are BSD (4.3 ?)
* MCSETA and MCGETA are HPUX
*/
void serial_rts(int fd, int on)
{
  int controlbits;
  ioctl(fd, TIOCMGET, &controlbits);
  if (on) {
    controlbits |= TIOCM_RTS;
  } else {
    controlbits &= ~TIOCM_RTS;
  }
  ioctl(fd, TIOCMSET, &controlbits);
}


/*
* Set or Clear DTR modem control line
*
* Note: TIOCMBIS: CoMmand BIt Set
* TIOCMBIC: CoMmand BIt Clear
*
*/
void serial_dtr (int fd, int on)
{
  int controlbits = TIOCM_DTR;
  ioctl(fd, (on ? TIOCMBIS : TIOCMBIC), &controlbits);
}

// timeout in ms
int  serial_readt(int fd, int timeout0, int timeout, int size, unsigned char *buf)
{
  unsigned char c;
  int count, rv;
  struct pollfd pfd;
  for (count = 0; count < size; count++) {
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0x0000;
    if (count==0) rv = poll(&pfd, 1, timeout0 ); else rv = poll(&pfd, 1, timeout );
    if ( rv!= 1) return count;
    if ((pfd.revents & POLLIN) == POLLIN) {
      if (read(fd, &c, 1) != 1) return count;
      buf[count] = c;
    }
    else return count;
  }
  return size;
}


int phoenix_reset(int fd) 
{
// CARD_RST_PIN=0 ==> Card Reset
  printf("Resetting Card...\n");
  tcflush(fd, TCIOFLUSH);
  // Set the reset line
  if (serial_setctrl( fd, TIOCM_RTS | TIOCM_CTS | TIOCM_DTR ))
    {
      printf("phoenix_error: internal error on file descriptor\n");
      return (-1);
    }
 
  usleep(20000);
  // Clear the reset line
  if (serial_setctrl(fd, TIOCM_CTS | TIOCM_DTR))
    {
      printf("phoenix_error: internal error on file descriptor\n");
      return (-1);
    }
  usleep(100000);
  printf("success\n");
  return (0);
}

