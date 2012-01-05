#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include "common.h"
#include "cccamcrypt.h"
#include "cccamconn.h"
#include "sockets.h"

//#define SERVER_KEEP_ALIVE 75
///////////////////////////////////////////////////////////////////////////////
// Return:
// =0: Disconnected
// -1: Packet Error
// >0: Success


int cc_msg_recv(int handle,struct cc_crypt_block *recvblock, uint8 *buf)
{
  int len,msglen;
  uint8 netbuf[CC_MAXMSGSIZE];

  if (handle < 0) return -1;

  //len = recv(handle, netbuf, 4, 0);
  len = recv_nonb(handle, netbuf, 4,2000);

  if (!len) return 0;

  if (len != 4) { // invalid header length read
   
    debug("CCcam: invalid header length\n");
    //debugdump(netbuf, len, "Header:");
    return -1;
  }

  cc_decrypt(recvblock, netbuf, 4);
  //debugdump(netbuf, 4, "CCcam: decrypted header:");

  if (((netbuf[2] << 8) | netbuf[3]) != 0) {  // check if any data is expected in msg
    if (((netbuf[2] << 8) | netbuf[3]) > CC_MAXMSGSIZE - 2) {
		debug("CCcam: message too big\n");
		//debugdump(netbuf, 4, "CCcam: decrypted header:");
		//exit(0);
		return -1;
    }

	len = recv_nonb(handle, netbuf+4, (netbuf[2] << 8) | netbuf[3],2000);

    if (len != ((netbuf[2] << 8) | netbuf[3])) {
      debug("CCcam: invalid message length read %d(%d)\n",(netbuf[2] << 8) | netbuf[3],len);
	  //debugdump(netbuf, len, "CCcam: Reveive Data");
      return -1;
    }

    cc_decrypt(recvblock, netbuf+4, len);
    len += 4;
  }

  //debugdump(netbuf, len, "CCcam: Reveive Data");
  memcpy(buf, netbuf, len);
  
  return len;
}

///////////////////////////////////////////////////////////////////////////////
// Return:
// =0: Disconnected
// -1: Packet Error
// >0: Success
int cc_msg_recv_nohead(int handle, struct cc_crypt_block *recvblock, uint8 *buf, int len)
{
	if (handle < 0) return -1;
	len = recv_nonb(handle, buf, len, 2000);  // read rest of msg
	cc_decrypt(recvblock, buf, len);
	return len;
}

///////////////////////////////////////////////////////////////////////////////
int cc_msg_send(int handle,struct cc_crypt_block *sendblock, cc_msg_cmd cmd, int len, uint8 *buf)
{
  int n;
  uint8 netbuf[CC_MAXMSGSIZE];

  memset(netbuf, 0, len+4);

  if (cmd == CC_MSG_NO_HEADER) memcpy(netbuf, buf, len);
  else {
    // build command message
    netbuf[0] = 0;   // flags??
    netbuf[1] = cmd & 0xff;
    netbuf[2] = len >> 8;
    netbuf[3] = len & 0xff;
    if (buf) memcpy(netbuf+4, buf, len);
    len += 4;
  }
  //debugdump(netbuf, len, "CCcam: Send data");
  cc_encrypt(sendblock, netbuf, len);
  n = send_nonb(handle, netbuf, len, 100);
  return n;
}

