CROSS=$(HOME)/mipsel-unknown-linux-gnu/bin/mipsel-unknown-linux-gnu-
CC	= $(CROSS)gcc
CXX	= $(CROSS)g++
STRIP	= $(CROSS)strip
CFLAGS	= -O2 -I.
LFLAGS	= -lpthread
#KERNELDIR= /snakeos-sdk/kernels/linux

OBJECTS = tools.o convert.o debug.o sockets.o serial.o config.o threads.o sha1.o cccamcrypt.o cccamconn.o cccam-srv.o main.o 

link : $(OBJECTS)
	$(CC) $(LFLAGS) -o twin2cc $(OBJECTS)
	$(STRIP) twin2cc

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	-rm *.o
	-rm twin2cc
