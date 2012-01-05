#include "common.h"
#include "cccamcrypt.h"


///////////////////////////////////////////////////////////////////////////////
void cc_crypt_swap(unsigned char *p1, unsigned char *p2)
{
  unsigned char tmp=*p1; *p1=*p2; *p2=tmp;
}

///////////////////////////////////////////////////////////////////////////////
void cc_crypt_init( struct cc_crypt_block *block, uint8 *key, int len)
{
  int i = 0 ;
  uint8 j = 0;

  for (i=0; i<256; i++) {
    block->keytable[i] = i;
  }

  for (i=0; i<256; i++) {
    j += key[i % len] + block->keytable[i];
    cc_crypt_swap(&block->keytable[i], &block->keytable[j]);
  }

  block->state = *key;
  block->counter=0;
  block->sum=0;
}

///////////////////////////////////////////////////////////////////////////////
// XOR init bytes with 'CCcam'
void cc_crypt_xor(uint8 *buf)
{
  const char cccam[] = "CCcam";
  uint8 i;

  for ( i = 0; i < 8; i++ ) {
    buf[8 + i] = i * buf[i];
    if ( i <= 5 ) {
      buf[i] ^= cccam[i];
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void cc_decrypt(struct cc_crypt_block *block, uint8 *data, int len)
{
  int i;
  uint8 z;

  for (i = 0; i < len; i++) {
    block->counter++;
    block->sum += block->keytable[block->counter];
    cc_crypt_swap(&block->keytable[block->counter], &block->keytable[block->sum]);
    z = data[i];
    data[i] = z ^ block->keytable[(block->keytable[block->counter] + block->keytable[block->sum]) & 0xff] ^ block->state;
    z = data[i];
    block->state = block->state ^ z;
  }
}

///////////////////////////////////////////////////////////////////////////////
void cc_encrypt(struct cc_crypt_block *block, uint8 *data, int len)
{
  int i;
  uint8 z;
  // There is a side-effect in this function:
  // If in & out pointer are the same, then state is xor'ed with modified input
  // (because output(=in ptr) is written before state xor)
  // This side-effect is used when initialising the encrypt state!
  for (i = 0; i < len; i++) {
    block->counter++;
    block->sum += block->keytable[block->counter];
    cc_crypt_swap(&block->keytable[block->counter], &block->keytable[block->sum]);
    z = data[i];
    data[i] = z ^ block->keytable[(block->keytable[block->counter] + block->keytable[block->sum]) & 0xff] ^ block->state;
    block->state = block->state ^ z;
  }
}

///////////////////////////////////////////////////////////////////////////////
// node_id : client nodeid, the sender of the ECM Request(big endian)
// card_id : local card_id for the server
void cc_crypt_cw(uint8 *nodeid/*client node id*/, uint32 card_id, uint8 *cws)
{
  uint8 tmp;
  int i,n;
  uint8 nod[8];

  for(i=0; i<8; i++) nod[i] = nodeid[7-i];
  for (i = 0; i < 16; i++) {
    if (i&1)
	if (i!=15) n = nod[i>>1]>>4 |  nod[(i>>1)+1]<<4; else n = nod[i>>1]>>4;
    else n = nod[i>>1];
    n = n & 0xff;
    tmp = cws[i] ^ n;
    if (i & 1) tmp = ~tmp;
    cws[i] = (card_id >> (2 * i)) ^ tmp;
    //printf("(%d) n=%02x, tmp=%02x, cw=%02x\n",i,n,tmp,cws[i]); 
  }
}
