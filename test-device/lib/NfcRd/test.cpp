

#define LOG_TAG "NfcRd"
//#define LOG_NDDEBUG 0

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <cassert>
#include <cstdlib>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>

#include "pn512.h"
#include "yl0301.h"

void ID_SwipingCallback(NfcDev *dev, unsigned char* id, int len, int type)
{
	printf("Got ID card:%02X%02X%02X%02X\n", id[0], id[1], id[2], id[3]);
}

void IC_SwipingCallback(NfcDev *dev, unsigned char* id, int len, int type)
{
	printf("Got IC card:%02X%02X%02X%02X\n", id[0], id[1], id[2], id[3]);
}

int main(int argc, char *argv[])
{
	unsigned char  picc_atqa[2], picc_uid[15], picc_sak[3];
	unsigned char  DefualtKey[6] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
	unsigned char  DefualtData[16] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16 };
	unsigned char  DefualtData2[16] = { 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xa0,0xb1,0xc2,0xd3,0xe4,0xf5,0x06 };
	unsigned char  read_data[FIFO_MAX_SIZE];
	unsigned char  Send_Buff[FIFO_MAX_SIZE];
	unsigned char  PUPI[4];
	int ret,i;

	unsigned int Rec_len;  

	PN512 *p_ic;
	YL0301 *p_id;

	if(strstr(argv[1], "PN512") != NULL) {
		//printf("PN512 START !\n");
		p_ic = new PN512();
		//获取ID
		p_ic->swiping_event = IC_SwipingCallback;
		p_ic->start();
		while(1){sleep(10);}
	
	}	
	else if(strstr(argv[1], "YL0301") != NULL) {
		//printf("YL0301 START !\n");
		p_id = new YL0301();
		
		while(1){sleep(10);}
	}
	else {
		
		p_ic = new PN512();
		p_id = new YL0301();

		p_ic->swiping_event = IC_SwipingCallback;
		p_id->swiping_event = ID_SwipingCallback;

		p_ic->start();
		p_id->start();

		while(1){sleep(10);}
	}
}
