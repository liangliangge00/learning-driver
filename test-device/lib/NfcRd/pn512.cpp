/*--------------------------------------------------------------------------------
File Name	:	pn512.cpp
Description	:	PN512读写接口，兼容
				NXP：PN512,RC522,RC520
				FM：FM17520,FM17522,FM17550
Revision	:	V1.0.0
Author		:	Bryan Jiang <598612779@qq.com>
Date		:	2018/10/25
----------------------------------------------------------------------------------*/

#define LOG_TAG "PN512"
//#define LOG_NDDEBUG 0

#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <cassert>
#include <cstdlib>
#include <unistd.h>
#include <sys/select.h> 

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>
#include "pn512.h"


PN512::PN512(void)
{
	ALOGI("%s:Create\n", __func__);

	swiping_event = NULL;
	pt_findcard_exit = 1;
	available = 0;

	poll_cycle = 200;
	card_type = PN512_ISOTYPE_A;

	memset(uid, 0, 16);
	memset(pre_uid, 0, 16);
	uid_len = 0;
	pre_uid_len = 0;

	//打开PN512设备
	fd = open("/dev/pn512", O_RDWR);
	if (fd == -1) {
		ALOGE("%s: pn512 dev open error", __func__);
		return;
	}
		
	pn512_io = new PN512_IO(fd);
	TypeA = new NfcTypeA(pn512_io);
	TypeB = new NfcTypeB(pn512_io);

	//NFC芯片上电复位
	pn512_io->hw_powerdown(0);
	pn512_io->hw_reset();
	
	//测试NFC设备是否工作
	if (pn512_io->dev_test() < 0) {
		available = 0;
		return;
	}
	else
		available = 1;
}

PN512::~PN512(void)
{
	ALOGI("%s:Destroy\n", __func__);
	
	if(pt_findcard_exit == 0)
		stop();

	delete TypeA;
	TypeA = NULL;
	delete TypeB;
	TypeB = NULL;
	delete pn512_io;
	pn512_io = NULL;

	if(fd > 0)
		close(fd);
}

int PN512::start(void)
{
	int ret;
    char property[128];
	
    ALOGI("PN512 dev start\n");

	if(available == 0){
		ALOGE("%s:PN512 not available, start error\n", __func__);
		return -1;
	}

	poll_cycle = 200;
	card_type = PN512_ISOTYPE_A;

	memset(uid, 0, 16);
	memset(pre_uid, 0, 16);
	uid_len = 0;
	pre_uid_len = 0;

	//NFC芯片上电复位
	pn512_io->hw_powerdown(0);
	pn512_io->hw_reset();

	//上电延时200ms, 等待芯片稳定
	usleep(200000);
 	
	//获取安卓属性，是否打开lpcd
	if (property_get("persist.fm175xx.lpcd", property, NULL) != 0) {
		if(strcmp(property, "true") == 0) {
			lpcd_modules = 1;
            ALOGE("%s:lpcd modules on: %s\n", __func__, property);   
        }
	}
	else {
		lpcd_modules = 0;
        ALOGE("%s:lpcd modules off: %s\n", __func__, property);   
    } 
	
	//创建串口接收处理线程，这时候java层就不会阻塞，只是在等待回调
	pt_findcard_exit = 0;
	pthread_create(&pt_findcard, NULL, findcard, (void *)this);

	return 0;
}

int PN512::stop(void)
{
	ALOGI("PN512 dev stop\n");
	
	if(available){
		pt_findcard_exit = 1;
		pthread_join(pt_findcard, NULL);

		//NFC设备进入低功耗模式
		pn512_io->hw_powerdown(1);
	}
	return 0;
}

int PN512::poll_TypeA(PN512 *p_dev)
{	
	int ret,i;
	char str[64], *p_str;

	//寻卡
	ret = p_dev->TypeA->CardActive(p_dev->uid);
	if (ret > 0){
		p_dev->uid_len = ret;
		if (p_dev->uid_len > 16)
			p_dev->uid_len = 16;
		
		//判断IC卡号是否变化
		if ((memcmp(p_dev->uid, p_dev->pre_uid, p_dev->uid_len) != 0) || (p_dev->foundcard == 0))
		{
			memcpy(p_dev->pre_uid, p_dev->uid, p_dev->uid_len);
			p_dev->pre_uid_len = p_dev->uid_len;
			p_dev->card_type = PN512_ISOTYPE_A;
			p_dev->foundcard = 1;

			//执行回调函数
			if(p_dev->swiping_event != NULL)
				p_dev->swiping_event(p_dev, p_dev->pre_uid, p_dev->uid_len, p_dev->card_type);
			/*
			//将UID号转换为字符串
			p_str = str;
			for (i = 0; i < p_dev->uid_len; i++){
				sprintf(p_str, "%02X", p_dev->pre_uid[i]);
				p_str += 2;
			}
			*p_str = 0;

			ALOGD("Got IC card:%s", str);
			*/
		}

		p_dev->TypeA->Halt();
	}

	return ret;
}

int PN512::poll_TypeB(PN512 *p_dev)
{
	int res, ret = -1;
	unsigned char read_data[FIFO_MAX_SIZE];
	unsigned char pupi[4];
	unsigned int Rec_len;
	
	p_dev->pn512_io->sw_reset();
	p_dev->pn512_io->set_isotype(PN512_ISOTYPE_B);//设置TypeB

	//开启RF发射
	p_dev->pn512_io->set_rfmode(PN512_RFMODE_ALLON);
	p_dev->TypeB->WakeUp(pupi, read_data, &Rec_len);
	if(Rec_len == 0x60){	
		res = p_dev->TypeB->Select(pupi, &Rec_len, read_data);
		if(res > 0) {
			
			//ALOGE("get card: %x%x%x%x\n", pupi[0], pupi[1], pupi[2], pupi[3]);
			//Rec_len = 16;
			
			res = p_dev->TypeB->GetUID(&Rec_len, p_dev->uid);	
			if(res > 0){
				ALOGE("b getuid res :%d\n",res);
				p_dev->uid_len = res;
			//	ALOGE("pre_uid :%x%x%x%x%x%x%x%x%x%x\n", p_dev->pre_uid[0],p_dev->pre_uid[1],p_dev->pre_uid[2],p_dev->pre_uid[3],p_dev->pre_uid[4],p_dev->pre_uid[5],p_dev->pre_uid[6],p_dev->pre_uid[7],p_dev->pre_uid[8],p_dev->pre_uid[9]);
			//	ALOGE("    uid :%x%x%x%x%x%x%x%x%x%x\n", p_dev->uid[0],p_dev->uid[1],p_dev->uid[2],p_dev->uid[3],p_dev->uid[4],p_dev->uid[5],p_dev->uid[6],p_dev->uid[7],p_dev->uid[8],p_dev->uid[9]);
				//判断IC卡号是否变化
				if ((memcmp(p_dev->uid, p_dev->pre_uid, p_dev->uid_len) != 0) || (p_dev->foundcard == 0)){
					memcpy(p_dev->pre_uid, p_dev->uid, p_dev->uid_len);
					p_dev->pre_uid_len = p_dev->uid_len;
					p_dev->foundcard = 1;
					p_dev->card_type = PN512_ISOTYPE_B;
				

					//执行回调函数
					if(p_dev->swiping_event != NULL)
						p_dev->swiping_event(p_dev, p_dev->pre_uid, p_dev->uid_len, p_dev->card_type);
				}

				ret = 1;
			}
		}

		p_dev->TypeB->Halt(pupi);
	}
	
	return ret;
}

void* PN512::findcard(void *param) 
{
	PN512 *p_dev = (PN512 *)param;
	
 	int ret, error_count;
	
	char str[64], *p_str, rdbuf[9];

	if(p_dev->lpcd_modules){
		p_dev->pn512_io->lpcd_calibration();// LPCD校验
		p_dev->pn512_io->lpcd_enable(1);// 开启LPCD功能
	}
	
 	while (!p_dev->pt_findcard_exit){
		
		if(p_dev->lpcd_modules){
			//ALOGE("$$$$$$$$$$INTO LPCD_MODULES 1\n");
			ret = read(p_dev->pn512_io->fd, rdbuf, 5);// 获取是否是IC卡触发
			if(ret){
				p_dev->pn512_io->lpcd_enable(0);	//关闭LPCD

				error_count = 0;
				while (!p_dev->pt_findcard_exit){
					//开启RF发射
					p_dev->pn512_io->set_rfmode(PN512_RFMODE_ALLON);

					// TypeA 寻卡
					ret = p_dev->poll_TypeA(p_dev);
					if(ret <= 0){  
						// TypeB 寻卡
						ret = p_dev->poll_TypeB(p_dev);
						if(ret > 0)
							error_count=0;
						else {
							error_count++;	
							p_dev->foundcard = 0;
						}
					}			
					else
						error_count=0;
					
					//关闭RF发射
					p_dev->pn512_io->set_rfmode(PN512_RFMODE_OFF);
				
					if(error_count >= 30) {	
						p_dev->pn512_io->lpcd_enable(1);
						break;
					}
					else
						usleep(p_dev->poll_cycle * 1000);
				}
			}
		}
 		else{
			//开启RF发射
			p_dev->pn512_io->set_rfmode(PN512_RFMODE_ALLON);
			while (!p_dev->pt_findcard_exit){
				// TypeA 寻卡
				ret = p_dev->poll_TypeA(p_dev);
				if(ret <= 0){
					// TypeB 寻卡
					ret = p_dev->poll_TypeB(p_dev);
					if(ret > 0){
						error_count=0;
					}
					else {
						error_count++;	
						p_dev->foundcard = 0;
					}
				}			
				else
					error_count=0;
				//sleep(1);
				usleep(200*1000);
			}
			//关闭RF发射
			p_dev->pn512_io->set_rfmode(PN512_RFMODE_OFF);
		}//lpcd_modules==0 
	}//while(1)

	return 0;
}




