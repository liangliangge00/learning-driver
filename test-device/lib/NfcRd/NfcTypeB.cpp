#define LOG_TAG "NfcTypeB"
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

#include "pn512_io.h"
#include "pn512_reg.h"
#include "NfcTypeB.h"

unsigned char PUPI[4];	


NfcTypeB::NfcTypeB(PN512_IO *dev)
{
	nfc_dev = dev;
}

/*****************************************************************************************/
/*名称：TypeB_Halt																		 */
/*功能：设置TYPE B卡片进入停止状态														 */
/*输入：card_sn：卡片的PUPI																 */
/*输出：																				 */
/*	   	TRUE：操作成功																	 */
/*		ERROR：操作失败																	 */
/*****************************************************************************************/
int NfcTypeB::Halt(unsigned char *card_sn)
{
	unsigned char send_buff[5],rece_buff[1];
	unsigned int  bitlen;
	int res;

	send_buff[0] = 0x50;
	memcpy(send_buff + 1, card_sn, 4);
// 	Pcd_SetTimer(10);
// 	ret = Pcd_Comm(Transceive, send_byte, 5, rece_byte, &bitlen);
	res = nfc_dev->transfer(Transceive, 10,
		send_buff, 5,
		rece_buff, 1, &bitlen);
	return res;
}

/*****************************************************************************************/
/*名称：TypeB_WUP																		 */
/*功能：TYPE B卡片唤醒																	 */
/*输入：N/A																				 */
/*输出：																				 */
/*		bitlen:卡片应答的数据长度；buff：卡片应答的数据指针							 */
/*		card_sn:卡片的PUPI字节															 */
/*	   	TRUE：操作成功																	 */
/*		ERROR：操作失败																	 */
/*****************************************************************************************/
int NfcTypeB::WakeUp(unsigned char *card_sn, unsigned char *buf, unsigned int *bitlen)
{						
	unsigned char  send_buff[3];
	int res;
	int i;
	send_buff[0]=0x05;//APf
	send_buff[1]=0x00;//AFI (00:for all cards)
	send_buff[2]=0x08;//PARAM(WUP,Number of slots =0)
// 	Pcd_SetTimer(10);
// 	ret=Pcd_Comm(Transceive,send_buff,3,buff,&(*bitlen));

	res = nfc_dev->transfer(Transceive, 10,
		send_buff, 3,
		buf, FIFO_MAX_SIZE, &(*bitlen));

	if (res > 0)
		memcpy(card_sn, &buf[1], 4);

	return res;
}

/*****************************************************************************************/
/*名称：TypeB_Request																	 */
/*功能：TYPE B卡片选卡																	 */
/*输入：																				 */
/*输出：																				 */
/*	   	TRUE：操作成功																	 */
/*		ERROR：操作失败																	 */
/*****************************************************************************************/
int NfcTypeB::Request(unsigned int *bitlen,unsigned char *buff,unsigned char *card_sn)
{			
	unsigned char send_buff[3];
	int res;
		
	send_buff[0]=0x05;	//APf
	send_buff[1]=0x00;	//AFI (00:for all cards)
	send_buff[2]=0x00;	//PARAM(REQB,Number of slots =0)

// 	Pcd_SetTimer(10);
// 	ret=Pcd_Comm(Transceive,send_buff,3,buff,bitlen);
	res = nfc_dev->transfer(Transceive, 10,
		send_buff, 3,
		buff, FIFO_MAX_SIZE, &(*bitlen));

	if (res > 0)
		memcpy(card_sn, buff + 1, 4);

	return res;
}	

/*****************************************************************************************/
/*名称：TypeB_Select																	 */
/*功能：TYPE B卡片选卡																	 */
/*输入：card_sn：卡片PUPI字节（4字节）													 */
/*输出：																				 */
/*		bitlen：应答数据的长度														 */
/*		buff：应答数据的指针															 */
/*	   	TRUE：操作成功																	 */
/*		ERROR：操作失败																	 */
/*****************************************************************************************/
int NfcTypeB::Select(unsigned char *card_sn,unsigned int *bitlen,unsigned char *buff)
{
	unsigned char send_buff[9];
	int res;
	
	send_buff[0] = 0x1d;
	memcpy(send_buff + 1, card_sn, 4);
	send_buff[5] = 0x00;  //------Param1
	send_buff[6] = 0x08;  //01--24，08--256------Param2
	send_buff[7] = 0x01;  //COMPATIBLE WITH 14443-4------Param3
	send_buff[8] = 0x02;  //CID：01 ------Param4
    
// 	Pcd_SetTimer(20);
// 	ret=Pcd_Comm(Transceive,send_buff,9,buff,&(*bitlen));
	res = nfc_dev->transfer(Transceive, 20,
		send_buff, 9,
		buff, FIFO_MAX_SIZE, &(*bitlen));

	return res;
}	

/*****************************************************************************************/
/*名称：TypeB_GetUID																	 */
/*功能：身份证专用指令																	 */
/*输入：N/A																				 */
/*输出：bitlen：返回数据的长度														 */
/*		buff：返回数据的指针															 */
/*	   	TRUE：操作成功																	 */
/*		ERROR：操作失败																	 */
/*****************************************************************************************/
int NfcTypeB::GetUID(unsigned int *bitlen,unsigned char *buff)
{
	unsigned char send_buff[5];
	int res;
		
	send_buff[0]=0x00;
	send_buff[1]=0x36;
	send_buff[2]=0x00;
	send_buff[3]=0x00;
	send_buff[4]=0x08;

// 	Pcd_SetTimer(15);
// 	ret=Pcd_Comm(Transceive,send_buff,5,buff,&(*bitlen));
	res = nfc_dev->transfer(Transceive, 15,
		send_buff, 5,
		buff, FIFO_MAX_SIZE, &(*bitlen));

	return res;
}	
