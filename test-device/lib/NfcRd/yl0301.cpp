

#define LOG_TAG "YL0301"
#define LOG_NDDEBUG 0

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termio.h>     /* POSIX terminal control definitions */
#include <sys/time.h>	/* Time structures for select() */
#include <unistd.h>     /* POSIX Symbolic Constants */
#include <errno.h>      /* Error definitions */
#include <pthread.h>
#include <signal.h> 
#include <sys/time.h>
#include <strings.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>

#include "yl0301.h"

//----------------------------------------------------------------------
// Name:			set_serial
// Parameters:		
// Returns:			0 ==> 成功， <0 ==> 失败
// Description:		串口设置
//----------------------------------------------------------------------
static int set_serial(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;

	ALOGI("%s: %d %d %d %d\n", __func__, nSpeed, nBits, nEvent, nStop);

    if(tcgetattr(fd,&oldtio)!=0)
        return -1;
    bzero(&newtio,sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;   //激活选项(用于本地连接和接收使能)
    newtio.c_cflag &= ~CSIZE;           //设置数据位长度

    switch(nBits)
    {
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
    }

    switch(nEvent)  //设置奇偶校验位
    {
        case 'O':           //偶校验
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'E':           //奇校验
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'N':           //无校验
            newtio.c_cflag &= ~PARENB;
            break;
    }
    switch(nSpeed)  //设置通信速率
    {
        case 1200:
            cfsetispeed(&newtio, B1200);
            cfsetospeed(&newtio, B1200);
            break;
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 19200:
            cfsetispeed(&newtio, B19200);
            cfsetospeed(&newtio, B19200);
            break;
		case 57600:
			cfsetispeed(&newtio, B57600);
			cfsetospeed(&newtio, B57600);
			break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }
    if(nStop==1)    //设置停止位长度
        newtio.c_cflag &= ~CSTOPB;
    else if (nStop==2)
        newtio.c_cflag |= CSTOPB;

    newtio.c_cflag |= 1;
    newtio.c_cc[VTIME]= 0;
    newtio.c_cc[VMIN] = 1;
    tcflush(fd,TCIFLUSH);                   //清空发送/接收缓冲区

	if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)  //重新配置串口参数
	{
		ALOGE("%s: tcsetattr error\n", __func__);
		return -1;
	}

    return 0;
}

//----------------------------------------------------------------------
// Name:			CheckSum
// Parameters:		
// Returns:			0 ==> 成功， <0 ==> 失败
// Description:		ID卡号校验
//----------------------------------------------------------------------
static unsigned char CheckSum(unsigned char *dat, unsigned char num)
{
  unsigned char bTemp = 0, i;
  
  for(i = 0; i < num; i ++)
  	bTemp ^= dat[i];
  
  return bTemp;
}

//----------------------------------------------------------------------
// Name:			pstn_pthread
// Parameters:		
// Returns:			none
// Description:		PSTN处理线程
//----------------------------------------------------------------------
void* YL0301::serial_pthread(void *param)
{
	YL0301 *p_srv = (YL0301 *)param;

	//文件描述符
	fd_set rfds;
	struct timeval tv;

	int res, time, rec_count, rec_timecount;
	unsigned char *rec_buf;

	//申请内存用于接收缓冲
	rec_buf = (unsigned char *)malloc(1024);

	rec_count = 0;
    rec_timecount = 0;

	while (!p_srv->pt_serial_exit)
	{
		//添加文件描述符
		FD_ZERO(&rfds);
		FD_SET(p_srv->fd_serial, &rfds);
		//设置阻塞超时时间
		tv.tv_sec = 0;
		tv.tv_usec = 10 * 1000;		//10ms

		res = select(p_srv->fd_serial + 1, &rfds, NULL, NULL, &tv);
		if (res > 0)
		{
			//读取串口接收到的数据
			res = read(p_srv->fd_serial, rec_buf, YL0301_PACKAGE_SIZE - rec_count);
            if( res > 0) {
                rec_count += res;
                if(rec_count >= YL0301_PACKAGE_SIZE) {
                    rec_count = 0;
                    //检验数据包是否正确
                    if(CheckSum(rec_buf, 4) == rec_buf[4]) {
                        //执行回调函数
                        if(p_srv->swiping_event != NULL)
                            p_srv->swiping_event(p_srv, rec_buf, 4, 2);
                    }
                }

                rec_timecount = 0;
            }
		}

		time = (10 * 1000) - tv.tv_usec;
        // 接收数据超时处理
        if(rec_count) {
            rec_timecount += time;
            if(rec_timecount > YL0301_PACKAGE_TIMEOUT) {
                ALOGE("Serial receive timeout %d\n", rec_timecount);
                rec_count = 0;
            }
        }        
	}

	free(rec_buf);

	return 0;
}

//----------------------------------------------------------------------
// Name:			YL0301
// Parameters:		
// Returns:			
// Description:		PSTN服务构造函数
//----------------------------------------------------------------------
YL0301::YL0301(void)
{
	char property[128];
	
    available = 0;

	// 获取ID卡模块型号
	if (property_get("ro.board.idcard.model", property, NULL) != 0) {
		if(strcmp(property, "YL0301") != 0) {
            // 当前的ID卡模块不匹配
            ALOGE("%s:id card model not match: %s\n", __func__, property);  
            return;  
        }			
	}

    // 获取ID卡模块对应的串口设备号
	if (property_get("ro.board.idcard.port", property, NULL) == 0) {
		ALOGE("%s:unknown id card model port\n", __func__);  
        return;  
	}
    sprintf(serial_port, "/dev/%s", property);

    available = 1;
    swiping_event = NULL;
    pt_serial_exit = 1;
}

//----------------------------------------------------------------------
// Name:			YL0301_Destroy
// Parameters:		none
// Returns:			none
// Description:		PSTN服务销毁函数
//----------------------------------------------------------------------
YL0301::~YL0301(void)
{
	ALOGI("%s:Destroy\n", __func__);

    if(pt_serial_exit==0)
        stop();
}

int YL0301::start(void)
{
    ALOGI("YL0301 dev start\n");

	if(available == 0){
		ALOGE("%s:YL0301 not available, start error\n", __func__);
		return -1;
	}
    
    //打开串口设备
	fd_serial = open(serial_port, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd_serial < 0) {
		ALOGE("open serial %s failed!\n", serial_port);
        available = 0;
		return -2;
	}
    
	//设置串口参数
	set_serial(fd_serial, YL0301_SERIAL_BAUDRATE, 8, 'N', 1);
		
	//创建串口接收处理线程
	pt_serial_exit = 0;
	pthread_create(&pt_serial, NULL, serial_pthread, (void *)this);

    return 0;
}

int YL0301::stop(void)
{
    ALOGI("YL0301 dev stop\n");
    
    //结束接收线程
    if(available){
	    pt_serial_exit = 1;
	    pthread_join(pt_serial, NULL);
    }

	//关闭串口设备
	if (fd_serial != 0)
	{
		close(fd_serial);
		fd_serial = 0;
	}

    return 0;
}








