/*--------------------------------------------------------------------------------
File Name	:	wiegand.cpp
Description	:	wiegand读写接口
Revision	:	V1.0.0
Author		:	shenxj <2200319548@qq.com>
Date		:	2019/10/21
----------------------------------------------------------------------------------*/

#define LOG_TAG "wiegand"
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
#include "wiegand.h"


WiegandDev::WiegandDev(void)
{
	ALOGI("%s:Create\n", __func__);	
	fd = open("/sys/class/wiegand/wiegand-send/receive", O_RDWR);
	if(fd ==-1)
	{
		ALOGE("%s: WiegandDev open error", __func__);
		return;
	}

	memset(data, 0, WIEGAND_DATA_BUF_LEN);
	memset(pre_data, 0, WIEGAND_DATA_BUF_LEN);
	data_len = 0;
}

WiegandDev::~WiegandDev(void)
{
	ALOGI("%s:Destroy\n", __func__);
	
	if(pt_getdata_exit == 0)
		stop();

	if(fd > 0)
		close(fd);
}

int WiegandDev::start(void)
{
	int ret;
	
    ALOGI("WiegandDev start\n");

	poll_cycle = 200;

	memset(data, 0, WIEGAND_DATA_BUF_LEN);
	memset(pre_data, 0, WIEGAND_DATA_BUF_LEN);
	data_len = 0;
	pre_data_len = 0;
	
	//创建串口接收处理线程，这时候java层就不会阻塞，只是在等待回调
	pt_getdata_exit = 0;
	pthread_create(&pt_getdata, NULL, getdata, (void *)this);

	return 0;
}

int WiegandDev::stop(void)
{
	ALOGI("WiegandDev stop\n");

	pt_getdata_exit = 1;
	pthread_join(pt_getdata, NULL);

	return 0;
}

static int get_system_output(char *cmd, char *output, int size)
{
    FILE *fp=NULL;  
    fp = popen(cmd, "r");   
    if (fp)
    {       
        if(fgets(output, size, fp) != NULL)
        {       
            if(output[strlen(output)-1] == '\n')            
                output[strlen(output)-1] = '\0';    
        }   
        pclose(fp); 
    }
    return 0;
}

void* WiegandDev::getdata(void *param) 
{
	WiegandDev *p_dev = (WiegandDev *)param;
	int ret = 0;
	char rec_buf[WIEGAND_DATA_BUF_LEN]={0};
	char new_flag[8]={0};

 	while (!p_dev->pt_getdata_exit){		
		//ret = read(p_dev->fd, p_dev->data, WIEGAND_DATA_BUF_LEN);
		/*ret = read(p_dev->fd, rec_buf, WIEGAND_DATA_BUF_LEN);
		ALOGI("%s[%d],rec_buf{%s}\n", __func__, __LINE__,
			rec_buf);*/
		//p_dev->data_len = strlen((char *)(p_dev->data));
		//ALOGI("%s[%d],data[%s],data_len[%d]\n", __func__, __LINE__,
		//	p_dev->data, p_dev->data_len);

		//use system		
		get_system_output("cat /sys/class/wiegand/wiegand-send/new_flag",
			new_flag, 8);
		
		if(strcmp(new_flag, "1") == 0)
		{
			get_system_output("cat /sys/class/wiegand/wiegand-send/receive",
				(char *)(p_dev->data), WIEGAND_DATA_BUF_LEN);
			p_dev->data_len = strlen((char *)(p_dev->data));
			
			ALOGI("%s[%d],data[%s],data_len[%d]\n", __func__, __LINE__,
				p_dev->data, p_dev->data_len);
			
			for(int index=0;index < p_dev->data_len;index++){
				p_dev->data[index] -= 48;//ASCII to number
			}

			get_system_output("echo 0 > /sys/class/wiegand/wiegand-send/new_flag",
				NULL, 0);
			
			//执行回调函数
			if(p_dev->recv_event != NULL)
				p_dev->recv_event(p_dev, p_dev->data, (int)(p_dev->data_len));
		}
		usleep(p_dev->poll_cycle*1000);
	}

	return 0;
}


