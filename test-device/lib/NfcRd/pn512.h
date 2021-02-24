/*--------------------------------------------------------------------------------
File Name	:	pn512.cpp
Description	:	PN512读写接口，兼容
				NXP：PN512,RC522,RC520
				FM：FM17520,FM17522,FM17550
Revision	:	V1.0.0
Author		:	Bryan Jiang <598612779@qq.com>
Date		:	2018/10/25
----------------------------------------------------------------------------------*/

#ifndef __PN512_H__
#define __PN512_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include "NfcDev.h"
#include "pn512_io.h"
#include "pn512_reg.h"
#include "NfcTypeA.h"
#include "NfcTypeB.h"

class PN512:public NfcDev
{
public:
	PN512();
	~PN512();

	PN512_IO	*pn512_io;

	int start(void);
	int stop(void);

	bool			foundcard;				//当前是否找到IC卡
	unsigned char 	uid[16];				//当前获取到的UID号
	unsigned int 	uid_len;



private:
	int fd;		
	
	NfcTypeA 	*TypeA;
	NfcTypeB 	*TypeB;
	
	int 			lpcd_modules;

	pthread_t		pt_findcard;			//寻卡处理线程
	int				pt_findcard_exit;		//线程退出标志
	static void*	findcard(void *param);		

	int				poll_cycle;				//寻卡轮询周期，单位为ms	
	int				card_type;				//当前IC卡类型

	unsigned char 	pre_uid[16];			//上一次获取到的UID号
	unsigned int 	pre_uid_len;
	
	int 			poll_TypeA(PN512 *p_dev);
	int 			poll_TypeB(PN512 *p_dev);
};

#endif
