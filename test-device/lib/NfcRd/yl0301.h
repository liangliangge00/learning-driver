/*--------------------------------------------------------------------------------
File Name	:	yl0301.cpp
Description	:	YL0301 ID卡读卡模块处理，支持em4100，TK4100 125K卡片
Revision	:	V1.0.0
Author		:	Bryan Jiang <598612779@qq.com>
Date		:	2019/6/17
----------------------------------------------------------------------------------*/

#ifndef __YL0301_H__
#define __YL0301_H__

#include "NfcDev.h"

//YL0301 串口默认波特率
#define YL0301_SERIAL_BAUDRATE  9600
//YL0301 数据包大小
#define YL0301_PACKAGE_SIZE     5
//TL0301 数据包超时时间，单位为us
#define YL0301_PACKAGE_TIMEOUT  50000       //50ms

class YL0301:public NfcDev
{
public:
	YL0301(void);
	~YL0301(void);

    int start(void);
	int stop(void);

private:
	int			fd_serial;						//串口设备描述符
	char		serial_port[64];				//串口设备
	
	pthread_t	pt_serial;						//pstn处理线程
	int			pt_serial_exit;					//线程退出标志
	static void* serial_pthread(void *param);
};

#endif
