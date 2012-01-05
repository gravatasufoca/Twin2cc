#ifndef _COMMON_H_
#define _COMMON_H_

#define CHKMAC

//#define SUPERUSER
//#define USE_THREAD
#define FALSE 0
#define TRUE 1

typedef unsigned char uchar;
typedef unsigned short int usint;typedef unsigned int uint;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

// win32 definitions
#ifndef WIN32

typedef int SOCKET;
typedef int HANDLE;
#define INVALID_HANDLE_VALUE -1
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#endif

#endif
