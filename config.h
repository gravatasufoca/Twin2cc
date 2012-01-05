struct twin_client_data
{
	struct twin_client_data *next;
	// General Connection Data
	unsigned int ip;
	SOCKET handle;
	// NEWCAMD SPECIFIC DATA
	char user[64];
	char pass[64];
	unsigned char sessionkey[16];
	unsigned char oldcw[17];
};

struct twin_server_data
{
	struct twin_server_data *next;
	char dns[255];
	uint32 ip;
	int port;
	char user[32];
	char pass[32];
	SOCKET handle;
	char device[32];
};

struct twin_card_data
{
	unsigned short caid;		// Card CAID
	int  nbprov;			// Nb providers
	unsigned int prov[16];		// Card Providers
};
struct cccam_server {
	pthread_mutex_t lockcards;
	struct cc_card_data *cards;
	struct cc_client_data *clients;
	struct cc_sat_info *sats;
	
	int handle;
	int port; // output port
	int cwdelay;
	int cards_id; // incremental cards id for local share
	unsigned char nodeid[8];
	char version[32];
	char build[32];

};
struct config_data
{
  	struct cccam_server cccam;
#ifdef NEWCAMD_SERVER
	struct newcamd_server newcamd;
#endif

	struct twin_client_data *client;
	struct twin_card_data card;

	//NEWCAMD SERVER
	unsigned char key[16];
	int port; // output port
	SOCKET handle;

	SOCKET com_handle;
	char com_device[32];
	unsigned long twin_try_delay;
	unsigned int  twin_max_tries;
        unsigned long twin_time_out_nrw ;
// 	unsigned long twin_key_delay ;

};


struct channel_info_data {
		uint16 caid;
		uint32 prov;
		uint16 sid;
		uint16 deg;
		uint16 freq;
		char name[64];

		int ecmlen;
		uint32 ecmcrc;
		uint8 cw[16];
};

extern struct channel_info_data chninfo[1024];
extern int nbchninfo;
extern struct config_data cfg;

int read_chninfo(char *chninfofile);
int getchannel(uint16 caid, uint32 prov, uint16 sid);
int read_config(char *config_file, struct config_data *cfg);
