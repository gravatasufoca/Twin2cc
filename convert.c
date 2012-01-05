// Last Modification: 03-06-2009
#include <stdio.h>

#include "convert.h"

///////////////////////////////////////////////////////////////////////////////
void bin8(unsigned char n, char *s)
{
  int i;
  for ( i=7; i>=0; i-- )
  {
    *s = '0' + ( (n >> i)& 1 );
    s++;
  }
  *s='\0';
}

///////////////////////////////////////////////////////////////////////////////
void bin16(unsigned short n, char *s)
{
  int i;
  for ( i=15; i>=0; i-- )
  {
    *s = '0' + ( (n >> i)& 1 );
    s++;
  }
  *s='\0';
}

///////////////////////////////////////////////////////////////////////////////
void bin32(unsigned short n, char *s)
{
  int i;
  for ( i=31; i>=0; i-- )
  {
    *s = '0' + ( (n >> i)& 1 );
    s++;
  }
  *s='\0';
}

///////////////////////////////////////////////////////////////////////////////
void array2bin( char *src, char *dest, int count )
{
  int i,j;
  char c;
  for ( i=0; i<count; i++)
  {
    c = *src;
    for ( j=7; j>=0; j-- )
    {
      *dest = '0' + ( (c >> j)& 1 );  // if ( (c >> j)&1 ) *dest = '1'; else *dest='0';
      dest++;
    }
    *dest=' '; dest++;
    src++;
  }
  *dest='\0';
}



///////////////////////////////////////////////////////////////////////////////
char hexchars[]="0123456789ABCDEF";
void hex8(int n, char *s)
{
  int i;
  for( i=4; i>=0; i-=4 ) 
  {
    *s = hexchars[ (n>>i) & 0x0F ];
    s++;
  }
  *s = '\0';
}

///////////////////////////////////////////////////////////////////////////////
void hex16(int n, char *s)
{
  int i;
  for( i=12; i>=0; i-=4 ) 
  {
    *s = hexchars[ (n>>i) & 0x0F ];
    s++;
  }
  *s = '\0';
}

///////////////////////////////////////////////////////////////////////////////
void hex32(int n, char *s)
{
  int i;
  for( i=28; i>=0; i-=4 ) 
  {
    *s = hexchars[ (n>>i) & 0x0F ];
    s++;
  }
  *s = '\0';
}

///////////////////////////////////////////////////////////////////////////////
char *array2hex( char *src, char *dest, int count )
{
  int i;
  char c;
  for ( i=0; i<count; i++)
  {
    c = *src;
    *dest = hexchars[ (c>>4) & 0x0F ]; dest++;
    *dest = hexchars[ c & 0x0F ]; dest++;
    *dest = ' '; dest++;
    src++;
  }
  *dest='\0';
  return dest;
}

///////////////////////////////////////////////////////////////////////////////
int hex2int(char *src)
{
 int rt=0;
 while (1)
 {
   if ( (*src>='0')&&(*src<='9') )  rt = (rt<<4) + (*src-'0');
   else if ( (*src>='a')&&(*src<='f') ) rt = (rt<<4) + (*src-'a')+10;
   else if ( (*src>='A')&&(*src<='F') ) rt = (rt<<4) + (*src-'A')+10;
   else break;
   src++;
 }
 return ( rt );
}

///////////////////////////////////////////////////////////////////////////////
int hex2array( char *src, unsigned char *dest)
{
  unsigned int rt=0;
  unsigned char c;
  int size=0;
  int chars=0;
  do
  {
    c=*src;
    if ( (c>='0')&&(c<='9') ) { rt = (rt<<4) + (c-'0'); chars++; }
    else if ( (c>='a')&&(c<='f') ) { rt = (rt<<4) + (c-'a')+10; chars++; }
    else if ( (c>='A')&&(c<='F') ) { rt = (rt<<4) + (c-'A')+10; chars++; }
    else if (chars>0) 
    {
      *dest = rt;
      dest++;
      size++;
      rt=0;
      chars=0;
    }
    if (chars==2)
    {
      *dest = rt;
      dest++;
      size++;
      rt=0;
      chars=0;
    }    
    src++;
  } while (c!='\0');
  return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

