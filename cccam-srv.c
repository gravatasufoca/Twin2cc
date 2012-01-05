#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <termios.h>

#include "global.h"

struct cccam_server cccam;

///////////////////////////////////////////////////////////////////////////////
// CARDS FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

void cc_card_add(struct cc_card_data *card)
{
  // Add to DB
	card->prev = NULL;
	card->next = cfg.cccam.cards;
	if (cfg.cccam.cards) cfg.cccam.cards->prev = card;
	cfg.cccam.cards = card;
}

void cc_sat_add(struct cc_sat_info *sat)
{
  // Add to DB
	sat->prev = NULL;
	sat->next = cfg.cccam.sats;
	if (cfg.cccam.sats) cfg.cccam.sats->prev = sat;
	cfg.cccam.sats = sat;
}

struct cc_card_data *getcardbylocalid(uint32 localid)
{
	struct cc_card_data *card = cfg.cccam.cards;
	while (card) {
		if (card->localid==localid) return card;
		card = card->next;
	}
	return NULL;
}

struct cc_sat_info *getsatinfo(int channelsat)
{
	struct cc_sat_info *sat = cfg.cccam.sats;
	while (sat) {
		if (sat->sat==channelsat) return sat;
		sat = sat->next;
	}
	return NULL;
}

struct cc_card_data *getcardbycaidprov(uint16 caid, uint32 prov)
{
	struct cc_card_data *card = cfg.cccam.cards;
	int i;

	while (card) {
		if (card->caid==caid)
			for(i=0;i<card->provcount;i++) if (card->prov[i].prov==prov) return card;
		card = card->next;
	}
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// SERVER MODE
///////////////////////////////////////////////////////////////////////////////

static unsigned int seed;
static uint8 fast_rnd()
{
  unsigned int offset = 12923;
  unsigned int multiplier = 4079;

  seed = seed * multiplier + offset;
  return (uint8)(seed % 0xFF);
}

static int cc_sendinfo_cli(struct cc_client_data *cli)
{
  uint8 buf[CC_MAXMSGSIZE];
  memset(buf, 0, CC_MAXMSGSIZE);
  memcpy(buf, cfg.cccam.nodeid, 8 );
  memcpy(buf + 8, cfg.cccam.version, 32);		// cccam version (ascii)
  memcpy(buf + 40, cfg.cccam.build, 32);       // build number (ascii)
  //debugdump(cfg.cccam.nodeid,8,"Sending server data version: %s, build: %s nodeid ", cfg.cccam.version, cfg.cccam.build);
  return cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_SRV_INFO, 0x48, buf);
}

///////////////////////////////////////////////////////////////////////////////
int cc_sendcard_cli(struct cc_card_data *card, struct cc_client_data *cli)
{
	int j;
	static uint8 buf[CC_MAXMSGSIZE];

	memset(buf, 0, sizeof(buf));
	buf[0] = card->localid >> 24;
	buf[1] = card->localid >> 16;
	buf[2] = card->localid >> 8;
	buf[3] = card->localid & 0xff;
	buf[4] = card->shareid >> 24;
	buf[5] = card->shareid >> 16;
	buf[6] = card->shareid >> 8;
	buf[7] = card->shareid & 0xff;
	buf[8] = card->caid >> 8;
	buf[9] = card->caid & 0xff;
	buf[10] = card->uphops-1;
	buf[11] = 0;//card->maxdown-1;
	buf[20] = card->provcount;
	for (j=0; j<card->provcount; j++) {
		//memcpy(buf + 21 + (j*7), card->provs[j], 7);
		buf[21+j*7] = 0xff&(card->prov[j].prov>>16);
		buf[22+j*7] = 0xff&(card->prov[j].prov>>8);
		buf[23+j*7] = 0xff&(card->prov[j].prov);
		buf[24+j*7] = 0xff&(card->prov[j].ua>>24);
		buf[25+j*7] = 0xff&(card->prov[j].ua>>16);
		buf[26+j*7] = 0xff&(card->prov[j].ua>>8);
		buf[27+j*7] = 0xff&(card->prov[j].ua);
	}
	buf[21 + (card->provcount*7)] = 1;
	memcpy(buf + 22 + (j*7), card->nodeid, 8);
	cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_CARD_ADD, 30 + (j*7), buf);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
void cc_sendcards_cli(struct cc_client_data *cli)
{
	int nbcard=0;
	struct cc_card_data *card;
	card = cfg.cccam.cards;
	while (card) {
		if (cc_sendcard_cli(card, cli)) nbcard++;
		card = card->next;
	}
	debug("%d cards --> client(%s)\n",  nbcard, cli->username);
}


cc_disconnect_cli(struct cc_client_data *cli)
{
	cli->connected = 0;
	usleep(10000);
	debug("client '%s' disconnected \n", cli->username);
	close(cli->handle);
	cli->handle = -1;
}


struct struct_clicon {
	int sock;
	uint32 ip;
};

///////////////////////////////////////////////////////////////////////////////
void *cc_connect_cli(struct struct_clicon *param)
{
	static uint8 buf[300];
	uint8 data[16];
	int i;
	struct cc_crypt_block sendblock;	// crypto state block
	struct cc_crypt_block recvblock;	// crypto state block
	char usr[20];
	char pwd[20];

	int sock = param->sock;
	uint32 ip = param->ip;
	free(param);

	memset(usr, 0, sizeof(usr));
	memset(pwd, 0, sizeof(pwd));
  // calc + send random seed
	uint32 seed = (unsigned int) time((time_t*)0);
	for(i=0; i<16; i++ ) data[i]=fast_rnd();
	send_nonb(sock, data, 16, 100);

	cc_crypt_xor(data);  // XOR init bytes with 'CCcam'

	SHA_CTX ctx;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, data, 16);
	SHA1_Final(buf, &ctx);

	//initialisate crypto states
	cc_crypt_init(&sendblock, buf, 20);
	cc_decrypt(&sendblock, data, 16);
	cc_crypt_init(&recvblock, data, 16);
	cc_decrypt(&recvblock, buf, 20);
	//debugdump(buf, 20, "SHA1 hash:");
	memcpy(usr,buf,20);
	if ((i=recv_nonb(sock, buf, 20,1000)) == 20) {
		cc_decrypt(&recvblock, buf, 20);
		//debugdump(buf, 20, "Recv SHA1 hash:");
		if ( memcmp(buf,usr,20)!=0 ) {
			debug("wrong sha1 hash from client! (%s)\n",ip2string(ip));
			close(sock);
			return NULL;
		}
	} else {
		close(sock);
		return NULL;
	}

  // receive username
	if ((i=recv_nonb(sock, buf, 20,3000)) == 20) {
		cc_decrypt(&recvblock, buf, i);
		strcpy(usr, buf);
		//debug("Server: username '%s'\n", usr);
	} else {
		close(sock);
		return NULL;
	}

  // Check for username
	int found = 0;
	struct cc_client_data *cli;
	cli = cfg.cccam.clients;
	while (cli) {
		if (!strcmp(cli->username,usr)) {
			if (cli->connected) {
				if (cli->ip==ip) {
					debug("user '%s' already connected\n", usr);
					cc_disconnect_cli(cli);
				}
				else {
					debug("user '%s' already connected with different ip!!!\n", usr);
					usleep(3000000);
					cc_disconnect_cli(cli);
				}
			}
			found = 1;
			break;
		}
		cli = cli->next;
	}
	if (!found) {
		debug("unknown user '%s'\n", usr);
		close(sock);
		return NULL;
	}

  // receive passwd / 'CCcam'
	strcpy( pwd, cli->password);
	cc_decrypt(&recvblock, pwd, strlen(pwd));
	if ((i=recv_nonb(sock, buf, 6,1000)) == 6) {
		cc_decrypt(&recvblock, buf, 6);
		if (memcmp( buf+1, "Ccam\0",5)) {
			debug("login failed to client '%s'\n", usr);
			close(sock);
			return NULL;
		}
	} 
	else {
		close(sock);
		return NULL;
	}

  // send passwd ack
	memset(buf, 0, 20);
	memcpy(buf, "CCcam\0", 6);
	//debug("Server: send ack '%s'\n",buf);
	cc_encrypt(&sendblock, buf, 20);
	send_nonb(sock, buf, 20, 100);

  // Setup Client Data
	cli->handle = sock;
	cli->ip = ip;
	cli->ecmnb=0;
	cli->ecmok=0;
	memcpy(&cli->sendblock,&sendblock,sizeof(sendblock));
	memcpy(&cli->recvblock,&recvblock,sizeof(recvblock));
	debug("client '%s' connected\n", usr);

  // recv cli data
	memset(buf, 0, sizeof(buf));
	i = cc_msg_recv( cli->handle, &cli->recvblock, buf);
	if (i!=97 && i!=98) {
		debug("error recv cli data\n");
		close(sock);
		return NULL;
	}
	memcpy( cli->nodeid, buf+24, 8);
	memcpy( cli->version, buf+33, 32);
	memcpy( cli->build, buf+65, 32 );
	debugdump(cli->nodeid,8,"client '%s' running version %s build %s nodeid ", usr, cli->version, cli->build);

  // send cli data ack
	cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_CLI_INFO, 0, NULL);
	cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_BAD_ECM, 0, NULL);
	cc_sendinfo_cli(cli);
	cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_BAD_ECM, 0, NULL);

	cli->cardsent = 0;
	cli->connected = getseconds();
	cli->reqecm = 0;
	return cli;
}


void *cc_connect_cli_thread(void *param)
{
  SOCKET client_sock =-1;
  struct sockaddr_in client_addr;
  socklen_t socklen = sizeof(client_addr);

	pthread_t srv_tid;

  while(1)
  {
    if ( fdstatus_accept(cfg.cccam.handle)==1 )
    {
      client_sock = accept( cfg.cccam.handle, (struct sockaddr*)&client_addr, /*(socklen_t*)*/&socklen);
      if ( client_sock==INVALID_SOCKET ) {
        debug("Accept Error\n");
        break;
      }
      else {
			struct struct_clicon *clicondata = malloc( sizeof(struct struct_clicon) );
			clicondata->sock = client_sock; 
			clicondata->ip = client_addr.sin_addr.s_addr;
			int ret = create_prio_thread(&srv_tid, cc_connect_cli,clicondata, 20);
		}
    }
  }// While
}

pthread_t cc_cli_tid;
int init_cccam()
{
	//Bind Server listen port
	if ( (cfg.cccam.handle=CreateServerSockTcp_nonb(cfg.cccam.port)) == -1) {
		debug("socket: bind port failed\n");
		return -1;
	}
	create_prio_thread(&cc_cli_tid, cc_connect_cli_thread, NULL, 50); // Lock server

	debug("CCcam server started on port %d\n",cfg.cccam.port);
	return 0;
}

done_cccam()
{
  if (close(cfg.cccam.handle)) debug("Close failed(%d)",cfg.cccam.handle);
}
