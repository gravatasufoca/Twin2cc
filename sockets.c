#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <errno.h>
#endif

#include "common.h"
#include "sockets.h"


// CONVERTION
uint hostname2ip( const char *hostname )
{
  struct hostent *phostent;
  unsigned int hostaddr;
  unsigned char *temp;

  phostent = gethostbyname(hostname);
  if (phostent==NULL) {
    //printf("errror gethostbyname() %d\n",  WSAGetLastError());
    return 0;
  }
  temp = ((unsigned char *) phostent->h_addr_list[0]);
  hostaddr = *(unsigned int*)temp;//   *(*temp<<24) + ( *(temp+1)<<16 ) + ( *(temp+2)<<8 ) + (*(temp+3));
  //printf("IP = %03d.%03d.%03d.%03d\n", *temp, *(temp+1), *(temp+2), *(temp+3));
  //if (hostaddr==0x7F000001) hostaddr=0;
  return hostaddr;
}

char *iptoa(char *dest, unsigned int ip )
{
  sprintf(dest,"%d.%d.%d.%d", 0xFF&(ip), 0xFF&(ip>>8), 0xFF&(ip>>16), 0xFF&(ip>>24));
  return dest;
}

char ip_string[0x40];
char *ip2string( unsigned int ip )
{
  return iptoa(ip_string, ip );
}

////////////////////////////////////////////////////////////////////////////////
// SOCKETS FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

int fdstatus_read(SOCKET s)
{
  fd_set readfds;
  int retval;
  struct timeval timeout;
  FD_ZERO(&readfds);
  FD_SET(s, &readfds);
  timeout.tv_usec = 0;
  timeout.tv_sec = 0;
  do {
#ifdef WIN32
  retval = select(0, &readfds, NULL, NULL, NULL); //&timeout); 
#else
  retval = select(s+1, &readfds, NULL, NULL,&timeout); 
#endif
  } while(retval<0 && errno==EINTR);
  return retval;
}

int fdstatus_readt(SOCKET s, int tim)
{
  fd_set readfds;
  int retval;
  struct timeval timeout;

	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	timeout.tv_usec = (tim%1000)*1000;
	timeout.tv_sec = tim/1000;
  do {
#ifdef WIN32
	retval = select(0, &readfds, NULL, NULL, &timeout); 
#else
	retval = select(s+1, &readfds, NULL, NULL,&timeout); 
#endif
  } while(retval<0 && errno==EINTR);

  return retval;
}

int fdstatus_writet(SOCKET s, int tim)
{
  fd_set writefds;
  int retval;
  struct timeval timeout;

	FD_ZERO(&writefds);
	FD_SET(s, &writefds);
	timeout.tv_usec = (tim%1000)*1000;
	timeout.tv_sec = tim/1000;
  do {
#ifdef WIN32
	retval = select(0, NULL, &writefds, NULL, &timeout); 
#else
	retval = select(s+1, NULL, &writefds, NULL,&timeout); 
#endif
  } while( (retval<0) && ( (errno==EINTR)||(errno==EAGAIN) ) );

  return retval;
}

int fdstatus_write(SOCKET s)
{
  fd_set writefds;
  int retval;
  struct timeval timeout;
  FD_ZERO(&writefds);
  FD_SET(s, &writefds);
  timeout.tv_sec = 0;
  timeout.tv_usec = 100;
  do {
	retval = select(s+1, NULL, &writefds, NULL,&timeout); 
  } while ( (retval<0) && ( (errno==EINTR)||(errno==EAGAIN) ) );
  return retval;
}


int fdstatus_accept(SOCKET s)
{
  fd_set fd;
  int retval;
  struct timeval timeout;

  FD_ZERO(&fd);
  FD_SET(s, &fd);
  timeout.tv_usec = 0;
  timeout.tv_sec = 10;
  do {
	retval = select(FD_SETSIZE, &fd, NULL, NULL,&timeout); 
  } while(retval<0 && errno==EINTR);
  return retval;
}


int SetSocketTimeout(SOCKET connectSocket, int milliseconds)
{
    struct timeval tv;

	tv.tv_sec = milliseconds / 1000 ;
	tv.tv_usec = ( milliseconds % 1000) * 1000  ;

    return setsockopt (connectSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof tv);
}


int SetSocketKeepalive(SOCKET sock)
{
	int val = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) < 0) return -1; 
	val = 3; 
	if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&val, sizeof(val)) < 0) return -1;
	if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, (void*)&val, sizeof(val)) < 0) return -1;
	val = 1;
	if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, (void*)&val, sizeof(val)) < 0) return -1;
	return 0;
}

int SetSocketNoDelay(SOCKET sock)
{
	int i;
	socklen_t   len;

	i = 1;
	len = sizeof(i);
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &i, len) < 0) return -1; 
	return 0;
}

/*
int SetSocketPriority(SOCKET sock)
{
	setsockopt(sock, SOL_SOCKET, SO_PRIORITY, (void *)&cfg->netprio, sizeof(ulong));
}
*/

///////////////////////////////////////////////////////////////////////////////
// UDP CONNECTION
///////////////////////////////////////////////////////////////////////////////

SOCKET CreateServerSockUdp(int port)
{
  int reuse=1;
  SOCKET sock;
  struct sockaddr_in saddr;

  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock==INVALID_SOCKET) {
    printf("Error in INP int creation\n");
    return(INVALID_SOCKET);
  }
  int flgs=fcntl(sock,F_GETFL);
  fcntl(sock,F_SETFL,flgs|O_NONBLOCK);

  saddr.sin_family = PF_INET;
  saddr.sin_addr.s_addr = htonl( INADDR_ANY );
  saddr.sin_port = htons(port);

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int))< 0)
  {
#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    printf("setsockopt() failed\n");
    return INVALID_SOCKET;
  }

  if ( bind( sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in)) == -1 ) {
    printf("Error in bind INP int\n");
#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return(INVALID_SOCKET);
  }

  return( sock );
}

SOCKET CreateClientSockUdp()
{
  SOCKET sock;
  sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
  if (sock==INVALID_SOCKET) {
    printf("Error: socket()\n");
    return INVALID_SOCKET;
  }
  int flgs=fcntl(sock,F_GETFL);
  fcntl(sock,F_SETFL,flgs|O_NONBLOCK);
  return sock;
}


///////////////////////////////////////////////////////////////////////////////
// TCP CONNECTION
///////////////////////////////////////////////////////////////////////////////

SOCKET CreateServerSockTcp(int port)
{
  SOCKET sock;
  struct sockaddr_in saddr;
 // Set up server
  saddr.sin_family = PF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(port);
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if ( sock==INVALID_SOCKET )
  {
    debug("socket() failed\n");
    return INVALID_SOCKET;
  }

  int reuse=1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int))< 0)
  {
    close(sock);
    debug("setsockopt(SO_REUSEADDR) failed\n");
    return INVALID_SOCKET;
  }

  if ( bind(sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr))==SOCKET_ERROR )
  {
    close(sock);
    debug("bind() failed\n");
    return INVALID_SOCKET;
  }
  if (listen(sock, 1) == SOCKET_ERROR)
  {
    close(sock);
    debug("listen() failed\n");
    return INVALID_SOCKET;
  }
  return sock;
}


SOCKET CreateClientSockTcp(unsigned int netip, int port)
{
  SOCKET sock;
  struct sockaddr_in saddr;
         
  sock = socket(PF_INET,SOCK_STREAM,0);
  if( sock<0 ) {
    //printf("Invalid Socket\n");
    return INVALID_SOCKET;
  }

  int optVal = TRUE;
  if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&optVal, sizeof(int))==-1) {
#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return INVALID_SOCKET;
  }

  memset(&saddr,0, sizeof(saddr));
  saddr.sin_family = PF_INET;
  saddr.sin_port = htons(port);
  saddr.sin_addr.s_addr = netip;

  if( connect(sock,(struct sockaddr *)&saddr,sizeof(struct sockaddr_in)) != 0)
  {
    close(sock);
    return INVALID_SOCKET;
  }
  return sock;
}


///////////////////////////////////////////////////////////////////////////////
// NON BLOCKED TCP CONNECTION
///////////////////////////////////////////////////////////////////////////////

int CreateClientSockTcp_nonb(unsigned int netip, int port)
{
	int flags, n;
	fd_set rset, wset;
	struct timeval tval;
	SOCKET sockfd;
	struct sockaddr_in saddr;
         
  struct protoent *ptrp;
  int p_proto;
  if ((ptrp = getprotobyname("tcp"))) p_proto = ptrp->p_proto; else p_proto = 6;

	sockfd = socket(PF_INET,SOCK_STREAM,p_proto);
	if( sockfd<0 ) return INVALID_SOCKET;

	//set to nonblocking mode
	flags=fcntl(sockfd,F_GETFL);
	if(flags<0) {
		close(sockfd);
		printf("socket: fcntl GETFL failed\n");
		return INVALID_SOCKET;
	}
	if ( fcntl(sockfd,F_SETFL,flags|O_NONBLOCK)<0 ) {
		close(sockfd);
		printf("socket: fcntl SETFL failed\n");
		return INVALID_SOCKET;
	}

	memset(&saddr,0, sizeof(saddr));
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = netip;
	errno = 0;
	do {
		n=connect( sockfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in) );
	} while ( (n<0)&&(errno==EINTR) );

	if ( (n<0)&&(errno!=EISCONN) ) {
		if (errno == EINPROGRESS || errno == EALREADY) {
			//usleep(100000);
			FD_ZERO(&rset);
			FD_SET(sockfd, &rset);
			wset = rset;
			tval.tv_sec = 3;
			tval.tv_usec = 0;
			if ( select(sockfd+1, &rset, &wset, 0, &tval) <= 0) {
				close(sockfd);
				return INVALID_SOCKET;
			}
			if ( FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset) ) {
				int error = -1;
				socklen_t len = sizeof(error);
				if ( getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 ) {
					close(sockfd);
					return INVALID_SOCKET;
				}
			}
		}
		else {
			if (errno == EBADF || errno == ENOTSOCK) debug("Socket: bad socket/descriptor\n");
			else if (errno == ETIMEDOUT) debug("Socket: connect timeout\n");
			else if (errno == ECONNREFUSED) debug("Socket: connection refused\n");
			else if (errno == ENETUNREACH) debug("Socket: network unreachable!\n");
			else if (errno == EADDRINUSE) debug("Socket: address in use!\n");
			close(sockfd);
			return INVALID_SOCKET;
		}
	}

	//restore blocking mode
	flags &=~ O_NONBLOCK;
	fcntl(sockfd, F_SETFL, flags);	/* restore file status flags */

	//SetSocketKeepalive(sockfd);
	SetSocketNoDelay(sockfd);

	return sockfd;
}

int recv_nonb2(SOCKET sock,uint8 *buf,int len,int timeout)
{
	int retval;
    int index = 0;

	uint32 ticks = GetTickCount()+timeout; // timeout ~ 2sec
	uint32 now;
	do {
		now = GetTickCount();
		if ( ticks<now ) {
			debug("socket: receive timeout\n");
			return -2; // timeout
		}
		errno = 0;
		retval = fdstatus_readt(sock,ticks-now);
		switch (retval) {
			case -1: // error
				if (errno==EINTR) continue;
				if (errno==EAGAIN) return -2;
				return -1;
			case 0: // timeout
				return -2;
			default: // nb descriptors
				do {
					errno = 0;
			  		retval = recv(sock, buf+index, len-index, 0);
				} while(retval<0 && errno==EINTR);
				switch (retval) {
					case -1:
						if (errno==EAGAIN) return -2;
						return -1;
					case 0:
						if (errno==EINTR) return index;
						else if (!index) return -1;
							else return index;
					default: index+=retval;
				}
		}
	} while (index<len);
	return index; 
}

int recv_nonb(SOCKET sock,uint8 *buf,int len,int timeout)
{
	int retval;
    int index = 0;

	uint32 ticks = GetTickCount()+timeout; // timeout ~ 2sec
	uint32 now;
	do {
		now = GetTickCount();
		if ( ticks<now ) {
			debug("socket: receive timeout\n");
			return -2; // timeout
		}
		errno = 0;
		retval = fdstatus_readt(sock,ticks-now);
		switch (retval) {
			case -1: // error
				if (errno==EINTR) continue;
				if (errno==EAGAIN) continue;
				return -1;
			case 0: // timeout
				debug("socket: receive timeout\n");
				return -2;
			default: // nb descriptors
		  		retval = recv(sock, buf+index, len-index, 0);
				switch (retval) {
					case -1:
						if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR) return -1;
					case 0:
						errno = ECONNRESET;
						return -1;
					default: index+=retval;
				}
		}
	} while (index<len);
	return index; 
}


int send_nonb2(SOCKET sock,uint8 *buf,int len,int timeout)
{
	int retval;
	int index = 0;

	uint32 ticks = GetTickCount()+timeout;
	uint32 now;
	do {
		now = GetTickCount();
		if ( ticks<now ) {
			debug("send error timeout\n");
			return -2; // timeout
		}
		errno = 0;
		retval = fdstatus_writet(sock,ticks-now);
		switch (retval) {
			case -1: // error
				if (errno == EINTR) continue;
				debug("send error %d(%d)\n",retval,errno);
				return retval; // disconnection
			case 0: // timeout
				return retval;
			default: // nb. desriptors
				do {
					errno = 0;
			  		retval = send(sock, buf+index, len-index, 0);
				} while( (retval<0)&&(errno==EINTR) );
				if(retval>0) index+=retval;
		}
	} while (retval>0 && index<len);
	if (retval>0) return index; else return -1;
}

int send_nonb(SOCKET sock,uint8 *buf,int len,int to)
{
	int remain, got;
	uint8 *ptr;
	int  error, width;
	fd_set writefds;
	struct timeval timeout;

    error           = 0;
	timeout.tv_usec = (to%1000)*1000;
	timeout.tv_sec = to/1000;
    remain = len;
    ptr    = buf;

    while (remain) {
	    FD_ZERO(&writefds);
	    FD_SET(sock, &writefds);
        error = select( sock+1, NULL, &writefds, NULL, &timeout );
        if (error == 0) {
            errno = ETIMEDOUT;
			debug("send error %d\n",errno);
            return FALSE;
        } else if (error < 0) {
            if (errno != EINTR && errno != EAGAIN) {
				debug("send error %d\n",errno);
                return FALSE;
            }
        } else {
            got = write(sock, (void *) ptr, (size_t) remain);
            if (got >= 0) {
                remain -= got;
                ptr    += got;
            } else if (
                errno != EWOULDBLOCK &&
                errno != EAGAIN      &&
                errno != EINTR
            ) {
				debug("send error %d\n",errno);
                return FALSE;
            }
        }
    }
    return TRUE;
}

SOCKET CreateServerSockTcp_nonb(int port)
{
  SOCKET sock;
  struct sockaddr_in saddr;
 // Set up server
  saddr.sin_family = PF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(port);

  struct protoent *ptrp;
  int p_proto;
  if ((ptrp = getprotobyname("tcp"))) p_proto = ptrp->p_proto; else p_proto = 6;

  sock = socket(PF_INET, SOCK_STREAM, p_proto);
  if ( sock==INVALID_SOCKET )
  {
    printf("socket() failed\n");
    return INVALID_SOCKET;
  }

  int flgs=fcntl(sock,F_GETFL);
  if(flgs<0) {
	close(sock);
	printf("socket: fcntl GETFL failed\n");
    return INVALID_SOCKET;
  }
  if ( fcntl(sock,F_SETFL,flgs|O_NONBLOCK)<0 ) {
	close(sock);
	printf("socket: fcntl SETFL failed\n");
    return INVALID_SOCKET;
  }

  //SetSocketKeepalive(sock);
  SetSocketNoDelay(sock);

  int reuse=1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int))< 0)
  {
    close(sock);
    printf("setsockopt(SO_REUSEADDR) failed\n");
    return INVALID_SOCKET;
  }

  if ( bind(sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr))==SOCKET_ERROR )
  {
    close(sock);
    printf("bind() failed\n");
    return INVALID_SOCKET;
  }
  if (listen(sock, 5) == SOCKET_ERROR)
  {
    close(sock);
    printf("listen() failed\n");
    return INVALID_SOCKET;
  }
  return sock;
}

