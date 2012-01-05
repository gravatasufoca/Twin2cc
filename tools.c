#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "debug.h"

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

#include "tools.h"

#ifdef WIN32

void usleep( int count )
{
  DWORD Ticks = GetTickCount() + count/1000;
  while (Ticks>GetTickCount()) ;
}

#else

unsigned int GetTickCount()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    // this will rollover ~ every 49.7 days
    return (unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

unsigned int getseconds()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    // this will rollover ~ every 49.7 days
    return (unsigned int)tv.tv_sec;
}


#endif

