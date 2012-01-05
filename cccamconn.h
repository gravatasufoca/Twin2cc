// CCcam Connection Functions

#define CC_MAXMSGSIZE	1024
#define CC_MAX_PROV		16
#define CC_MAX_ECMS		50  // before reconnect

typedef enum
{
  CC_MSG_CLI_INFO,			// client -> server
  CC_MSG_ECM_REQUEST,		// client -> server
  CC_MSG_EMM_REQUEST,		// client -> server
  CC_MSG_CARD_DEL = 4,		// server -> client
  CC_MSG_BAD_ECM,
  CC_MSG_KEEPALIVE,		// client -> server
  CC_MSG_CARD_ADD,			// server -> client
  CC_MSG_SRV_INFO,			// server -> client
  CC_MSG_CMD_0B = 0x0b,	// server -> client ???????
  CC_MSG_ECM_NOK1 = 0xfe,	// server -> client ecm queue full, card not found
  CC_MSG_ECM_NOK2 = 0xff,	// server -> client
  CC_MSG_NO_HEADER = 0xffff
} cc_msg_cmd;

int cc_msg_recv(int handle,struct cc_crypt_block *recvblock, uint8 *buf);
int cc_msg_recv_nohead(int handle, struct cc_crypt_block *recvblock, uint8 *buf, int len);
int cc_msg_send(int handle,struct cc_crypt_block *sendblock, cc_msg_cmd cmd, int len, uint8 *buf);

