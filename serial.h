#ifndef SERIAL_H
#define SERIAL_H

int serial_init(char *devicename,int flags,int *resfd);
void serial_done(int fd);
int serial_read(int fd, char *buf, int len);
int serial_write(int fd, char *buf, int len);
int serial_setctrl(int fd, int ctrl);
int phoenix_reset(int fd) ;
void serial_clrrts(int fd);
void serial_setrts(int fd);
void serial_rts(int fd, int on);
void serial_dtr (int fd, int on);

int  serial_readt(int fd, int timeout0, int timeout, int size, unsigned char *buf);

#endif
