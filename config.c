#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"

struct channel_info_data chninfo[1024];
int nbchninfo = 0;

char currentline[1024];
char *start, *end;

char *uppercase(char *str)
{
	int i;
	for(i=0;;i++) {
		switch(str[i]) {
			case 'a'...'z':
				str[i] = str[i] - ('a'-'A');
				break;
			case 0:
				return str;
		}
	}
}

int skip_spaces()
{
	while ( (*start==' ')||(*start=='\t') ) start++;
}

int read_str(char *str)
{
	int len;
	skip_spaces();
	end=start;
	while ( (*end!=' ')&&(*end!='\t')&&(*end!=13)&&(*end!=10)&&(*end!=':') ) end++;
	if ( (len=end-start)>0 ) {
		memcpy(str, start, len);
		str[len] = 0;
		start = end;
	}
	return len;
}

int read_name(char *str)
{
	int len;
	skip_spaces();
	if (*start=='"') {
		start++;
		end=start;
		while ( (*end!='"')&&(*end!='\t')&&(*end!=13)&&(*end!=10) ) end++;
		if ( (len=end-start)>0 ) memcpy(str, start, len);
		str[len] = 0;
		start = end;
		return len;
	}
	else {
		str[0]=0;
		return 0;
	}
}

int read_int(char *str)
{
	int len;
	skip_spaces();
	end=start;
	while ( (*end>='0')&&(*end<='9') ) end++;
	if ( (len=end-start)>0 ) memcpy(str, start, len);
	str[len] = 0;
	start = end;
	return len;
}

int read_hex(char *str)
{
	int len;
	skip_spaces();
	end=start;
	while ( ((*end>='0')&&(*end<='9'))||((*end>='A')&&(*end<='F'))||((*end>='a')&&(*end<='f')) ) end++;
	if ( (len=end-start)>0 ) {
		memcpy(str, start, len);
		str[len] = 0;
		start = end;
	}
	return len;
}

int read_chninfo(char *chninfofile)
{
	int len,i;
	char str[255];
	FILE *fhandle;
	int nbline = 0;

	// Open Config file
	fhandle = fopen(chninfofile,"rt");
	if (fhandle==0) {
		debug("file not found '%s'\n",chninfofile);
		return -1;
	} else debug("config: parsing file '%s'\n",chninfofile);

	// Init config data
	nbchninfo = 0;

	while (!feof(fhandle))
	{
		if ( !fgets(currentline, 1023, fhandle) ) break;
		nbline++;
		start = &currentline[0];
		skip_spaces();
		if ( (*start=='#')||(*start==0)||(*start==13)||(*start==10) ) continue;

		read_hex(&str[0]);
		chninfo[nbchninfo].caid = hex2int(&str[0]);
		skip_spaces();
		if (*start!=':') {
			debug("config(%d,%d): ':' expected\n",nbline,start-currentline);
			continue;
		} else start++;

		read_hex(&str[0]);
		chninfo[nbchninfo].prov = hex2int(&str[0]);
		skip_spaces();
		if (*start!=':') {
			debug("config(%d,%d): ':' expected\n",nbline,start-currentline);
			continue;
		} else start++;

		read_hex(&str[0]);
		chninfo[nbchninfo].sid = hex2int(&str[0]);
		skip_spaces();
		if (*start!=':') {
			debug("config(%d,%d): ':' expected\n",nbline,start-currentline);
			continue;
		} else start++;

		read_hex(&str[0]);
		chninfo[nbchninfo].deg = hex2int(&str[0]);
		skip_spaces();
		if (*start!=':') {
			debug("config(%d,%d): ':' expected\n",nbline,start-currentline);
			continue;
		} else start++;


		read_hex(&str[0]);
		chninfo[nbchninfo].freq = hex2int(&str[0]);

		read_name(&str[0]);
		str[63] = 0;
		strcpy(chninfo[nbchninfo].name, str);
//		debug("%s -> %04x:%06x:%04x\n",chninfo[nbchninfo].name, chninfo[nbchninfo].caid,chninfo[nbchninfo].prov,chninfo[nbchninfo].sid);
		nbchninfo++;
	}
	fclose(fhandle);
	return 0;
}

int getchannel(uint16 caid, uint32 prov, uint16 sid)
{
  int i;
  for(i=0; i<nbchninfo; i++)
    if ( (chninfo[i].caid==caid)&&(chninfo[i].prov==prov)&&(chninfo[i].sid==sid) ) return i;
  return -1;
}

int read_config(char *config_file, struct config_data *cfg09)
{
	int len,i;
	char str[64];
	FILE *fhandle;
	int nbline = 0;
	struct cs_client_data *usr;
	struct twin_server_data *srv;
	// Open Config file
	fhandle = fopen(config_file,"rt");
	if (fhandle==0) {
		debug("file not found '%s'\n",config_file);
		return -1;
	} else debug("config: parsing file '%s'\n",config_file);
	// Init config data
	memset( &cfg, 0, sizeof(struct config_data) );
	memset( cfg.cccam.version, 0, 32);
	memset( cfg.cccam.build, 0, 32);
	strcpy(cfg.cccam.version,"2.1.4");
	strcpy(cfg.cccam.build,"3191");
	cfg.cccam.cards_id = 0x64;
	cfg.cccam.cards = NULL;
	cfg.cccam.sats=NULL;
	cfg.cccam.clients = NULL;
	memcpy(cfg.cccam.nodeid,"nsk",8);
	cfg.cccam.handle = INVALID_SOCKET;
	cfg.cccam.port = 44000;
	cfg.cccam.cwdelay = 1000;
	cfg.twin_max_tries = 35;
	cfg.twin_try_delay = 200000;
	cfg.twin_time_out_nrw = 3000;
	
	debug("**************************************************************\n");
	while (!feof(fhandle))
	{
		if ( !fgets(currentline, 1023, fhandle) ) break;
		nbline++;
		start = &currentline[0];
		skip_spaces();
		if ( (*start=='#')||(*start==0)||(*start==13)||(*start==10) ) continue;
		len = read_str(&str[0]);
		uppercase(str);
		if (!strcmp(str,"SERIAL")) {
			read_str(&str[0]);
			uppercase(str);
			if (!strcmp(str,"DEVICE")) {
				skip_spaces();
				if (*start!=':') {
					printf("':' expected\n");
					continue;
				} else start++;
				read_str(&cfg.com_device[0]);
				cfg.com_handle=-1;
				debug("* serial device ....: %s\n",cfg.com_device);
			}
		}
		else if (!strcmp(str,"F")) {
			skip_spaces();
			if (*start!=':') {
				printf("':' expected\n");
				continue;
			} else start++;
			struct cc_client_data *cli = malloc( sizeof(struct cc_client_data) );
			memset(cli, 0, sizeof(struct cc_client_data) );
			read_str(cli->username);
			read_str(cli->password);
			cli->handle = -1;
			cli->next = cfg.cccam.clients;
			cfg.cccam.clients = cli;
                        debug("* cccamuser/pwd ....: %s/%s\n",cli->username,cli->password);
		}
		else if (!strcmp(str,"SAT")) {			
			struct cc_sat_info *sat = malloc( sizeof(struct cc_sat_info) );
			memset(sat,0, sizeof(struct cc_sat_info));
			skip_spaces();
			read_hex(&str[0]);
			sat->sat = hex2int(&str[0]);
			skip_spaces();
			if (*start!=':') {
				printf("':' expected\n");
				continue;
			}
			
			do {
				start++;
				if (!read_int(&str[0])) {
					debug(0,"config(%d,%d): null timeout...\n",nbline,start-currentline);
					continue;
				}
				sat->timeout = atoi(&str[0]);
								
				skip_spaces();
			} while (*start==',');
			
			cc_sat_add(sat);
			debug("* sat/timeout .......: %d/%d\n",sat->sat,sat->timeout);			
		}
		else if (!strcmp(str,"LISTEN")) {
			read_str(&str[0]);
			uppercase(str);
			if (!strcmp(str,"PORT")) {
				skip_spaces();
				if (*start!=':') {
					printf("':' expected\n");
					continue;
				} else start++;
				read_int(&str[0]);
				cfg.cccam.port = atoi(&str[0]);
				debug( "* listen port ......: %s \n", str );
			}
		}
		else if (!strcmp(str,"CCCAM")) {
			read_str(&str[0]);
			uppercase(str);
			if (!strcmp(str,"CWDELAY")) {
				skip_spaces();
				if (*start!=':') {
					printf("':' expected\n");
					continue;
				} else start++;
				read_int(&str[0]);
				cfg.cccam.cwdelay = atoi(&str[0]);
				debug( "* cccam cwdelay ....: %s\n", str );
			}
		}
		else if (!strcmp(str,"CARD")) {
			struct cc_card_data *card = malloc( sizeof(struct cc_card_data) );
			memset(card,0, sizeof(struct cc_card_data));
			skip_spaces();
			read_hex(&str[0]);
			card->caid = hex2int(&str[0]);
			skip_spaces();
			if (*start!=':') {
				printf("':' expected\n");
				continue;
			}
			card->provcount = 0;
			do {
				start++;
				if (!read_hex(&str[0])) {
					debug(0,"config(%d,%d): null provider...\n",nbline,start-currentline);
					continue;
				}
				card->prov[card->provcount].prov = hex2int(&str[0]);
				card->prov[card->provcount].ua = 0;
				card->provcount++;
				if (card->provcount>=16) {
					debug(0,"config(%d,%d): many providers...\n",nbline,start-currentline);
					continue;
				}
				skip_spaces();
			} while (*start==',');
			card->shareid = cfg.cccam.cards_id;
			card->localid = cfg.cccam.cards_id;
			cfg.cccam.cards_id++;
			memcpy( card->nodeid, cfg.cccam.nodeid, 8);
			card->uphops = 1;
			card->maxdown = 1;
			card->ecmnb=0;
			card->ecmok=0;
			card->provcount = cfg.card.nbprov;
			cc_card_add(card);
			debug("* share/caid .......: %02x/%04x\n",card->shareid,card->caid);			
		}
        	else if (!strcmp(str,"TWIN_TRY_DELAY")) {
			skip_spaces();
			if (*start!=':') {
				debug("config(%d,%d): ':' expected\n",nbline,start-currentline);
				continue;
			} else start++;
			read_int(&str[0]);
			skip_spaces();
			cfg.twin_try_delay = atol(&str[0]);
			if ( (cfg.twin_try_delay<0)||(cfg.twin_try_delay>10000000) ) cfg.twin_try_delay=150000;
			debug("* twin try delay ...: %d\n",cfg.twin_try_delay);
		}
		else if (!strcmp(str,"TWIN_MAX_TRIES")) {
			skip_spaces();
			if (*start!=':') {
				debug("config(%d,%d): ':' expected\n",nbline,start-currentline);
				continue;
			} else start++;
			read_int(&str[0]);
			skip_spaces();
			cfg.twin_max_tries = atoi(&str[0]);
			if ( (cfg.twin_max_tries<0)||(cfg.twin_max_tries>5000) ) cfg.twin_max_tries=70;
			debug("* twin max tries ...: %d\n",cfg.twin_max_tries);
		}
		else if (!strcmp(str,"TWIN_TIME_OUT_NRW")) {
			skip_spaces();
			if (*start!=':') {
				debug("config(%d,%d): ':' expected\n",nbline,start-currentline);
				continue;
			} else start++;
			read_int(&str[0]);
			skip_spaces();
			cfg.twin_time_out_nrw = atoi(&str[0]);
			if ( (cfg.twin_time_out_nrw<0)||(cfg.twin_time_out_nrw>20000) ) cfg.twin_time_out_nrw=4000;
			debug("* twin time out nrw : %d\n",cfg.twin_time_out_nrw);
		}
	
	}
	debug("**************************************************************\n");
	fclose(fhandle);
	return 0;
}

int reread_config(char *config_file, struct config_data *cfg90)
{
	int len,i;
	char str[64];
	FILE *fhandle;
	int nbline = 0;
	struct cs_client_data *usr;
	cfg.cccam.sats=NULL;

	// Open Config file
	fhandle = fopen(config_file,"rt");
	if (fhandle==0) {
		debug("file not found '%s'\n",config_file);
		return -1;
	} else debug("config: parsing file '%s'\n",config_file);

	while (!feof(fhandle))
	{
		if ( !fgets(currentline, 1023, fhandle) ) break;
		nbline++;
		start = &currentline[0];
		skip_spaces();
		if ( (*start=='#')||(*start==0)||(*start==13)||(*start==10) ) continue;
		len = read_str(&str[0]);
		uppercase(str);
		
		if (!strcmp(str,"CCCAM")) {
			read_str(&str[0]);
			uppercase(str);
			if (!strcmp(str,"CWDELAY")) {
				skip_spaces();
				if (*start!=':') {
					printf("':' expected\n");
					continue;
				} else start++;
				read_int(&str[0]);
				cfg.cccam.cwdelay = atoi(&str[0]);
				debug( "* cccam cwdelay ....: %s\n", str );
			}
		}	
		else if (!strcmp(str,"TWIN_TRY_DELAY")) {
			skip_spaces();
			if (*start!=':') {
				debug("config(%d,%d): ':' expected\n",nbline,start-currentline);
				continue;
			} else start++;
			read_int(&str[0]);
			skip_spaces();
			cfg.twin_try_delay = atol(&str[0]);
			if ( (cfg.twin_try_delay<0)||(cfg.twin_try_delay>10000000) ) cfg.twin_try_delay=150000;
			debug("* twin try delay ...: %d\n",cfg.twin_try_delay);
		}
		else if (!strcmp(str,"SAT")) {
			struct cc_sat_info *sat = malloc( sizeof(struct cc_sat_info) );
			memset(sat,0, sizeof(struct cc_sat_info));
			skip_spaces();
			read_hex(&str[0]);
			sat->sat = hex2int(&str[0]);
			skip_spaces();
			if (*start!=':') {
				printf("':' expected\n");
				continue;
			}
			
			do {
				start++;
				if (!read_int(&str[0])) {
					debug(0,"config(%d,%d): null timeout...\n",nbline,start-currentline);
					continue;
				}
				sat->timeout = atoi(&str[0]);
								
				skip_spaces();
			} while (*start==',');
			
			cc_sat_add(sat);
			debug("* sat/timeout .......: %d/%d\n",sat->sat,sat->timeout);			
		}
		
	}
	fclose(fhandle);

	return 0;
}

