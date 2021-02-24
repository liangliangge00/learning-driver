/*************************************************************/
//2014.03.06修改版
/*************************************************************/

#define LOG_TAG "NfcTypeA"
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
#include "NfcTypeA.h"

NfcTypeA::NfcTypeA(PN512_IO *dev)
{
	nfc_dev = dev;
}

//ATQA是只有两个字节，UID却是4个字节加一个BCC校验值所以针对7字节UID防碰撞时就需要最多15字节的UID来存放
//SAK是一个字节的SAK+CRC_A(2字节)
/****************************************************************************************/
/*名称：TypeA_Request																	*/
/*功能：TypeA_Request卡片寻卡															*/
/*输入：																				*/
/*       			    			     												*/
/*	       								 												*/
/*输出：																			 	*/
/*	       	pTagType[0],pTagType[1] =ATQA                                         		*/					
/*       	TRUE: 应答正确                                                              	*/
/*	 		FALSE: 应答错误																*/
/****************************************************************************************/
int NfcTypeA::Request(unsigned char *pTagType)
{
	unsigned char  send_buff[1],rece_buff[2];
	unsigned int  rece_bitlen;  
	int res;

	nfc_dev->reg_clearbit(TxModeReg,0x80);//关闭TX CRC
	nfc_dev->reg_clearbit(RxModeReg,0x80);//关闭RX CRC
	nfc_dev->reg_setbit(RxModeReg, 0x08);//关闭位接收
	nfc_dev->reg_clearbit(Status2Reg,0x08);
	nfc_dev->reg_write(BitFramingReg,0x07);
	
   	send_buff[0] = 0x26;

	res = nfc_dev->transfer(Transceive, 1,
		send_buff, 1,
		rece_buff, 2, &rece_bitlen);

	//ALOGD("%s: %d, %d %d\n", __func__, rece_bitlen, rece_buff[0], rece_buff[1]);
	
	if ((res > 0) && (rece_bitlen == 2 * 8))
	{
		*pTagType = rece_buff[0];
		*(pTagType + 1) = rece_buff[1];
	}
	else
		res = -1;

	return res;
}
/****************************************************************************************/
/*名称：TypeA_WakeUp																	*/
/*功能：TypeA_WakeUp卡片寻卡															*/
/*输入：																				*/
/*       			    			     												*/
/*	       								 												*/
/*输出：																			 	*/
/*	       	pTagType[0],pTagType[1] =ATQA                                         		*/					
/*       	TRUE: 应答正确                                                              	*/
/*	 		FALSE: 应答错误																*/
/****************************************************************************************/
int NfcTypeA::WakeUp(unsigned char *pTagType)
{
	unsigned char  send_buff[1],rece_buff[2];
	unsigned int   rece_bitlen;  
	int res;

	nfc_dev->reg_clearbit(TxModeReg,0x80);//关闭TX CRC
	nfc_dev->reg_clearbit(RxModeReg,0x80);//关闭RX CRC
	nfc_dev->reg_setbit(RxModeReg, 0x08);//不接收小于4bit的数据
	nfc_dev->reg_clearbit(Status2Reg,0x08);
	nfc_dev->reg_write(BitFramingReg,0x07);

 	send_buff[0] = 0x52;

	nfc_dev->fifo_clear();
	res = nfc_dev->transfer(Transceive, 1,
		send_buff, 1,
		rece_buff, 2, &rece_bitlen);

	if ((res > 0) && (rece_bitlen == 2*8))
		memcpy(pTagType, rece_buff, 2);
	else
		res = -1;
	
	return res;
}
/****************************************************************************************/
/*名称：TypeA_Anticollision																*/
/*功能：TypeA_Anticollision卡片防冲突													*/
/*输入：selcode =0x93，0x95，0x97														*/
/*       			    			     												*/
/*	       								 												*/
/*输出：																			 	*/
/*	       	pSnr[0],pSnr[1],pSnr[2],pSnr[3]pSnr[4] =UID                            		*/					
/*       	TRUE: 应答正确                                                              	*/
/*	 		FALSE: 应答错误																*/
/****************************************************************************************/
int NfcTypeA::Anticollision(unsigned char selcode, unsigned char *pSnr)
{
    unsigned char i,send_buff[2],rece_buff[5];
    unsigned int rece_bitlen;
	int res;

	nfc_dev->reg_clearbit(TxModeReg,0x80);
	nfc_dev->reg_clearbit(RxModeReg,0x80);
    nfc_dev->reg_clearbit(Status2Reg,0x08);
    nfc_dev->reg_write(BitFramingReg,0x00);
    nfc_dev->reg_clearbit(CollReg,0x80);
 
    send_buff[0] = selcode;
    send_buff[1] = 0x20;             //NVB
    
	nfc_dev->fifo_clear();
	res = nfc_dev->transfer(Transceive, 1,
		send_buff, 2,
		rece_buff, 5, &rece_bitlen);

    if (res == 5)
    {
    	  memcpy(pSnr, rece_buff, 5);

		  //UID0、UID1、UID2、UID3、BCC(校验字节)
         if (pSnr[4] != (pSnr[0]^pSnr[1]^pSnr[2]^pSnr[3]))     
    		res = -1;
    }
	else
		res = -1;

	return res;
}

/****************************************************************************************/
/*名称：TypeA_Select																	*/
/*功能：TypeA_Select卡片选卡															*/
/*输入：selcode =0x93，0x95，0x97														*/
/*      pSnr[0],pSnr[1],pSnr[2],pSnr[3]pSnr[4] =UID 			    			     	*/
/*	       								 												*/
/*输出：																			 	*/
/*	       	pSak[0],pSak[1],pSak[2] =SAK			                            		*/					
/*       	TRUE: 应答正确                                                              	*/
/*	 		FALSE: 应答错误																*/
/****************************************************************************************/
int NfcTypeA::Select(unsigned char selcode,unsigned char *pSnr,unsigned char *pSak)
{
    unsigned char i,send_buff[7],rece_buff[5];
    unsigned int  rece_bitlen;
	int res;

	nfc_dev->reg_write(BitFramingReg,0x00);
  	nfc_dev->reg_setbit(TxModeReg,0x80);		//打开TX CRC
	nfc_dev->reg_setbit(RxModeReg,0x80);        //打开接收RX 的CRC校验
    nfc_dev->reg_clearbit(Status2Reg,0x08);
	
	send_buff[0] = selcode;			 //SEL
    send_buff[1] = 0x70;			 //NVB
    memcpy(send_buff + 2, pSnr, 5);
   	
	nfc_dev->fifo_clear();
	res = nfc_dev->transfer(Transceive, 1,
		send_buff, 7,
		rece_buff, 5, &rece_bitlen);
    
    if (res > 0)
		*pSak=rece_buff[0]; 
	else
		res = -1;
	 	
    return res;
}

/****************************************************************************************/
/*名称：TypeA_Halt																		*/
/*功能：TypeA_Halt卡片停止																*/
/*输入：																				*/
/*       			    			     												*/
/*	       								 												*/
/*输出：																			 	*/
/*	       											                            		*/					
/*       	TRUE: 应答正确                                                              	*/
/*	 		FALSE: 应答错误																*/
/****************************************************************************************/
int NfcTypeA::Halt(void)
{
    unsigned char  send_buff[2],rece_buff[1];
	unsigned int   rece_bitlen;
	int res;

    nfc_dev->reg_write(BitFramingReg,0x00);
  	nfc_dev->reg_setbit(TxModeReg,0x80);
	nfc_dev->reg_setbit(RxModeReg,0x80);
    nfc_dev->reg_clearbit(Status2Reg,0x08);

	send_buff[0] = 0x50;
	send_buff[1] = 0x00;

	nfc_dev->fifo_clear();
	res = nfc_dev->transfer(Transmit, 1,
		send_buff, 2,
		rece_buff, 1, &rece_bitlen);
    return res;
}

/****************************************************************************************/
/*名称：TypeA_CardActive																*/
/*功能：TypeA_CardActive卡片激活														*/
/*输入：																				*/
/*       			    			     												*/
/*	       								 												*/
/*输出：	pTagType[0],pTagType[1] =ATQA 											 	*/
/*	       	pSnr[0],pSnr[1],pSnr[2],pSnr[3]pSnr[4] =UID 		                   		*/
/*	       	pSak[0],pSak[1],pSak[2] =SAK			                            		*/					
/*       	TRUE: 应答正确                                                              	*/
/*	 		FALSE: 应答错误																*/
/****************************************************************************************/
int NfcTypeA::CardActive(unsigned char *uid)
{
	unsigned char  pTagType[2], pSak[3];
	int res;
	//Set_Rf(3);   //turn on radio
	//Pcd_ConfigISOType(0);

	//nfc_dev->set_rfmode(PN512_RFMODE_ALLON);
	nfc_dev->set_isotype(PN512_ISOTYPE_A);

	res = Request(pTagType);//寻卡 Standard	 send request command Standard mode

	if (res <= 0)
		return res;

	if ((pTagType[0] & 0xC0) == 0x00)       //M1卡,ID号只有4位
	{
		ALOGD("%s:M1 ID 4bit\n", __func__);

		res = Anticollision(0x93, uid);
		if(res < 0)
			return res;
		res = Select(0x93, uid, pSak);
		if (res < 0)
			return res;		

		ALOGD("%s:M1 ID: %x%x%x%x\n", __func__, uid[0], uid[1], uid[2], uid[3]);

		res = 4;
	}
	else if ((pTagType[0]&0xC0) == 0x40)     //ID号有7位
	{
		ALOGD("%s:M1 ID 7bit\n", __func__);

		res = Anticollision(0x93, uid);
		if (res < 0)
			return res;
		res = Select(0x93, uid, pSak);
		if (res < 0)
			return res;
		res = Anticollision(0x95, uid + 5);
		if (res < 0)
			return res;
		res = Select(0x95, uid + 5, pSak + 1);
		if (res < 0)
			return res;

		res = 7;
	}
	else if ((pTagType[0]&0xC0) == 0x80)       //ID号有10位
	{
		ALOGD("%s:M1 ID 10bit\n", __func__);

		res = Anticollision(0x93, uid);
		if (res < 0)
			return res;
		res = Select(0x93, uid, pSak);
		if (res < 0)
			return res;
		res = Anticollision(0x95, uid + 5);
		if (res < 0)
			return res;
		res = Select(0x95, uid + 5, pSak + 1);
		if (res < 0)
			return res;
		res = Anticollision(0x97, uid + 10);
		if (res < 0)
			return res;
		res = Select(0x97, uid + 10, pSak + 2);
		if (res < 0)
			return res;

		res = 10;
	}
	
	return res;
}

