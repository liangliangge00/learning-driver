/*--------------------------------------------------------------------------------
File Name	:	wiegand.h
Description	:	wiegand读写接口
Revision	:	V1.0.0
Author		:	shenxj <2200319548@qq.com>
Date		:	2019/10/21
----------------------------------------------------------------------------------*/

#ifndef __WIEGAND_H__
#define __WIEGAND_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#define WIEGAND_DATA_BUF_LEN 64

class WiegandDev
{
public:
	WiegandDev();
	~WiegandDev();

	
	
	unsigned char 	data[WIEGAND_DATA_BUF_LEN];			//获取到的数据
	unsigned int 	data_len;

	int start(void);
	int stop(void);
	void(*recv_event)(WiegandDev *dev, unsigned char* data, int len);//回调函数

private:	
	int fd;
	pthread_t		pt_getdata;				//获取数据处理线程
	int				pt_getdata_exit;		//线程退出标志
	static void*	getdata(void *param);

	int				poll_cycle;				//获取数据轮询周期，单位为ms	

	unsigned char 	pre_data[WIEGAND_DATA_BUF_LEN];			//上一次获取到的数据
	unsigned int 	pre_data_len;
	
};


#endif //__WIEGAND_H__

