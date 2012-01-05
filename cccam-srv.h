#define TIME_CCCAM_KEEPALIVE 75

typedef enum
{
	ECM_STATUS_NONE,	// nothing done
	ECM_STATUS_BUSY,	// pending ecm request is sent to server and no data back
	ECM_STATUS_DEL,		// delete ecm request, client was disconnected
	ECM_STATUS_FAILED,
	ECM_STATUS_SUCCESS
} ecm_state;

// ECM REQUEST will be set into server ecmlist
struct cc_ecm_data {

	ecm_state state;			// Message sent to server?
	uint16 caid;				// CA id
	uint32 prov;				// Provider
	uint16 sid;					// Service id
	uint8 len;					// ECM Length
	char data[256];				// ECM data
	uint8 cw[16];				// cw
	uint32 sendtime;
	uint32 recvtime;

	//struct ecm_data *ecm;
	struct cc_card_data *card;
	struct cc_client_data *cli; // points to client sending ecm
	struct cc_ecm_data *next;	// next in server list
} __attribute__((packed));


//typedef uint8 tprovider[7];
struct tprovider {
	uint32 prov;
	uint32 ua;
};

struct cc_card_data {
	struct cc_card_data *prev;
	struct cc_card_data *next;
	struct cc_server_data *srv;
	uint32 localid;				// Card local id
	uint32 shareid;				// Card server id
	uint8 nodeid[8];
	uint8 uphops;
	uint8 maxdown;
	struct cc_sid_data *ignoresids;
	// Statistics
	uint32 ecmnb;	// ecm number requested by clients
	uint32 ecmok;	// dcw's returned to clients
	uint8 key[8];				// card serial (for au)
	uint16 caid;
	uint8 provcount;			// providers number
	struct tprovider prov[16];			// providers data	
};

struct cc_sat_info {
  struct cc_sat_info *prev;
  struct cc_sat_info *next;
  int sat;
  unsigned int timeout;
  
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum
{
	SRV_CCCAM,
	SRV_NEWCAMD,
} server_type;


struct cc_client_data { // Connected Client
	struct cc_client_data *next;

	int handle;				// SOCKET
	struct cc_crypt_block sendblock;	// crypto state block
	struct cc_crypt_block recvblock;	// crypto state block
	uint32 ip;
	uint32 connected;

	uint32 conntime; // Connection Time / Reconnection Timeout

	//Status
	int cardsent; // 0: cards are not sent, 1:cards are sent to client
	//fline
	char username[32];
	char password[64];
	unsigned char uphops;		// Max distance to get cards
	unsigned char shareemus;		// Client use our emu
	unsigned char allowemm;		// Client has rights for au

	unsigned char nodeid[8];
	char version[32];
	char build[32];
	// Statistics
	uint32 ecmnb;	// ecm number requested
	uint32 ecmok;	// dcw's returned

	// Current data used by client
	int reqecm; // BOOL: false:no ecm request, true: pending ecm request(wait for requesttime to send ecm)
	int reqchn; // chn index
	int reqrecvtime; // ecm request recvtime
	int reqtimeout;
	int reqcardid;
	uint8 cw[16];
	unsigned char oldcw[17];
	int reqmean;
};


void cc_card_add(struct cc_card_data *card);
void cc_sat_add(struct cc_sat_info *sat);
struct cc_card_data *getcardbylocalid(uint32 localid);
struct cc_card_data *getcardbycaidprov(uint16 caid, uint32 prov);


