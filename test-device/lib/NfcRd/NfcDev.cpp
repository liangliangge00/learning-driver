/*--------------------------------------------------------------------------------
File Name	:	NfcDev.cpp 
Description	:	NFC设备线程处理
Revision	:	V1.0.0
Author		:	Bryan Jiang <598612779@qq.com>
Date		:	2018/10/25
----------------------------------------------------------------------------------*/

#define LOG_TAG "NfcDev"
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

#include "NfcDev.h"

NfcDev::NfcDev(void)
{
	int ret;
    
    ALOGI("%s:Create\n", __func__);

    pn512 = new PN512();
    TypeA = new NfcTypeA(pn512);
	TypeB = new NfcTypeB(pn512);
	
	dev_work = 0;
	poll_cycle = 200;
	foundcard = 0;
	card_type = PN512_ISOTYPE_A;

	memset(uid, 0, 16);
	memset(pre_uid, 0, 16);
	uid_len = 0;
	pre_uid_len = 0;

	//NFC芯片上电复位
	pn512->hw_reset();
	
	//测试NFC设备是否工作
	ret = pn512->dev_test();
	if (ret < 0) {
		dev_work = 0;
		return;
	}
	else
		dev_work = 1;

    swiping_event = NULL;

	//创建串口接收处理线程，这时候java层就不会阻塞，只是在等待回调
	pt_findcard_exit = 0;
	pthread_create(&pt_findcard, NULL, findcard, (void *)this);
}

NfcDev::~NfcDev(void)
{
	ALOGI("%s:Destroy\n", __func__);

	pt_findcard_exit = 1;
	pthread_join(pt_findcard, NULL);

	//NFC设备进入低功耗模式
	pn512->hw_powerdown(1);

    delete TypeA;
	TypeA = NULL;
	delete TypeB;
	TypeB = NULL;

    pn512->~PN512();
	delete pn512;
	pn512 = NULL;	
}

void* NfcDev::findcard(void *param) 
{
	NfcDev *p_dev = (NfcDev *)param;

	int res, i;
	char str[64], *p_str;

	while (!p_dev->pt_findcard_exit)
	{
		//开启RF发射
		p_dev->pn512->set_rfmode(PN512_RFMODE_ALLON);

		//寻卡
		res = p_dev->TypeA->CardActive(p_dev->uid);
		if (res > 0)
		{
			p_dev->uid_len = res;
			if (p_dev->uid_len > 16)
				p_dev->uid_len = 16;

			//判断IC卡号是否变化
			if ((memcmp(p_dev->uid, p_dev->pre_uid, p_dev->uid_len) != 0) | (p_dev->foundcard == 0))
			{
				memcpy(p_dev->pre_uid, p_dev->uid, p_dev->uid_len);
				p_dev->pre_uid_len = p_dev->uid_len;
				p_dev->foundcard = 1;
				p_dev->card_type = PN512_ISOTYPE_A;

                //执行回调函数
                if(p_dev->swiping_event != NULL)
                    p_dev->swiping_event(p_dev->pre_uid, p_dev->uid_len, p_dev->card_type);

                /*
				//将UID号转换为字符串
				p_str = str;
				for (i = 0; i < p_dev->uid_len; i++)
				{
					sprintf(p_str, "%02X", p_dev->pre_uid[i]);
					p_str += 2;
				}
				*p_str = 0;
                */

				//执行回调
				//env->CallIntMethod(p_dev->g_obj, javaCallbackId, uidStr, 0);

				//ALOGD("Got IC card:%s", str);
			}
		}
		else
			p_dev->foundcard = 0;

		//关闭RF发射
		p_dev->pn512->set_rfmode(PN512_RFMODE_OFF);

		//休眠
		if (p_dev->poll_cycle > 200)
		{
			res = p_dev->poll_cycle - 200;
			usleep(res * 1000);
		}
	}

	return 0;
}

