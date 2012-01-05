
#ifdef WIN32

void usleep( int count );

#else

unsigned int GetTickCount();
unsigned int getseconds();

#endif

