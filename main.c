#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <termios.h>

#include "global.h"
#include <unistd.h>

int END_PROCESS =0;
struct config_data cfg;

char config_file[256] = "/etc/twin/twin2cc.cfg";
char config_channelinfo[256]="/etc/twin/twin2cc.channelinfo";
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32 ecm_crc( uchar *ecm, int ecmlen)
{
  uchar checksum[4];
  int counter;

  checksum[3]= ecm[0];
  checksum[2]= ecm[1];
  checksum[1]= ecm[2];
  checksum[0]= ecm[3];
  for ( counter=1; counter< (ecmlen/4) - 4; counter++)
  {
    checksum[3] ^=ecm[counter*4];
    checksum[2] ^=ecm[counter*4+1];
    checksum[1] ^=ecm[counter*4+2];
    checksum[0] ^=ecm[counter*4+3];
  }
  return ( (checksum[3]<<24) | (checksum[2]<<16) | (checksum[1]<<8) | checksum[0] );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int twin_init()
{
	// Set up Serial Port
	//if (serial_init(cfg.com_device,B115200 | CS8 | CLOCAL | CSTOPB | CREAD,&cfg.com_handle)==-1) {
	if (serial_init(cfg.com_device,B115200 | CS8 | CLOCAL | CREAD,&cfg.com_handle)==-1) {
		debug("001-Error initializing serial device '%s'\n", cfg.com_device);
		return -1;
	} else serial_purge(cfg.com_handle);
	debug("002-Serial port '%s' opened. With handle: %d\n",cfg.com_device,cfg.com_handle);
	return 0;
}

int twin_done()
{
  serial_done(cfg.com_handle);
}

int isnullcw(unsigned char *cw)
{
	int i;
	for(i=0; i<16; i++){
	  if (cw[i]!=0) return 0;
	
	}
	return 1;
}


uint8 twinlastcw[16];
int twinlastchn = -1;

int twin_send(int chn, uint8* cw, unsigned char oldcw[17])
{
	uint8 wbuf[32]; // write
	uint8 rbuf[32]; // read
	int rlen;
	unsigned char fcw00[8];
	memset(fcw00, 0, 8);
	unsigned char fcw55[8];
	memset(fcw55, 0x55, 8);
	unsigned char fcw11[8];
	memset(fcw11, 0x11, 8);
	int error_tries=0;
	wbuf[0] = 7;
	wbuf[1] = 6;
	wbuf[2] = chninfo[chn].deg>>8;
	wbuf[3] = chninfo[chn].deg&0xff;
	wbuf[4] = chninfo[chn].freq>>8;
	wbuf[5] = chninfo[chn].freq&0xff;
	wbuf[6] = chninfo[chn].sid>>8;
	wbuf[7] = chninfo[chn].sid&0xff;
	wbuf[8] = wbuf[0]^wbuf[1]^wbuf[2]^wbuf[3]^wbuf[4]^wbuf[5]^wbuf[6]^wbuf[7];
	unsigned int keyfound = 0;
	

		serial_purge(cfg.com_handle);
		serial_write(cfg.com_handle, wbuf, 9);
		memset(rbuf,0,19);
		rlen = serial_readt(cfg.com_handle, 100, 50, 19, rbuf);

		if(!isnullcw(rbuf+3)){ 
		    if ( (rlen==19)&&(rbuf[0]==0xF7)&&(rbuf[1]==0x00)&&(rbuf[2]==0x16)&&(memcmp(oldcw,&rbuf[3],16)!=0)&&(memcmp(rbuf+11, fcw55,8)!=0)&&(memcmp(rbuf+11,fcw11,8)!=0) )
		    {	if ( (memcmp(rbuf+11,fcw00,8)!=0) || ((memcmp(rbuf+11,fcw00,8)==0)&&(memcmp(rbuf+3,wbuf,9)!=0)) )
			    {	
				    keyfound = 1;													      
				    
			    }
			    
		    }
		}

	if ( !keyfound ){		
		return 0;
	}

	memcpy(cw, rbuf+3, 16);
	
// 	debug("003-Get_Key_Twin: Data Received: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
// 	   rbuf[0],rbuf[1],rbuf[2],rbuf[3],rbuf[4],rbuf[5],rbuf[6],rbuf[7],rbuf[8],rbuf[9],
// 	   rbuf[10],rbuf[11],rbuf[12],rbuf[13],rbuf[14],rbuf[15],rbuf[16],rbuf[17],rbuf[18]);
	
	return 16;
}

int twin_send_ecm( int chn, uint8* cw, unsigned char oldcw[17])
{
    return twin_send(chn, cw, oldcw);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


struct clicon {
	int sock;
	uint32 ip;
	pthread_t tid;
};


////////////////////////////////////////////////////////////////////////////////
// CCCAM
////////////////////////////////////////////////////////////////////////////////

// Receive the key all the time
void *thr_getKey(void *param)
{
  
  struct cc_client_data *cli;
  
  uint8 cw[16];

 
  while(1){
	 cli = cfg.cccam.clients;
	 while(cli){
	    if (cli->connected){
	      
		if(twin_send_ecm( cli->reqchn, cw, cli->cw )==16){
		 
    // 		  if(memcmp(cw,cli->cw,8)){
			  //different key
			  if(!isnullcw(cw)){
			    memcpy(cli->cw,cw,16);
			    debugdump(cli->cw,8,"GET KEY: new key -> '%s' ch \"%s CW: ", cli->username, chninfo[cli->reqchn].name);
			  }
    // 		  }
		}
	    }
	    usleep(cfg.twin_try_delay);
	    cli = cli->next;
	 }
    	
   }

}


// Receive messages from clients
void cc_recvmsg_cli()
{
  unsigned char buf[1024];
//unsigned char oldcw[17];
  struct cc_card_data *card;
  struct cc_client_data *cli;
  struct cc_ecm_data *ecm;
  uint8 cw[16];
// 	printf("%d\n",cfg.cccam.clients->reqchn);
	cli = cfg.cccam.clients;
	while (cli)
	{	
		if (cli->connected){
		 
 		  if (fdstatus_read(cli->handle)>0)
 		  {
			  int len = cc_msg_recv( cli->handle, &cli->recvblock, buf);
			  if (len<=0) cc_disconnect_cli(cli);
			  else {
				  switch (buf[1]) {
				    case CC_MSG_ECM_REQUEST:
					  debug("CC_MSG_ECM_REQUEST\n");
					 
					  //Check for card availability
					  card = getcardbylocalid( buf[10]<<24 | buf[11]<<16 | buf[12]<<8 | buf[13] );
					  if (!card) {
						  debug("004-<- ecm from client '%s', card-id %x not found\n",cli->username, buf[10]<<24 | buf[11]<<16 | buf[12]<<8 | buf[13]);
						  cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_ECM_NOK2, 0, NULL);
					  }
					  else {
						
						  uint16 caid = buf[4]<<8 | buf[5];
						  uint prov = buf[6]<<24 | buf[7]<<16 | buf[8]<<8 | buf[9];
						  uint16 sid = buf[14]<<8 | buf[15];
						  int chn = getchannel(caid, prov, sid);
  
						  if (chn==-1) {
							  debug("005-<|> ecm from client '%s', ch %04x:%06x:%04x not found!!!\n", cli->username, caid, prov, sid);
							  cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_ECM_NOK1, 0, NULL);
						  }
						  else {  								  
								  debug("007-<- ecm from client '%s' ch \"%s\"\n", cli->username, chninfo[chn].name);
// 								  memcpy(cli->cw,cw,16);

								  struct cc_sat_info *sat;
								  sat=getsatinfo(chninfo[chn].deg);
								  
								  int timeout=sat->timeout;
								  if(chn!=cli->reqchn){								    
								    cli->reqchn = chn;
								    twin_send_ecm( chn, cli->cw, cli->oldcw );
								    timeout=0;
 								    usleep(100000);
								  }
								  
// 								  cli->reqecm = 1;
								  cli->reqchn = chn;
								  cli->reqcardid = buf[10]<<24 | buf[11]<<16 | buf[12]<<8 | buf[13];
								 
// 								  cli->reqrecvtime = GetTickCount(); // ecm request recvtime
								  
								  if(!isnullcw(cli->cw)){
								      
								      usleep(timeout);
								      memcpy( cli->oldcw, cli->cw, 16);							
								      debugdump(cli->cw,8,"015--> cw to client '%s' ch \"%s CW: ", cli->username, chninfo[chn].name);
								      cc_crypt_cw( cli->nodeid, cli->reqcardid , cli->cw);;
								      cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_ECM_REQUEST, 16, cli->cw);
								      cc_encrypt(&cli->sendblock, cli->cw, 16); // additional crypto step								     
// 								      cli->reqecm = 0;
								      
								  }else{								  								   
								      cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_ECM_NOK2, 0, NULL);						  
								      debug("018-<e3> decode failed to client '%s' ch \"%s\"\n", cli->username, chninfo[chn].name);						
								  }
								   								
								  
						  }
					  }

					  break;

				    case CC_MSG_KEEPALIVE:  
					  cc_msg_send( cli->handle, &cli->sendblock, CC_MSG_KEEPALIVE, 0, NULL);
					  debug("008-Keepalive from client '%s'\n",cli->username);
					  break;
				    case CC_MSG_BAD_ECM:					
					   cli->reqtimeout = GetTickCount()+200;
					   cli->reqmean=0;
					   cli->reqrecvtime=0;
// 					   cli->reqecm = 1;
					  debug("009-cmd 0x05 ACK from client '%s'\n",cli->username);
					  if (cli->cardsent==0) {
						  cc_sendcards_cli(cli);
						  cli->cardsent=1;
					  }

					  break;
				    default:
				   
				      debug("010-Unknown Packet ID : %02x from client '%s'\n", buf[1],cli->username);				  
				  }
			  }
		  }
		 // cli->reqecm = 1;
		}
		cli = cli->next;
		
	}
}


#include <sys/stat.h>
void *reread_config_thread(void *param)
{
	struct stat config_stat;
	time_t config_mtime;

	// Set UP Modification Time of Config file
	stat( config_file, &config_stat);
	config_mtime = config_stat.st_mtime;

	// Connect Clients
	while (!END_PROCESS) {
		usleep(10000000);
		stat( config_file, &config_stat);
		if (config_mtime != config_stat.st_mtime) {
			debug("019-Config file Changed...\n");
			config_mtime = config_stat.st_mtime;
			// ADD CLIENTS and new configurations
			reread_config(config_file, &cfg);
		}
    }

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

pthread_t cli_tid;
pthread_t cfg_tid;
pthread_t read_key;

int mainprocess()
{
  read_config(config_file, &cfg);
  read_chninfo(config_channelinfo);

	if (twin_init()==-1) {
		return -1;
	}

  init_cccam();

//   if ( (cfg.handle=CreateServerSockTcp_nonb(cfg.port)) == -1) {
//     debug("020-Socket failed\n");
//     return -1;
//   }

  create_prio_thread(&cfg_tid, reread_config_thread,NULL,10);
  usleep(100000);
  
  //read the keys all the time
  create_prio_thread(&read_key, thr_getKey,NULL,10);
  usleep(100000);
  
  debug("021-entering main loop...\n");
  while(1)
  {
	
	cc_recvmsg_cli();
// 	cc_checkecmrequest(); // send request to dongle after timeout
	usleep(10000);
  }
  twin_done();
  close(cfg.handle);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int  option_background = 0; // default
int  fork_return;

int main(int argc, char *argv[])
{
  char *args;

  debug("twin2cc 1.03a-by gravatasufoca inspired in basssem,dvbcrypt@gmail.com & theOthersTeam :)\n");
  debug("AzboxHD Enigma2\n");

  if (argc>1) {
    args = *(argv+1);
//    debug("%s\n", args);
    if (args[0]=='-')
      if (args[1]=='b') option_background = 1;
  }

  if (option_background==1)
  {
    fork_return = fork();
    if( fork_return < 0)
    {
      debug("022-Unable to create child process, exiting.\n");
      return -1;
    }
    if (fork_return>0)
    {
      //debug("main process, exiting.\n");
      return 0;
    }
    //else mainprocess();
  }
  mainprocess();
  debug("023-main process, exiting.\n");
}


