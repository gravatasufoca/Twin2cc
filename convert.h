// Last Modification: 03-06-2009
#include <stdio.h>

void bin8(unsigned char n, char *s);
void bin16(unsigned short n, char *s);
void bin32(unsigned short n, char *s);
void array2bin( char *src, char *dest, int count );
void hex8(int n, char *s);
void hex16(int n, char *s);
void hex32(int n, char *s);
char *array2hex( char *src, char *dest, int count );
int hex2int(char *src);
int hex2array( char *src, unsigned char *dest);
