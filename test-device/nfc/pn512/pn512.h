
#ifndef __PN512_H__
#define __PN512_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#define FIFO_MAX_SIZE	64

#define TRUE   1
#define FALSE  0

struct pn512_reg
{
	unsigned char address;
	unsigned char value;
};

struct pn512_transfer_tx
{
	unsigned char		buf[FIFO_MAX_SIZE];
	unsigned char		length;
	unsigned char		command;
};

struct pn512_transfer_rx
{
	unsigned char		buf[FIFO_MAX_SIZE];
	unsigned int		bitlen;
};

/*********************LPCD模式使用***************************/
struct calibration_struct
{
 unsigned char Reference;//基准电容
 unsigned char Value;    //LPCD校准数值
 unsigned char Threshold_Max;//LPCD阈值
 unsigned char Threshold_Min;//LPCD阈值
 unsigned char Gain_Index;//LPCD校准Gain的序号
 unsigned char Driver_Index;	//LPCD校准Drive的序号
 unsigned char Range_L;	
 unsigned char Range_H;
};
struct calibration_backup_struct
{
 unsigned char Reference;//基准电容
 unsigned char Gain_Index;//LPCD校准Gain的序号
 unsigned char Driver_Index;	//LPCD校准Drive的序号
};
struct lpcd_struct
{
 unsigned char Timer1;//T1休眠时间
 unsigned char Timer2;//T2启动时间
 unsigned char Timer3;//T3发射时间
 unsigned char Timer3_Div ;//T3分频系数
 unsigned char Timer3_Offset;
 unsigned char TimerVmid;//VMID启动时间
 unsigned char Fullscale_Value;	//T3时间的满程数值
 unsigned char Value;	
 unsigned char Irq;//LPCD中断	
 struct calibration_struct Calibration;
 struct calibration_backup_struct Calibration_Backup;
};
/***********************************************************/

#define	TYPE					'S' 
#define PN512_IOC_REGWRITE      _IOWR(TYPE, 0, unsigned int)
#define PN512_IOC_REGREAD		_IOWR(TYPE, 1, unsigned int)
#define PN512_IOC_REGSBIT		_IOWR(TYPE, 2, unsigned int)
#define PN512_IOC_REGCBIT		_IOWR(TYPE, 3, unsigned int)
#define PN512_IOC_TXDATA		_IOWR(TYPE, 4, unsigned int)
#define PN512_IOC_RXDATA		_IOWR(TYPE, 5, unsigned int)
#define PN512_IOC_IRQENABLE     _IOWR(TYPE, 6, unsigned int)
#define PN512_IOC_IRQDISABLE    _IOWR(TYPE, 7, unsigned int)
#define PN512_IOC_ISOTYPE		_IOWR(TYPE, 8, unsigned int)
#define PN512_IOC_RFMODE		_IOWR(TYPE, 9, unsigned int)
#define PN512_IOC_SETTIMER      _IOWR(TYPE, 10, unsigned int)
#define PN512_IOC_HWRESET		_IOWR(TYPE, 11, unsigned int)
#define PN512_IOC_SWRESET		_IOWR(TYPE, 12, unsigned int)
#define PN512_IOC_SETPWD		_IOWR(TYPE, 13, unsigned int)

//添加LPCD模式控制
#define PN512_IOC_LPCD			_IOWR(TYPE, 14, unsigned int)
#define PN512_IOC_STARTCAL		_IOWR(TYPE, 15, unsigned int)

#define	PN512_ISOTYPE_A			0
#define	PN512_ISOTYPE_B			1

#define	PN512_RFMODE_OFF		0
#define	PN512_RFMODE_TX1ON		1
#define	PN512_RFMODE_TX2ON		2
#define	PN512_RFMODE_ALLON		3

#endif




