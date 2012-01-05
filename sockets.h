
unsigned int hostname2ip( const char *hostname );
char *ip2string( unsigned int hostaddr );
char *iptoa(char *dest, unsigned int ip );

int fdstatus_read(SOCKET s);
int fdstatus_readt(SOCKET s, int tim);
int fdstatus_write(SOCKET s);
int fdstatus_accept(SOCKET s);
// SOCKET OPTIONS
int SetSocketTimeout(SOCKET connectSocket, int milliseconds);
int SetSocketKeepalive(SOCKET sock);
int SetSocketNoDelay(SOCKET sock);
// UDP CONNECTION
SOCKET CreateServerSockUdp(int port);
SOCKET CreateClientSockUdp();
// TCP CONNECTION
SOCKET CreateServerSockTcp(int port);
SOCKET CreateClientSockTcp(unsigned int netip, int port);
// TCP NON BLOCKED CONNECTION
int CreateClientSockTcp_nonb(unsigned int netip, int port);
int recv_nonb(SOCKET sock,uint8 *buf,int len,int timeout);
SOCKET CreateServerSockTcp_nonb(int port);
