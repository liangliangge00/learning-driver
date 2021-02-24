/*--------------------------------------------------------------------------------
File Name	:	pn512.cpp
Description	:	PN512读写接口，兼容
				NXP：PN512,RC522,RC520
				FM：FM17520,FM17522,FM17550
Revision	:	V1.0.0
Author		:	Bryan Jiang <598612779@qq.com>
Date		:	2018/10/25
----------------------------------------------------------------------------------*/

#ifndef __NFCDEV_H__
#define __NFCDEV_H__

class NfcDev
{
public:
	int			available;						//设备是否可用
	int get_available(void)
	{
		return available;
	}

	void(*swiping_event)(NfcDev *dev, unsigned char* id, int len, int type);		//刷卡回调函数
};

#endif
