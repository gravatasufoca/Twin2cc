#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string.h>


#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

#include "common.h"
#include "debug.h"


char debugline[0x1000];

void CurrentTime()
{
  time_t seconds;
  seconds = time (NULL);
  int h,m,s;
  h = (seconds/3600)%24;
  m = (seconds/60)%60;
  s = (seconds%60);
  printf("[%02d:%02d:%02d] ", h,m,s);
}

char* debugtime(char *str)
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    int ms = tv.tv_usec / 1000;
    int hr = (1+(tv.tv_sec/3600)) % 24;
    int mn = (tv.tv_sec % 3600) /60;
    int sd = (tv.tv_sec % 60);
    sprintf( str, "[%02d:%02d:%02d.%03d]", hr,mn,sd,ms);
    return str;
}

void debug(const char *format, ...)
{
  FILE *fhandle;
  va_list args;
  va_start (args, format);
  vsprintf(debugline, format, args);
  va_end( args );
  //CurrentTime();
  char str[32];
  debugtime( &str[0] );
  printf("%s %s", str,debugline);
}

void debugdump(unsigned char *buf,int len,const char *format, ...)
{
  int i;
  FILE *fhandle;
  int index;
  va_list args;
  va_start (args, format);
  vsprintf(debugline, format, args);
  va_end( args );

  index = strlen(debugline);
  for(i=0;i<len;i++) sprintf(&debugline[index+i*2],"%02X",buf[i]);
  char str[32];
  debugtime(&str[0]);
  printf("%s %s\n", str,debugline); 
}

