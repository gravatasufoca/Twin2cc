// CCcam Cryptage Functions

struct cc_crypt_block
{
	uint8 keytable[256];
	uint8 state;
	uint8 counter;
	uint8 sum;
} __attribute__((packed));



void cc_crypt_swap(unsigned char *p1, unsigned char *p2);
void cc_crypt_init( struct cc_crypt_block *block, uint8 *key, int len);
void cc_crypt_xor(uint8 *buf);
void cc_decrypt(struct cc_crypt_block *block, uint8 *data, int len);
void cc_encrypt(struct cc_crypt_block *block, uint8 *data, int len);
void cc_crypt_cw(uint8 *nodeid, uint32 card_id, uint8 *cws);

