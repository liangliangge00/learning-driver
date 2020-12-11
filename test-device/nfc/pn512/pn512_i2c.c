
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <asm/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/switch.h>
#include <linux/semaphore.h>


#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#endif

#include "pn512.h"
#include "pn512_reg.h"

#define DEVICE_NAME			"pn512"
#define DRIVER_VERSION  	"1.0"

#define		LPCD_AUTO_WUP_EN			0     	//自动唤醒使能//0=Disable；1=Enable
#define 	LPCD_AUTO_WUP_CFG			0  		//自动唤醒时间设置//0=6秒；1=12秒；2=15分钟；3=30分钟；4=1小时；5=1.8小时；6==3.6小时；7=7.2小时 
#define		LPCD_BIAS_CURRENT   		3		//3 偏置电流//0=50nA；1=100nA；2=150nA；3=200nA；4=250nA；5=300nA；6=350nA；7=400nA；
#define 	LPCD_TX2RF_EN				1		//LPCD功能TX2发射输出使能//0=Disable；1=Enable
#define		LPCD_CALIBRA_IRQ_TIMEOUT	255 	//LPCD校准中断超时设置 1~255us

/*==============================================================================================
#define		LPCD_THRESHOLD	   			3 		//LPCD检测灵敏度设置   数值越高，灵敏度越低

#define LPCD_TIMER1_CFG   		3	//LPCD休眠阶段时间设置（阶段1）
//0=0秒；1=300毫秒；2=400毫秒；3=500毫秒；4=600毫秒；5=700毫秒；6=800毫秒；7=900毫秒；
//8=1秒；9=1.1秒；10=1.2秒；11=1.3秒；12=1.4秒；13=1.5秒；14=1.6秒；15=1.7秒；

#define LPCD_TIMER2_CFG  	  31//13	//LPCD准备阶段时间设置（阶段2）
//0=0ms；1=0ms；2=0.4ms；3=0.5ms；4=0.6ms；5=0.7ms；6=0.8ms；7=0.9ms；8=1.0ms；
//9=1.1ms；10=1.2ms；11=1.3ms；12=1.4ms；13=1.5ms；14=1.6ms；15=1.7ms；16=1.8ms；
//17=1.9ms；18=2.0ms；19=2.1ms；20=2.2ms；21=2.3ms；22=2.4ms；23=2.5ms；24=2.6ms；
//25=2.7ms；26=2.8ms；27=2.9ms；28=3.0ms；29=3.1ms；30=3.2ms；31=3.3ms；

#define LPCD_TIMER3_CFG  	  	31//4	//LPCD检测阶段时间设置（阶段3）
//0=0us；1=0us；2=4.7us；3=9.4us；4=14.1us；5=18.8us；6=23.5us；7=28.2us；8=32.9us；
//9=37.6us；10=42.3us；11=47us；12=51.7us；13=56.8us；14=61.1us；15=65.8us；16=70.5us；
//17=75.2us；18=79.9us；19=84.6us；20=89.3us；21=94us；22=98.7us；23=103.4us；24=108.1us；
//25=112.8us；26=117.5us；27=122.2us；28=126.9us；29=131.6us；30=136.3us；31=141us；
//==============================================================================================*/

#define	LPCD_TIMER_VMID_CFG		3	//VMID准备时间设置(参考电压)
//0=0ms；1=0ms；2=0.4ms；3=0.5ms；4=0.6ms；5=0.7ms；6=0.8ms；7=0.9ms；8=1.0ms；
//9=1.1ms；10=1.2ms；11=1.3ms；12=1.4ms；13=1.5ms；14=1.6ms；15=1.7ms；16=1.8ms；
//17=1.9ms；18=2.0ms；19=2.1ms；20=2.2ms；21=2.3ms；22=2.4ms；23=2.5ms；24=2.6ms；
//25=2.7ms；26=2.8ms；27=2.9ms；28=3.0ms；29=3.1ms；30=3.2ms；31=3.3ms；
#define Lpcd_Reset_Status() do{SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_CLR + JBIT_LPCD_RSTN);SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_SET + JBIT_LPCD_RSTN);}while(0)


static const unsigned char LPCD_GAIN[11] = {0x00,0x02,0x01,0x03,0x07,0x0B,0x0F,0x13,0x17,0x1B,0x1F};//0.4x 0.5x 0.6x 1.0x 1.2x 1.4x 2.0x 2.2x 2.6x 3.2x 4.0x
static const unsigned char LPCD_P_DRIVER[7]={0x01,0x02,0x03,0x04,0x05,0x06,0x07};
static const unsigned char LPCD_N_DRIVER[7]={0x01,0x04,0x06,0x08,0x0A,0x0C,0x0F};

struct pn512_dev {
	struct i2c_client   *client;
	struct miscdevice   misc_dev;

	wait_queue_head_t   read_wq;//队列
	struct mutex        i2c_mutex;
	struct mutex        io_mutex;

	unsigned int        irq_gpio;
	unsigned int        npd_gpio;
	unsigned int        npd_valid;
	struct pinctrl			*pctrl;
	struct pinctrl_state	*pctrl_active;
	
	unsigned int  lpcd_cfg_array[3];
	unsigned int lpcd_threshold;
	
	int flag;
	unsigned int current_len;
	bool                irq_enabled;
	spinlock_t          irq_enabled_lock;//自旋锁
};

static struct lpcd_struct Lpcd;
static struct pn512_dev *pn512_data;
struct wake_lock nfc_wake_lock;
static struct pn512_transfer_tx pn512_tx;
static struct pn512_transfer_rx pn512_rx;

static unsigned char reg_read(unsigned char address)
{
	unsigned char value,ret;
	
	mutex_lock(&pn512_data->i2c_mutex);

	ret = i2c_master_send(pn512_data->client, &address, 1);
	if(ret<0)
		pr_err("i2c_master_send failed\n");
	
	ret = i2c_master_recv(pn512_data->client, &value, 1);
	if(ret<0)
		pr_err("i2c_master_recv failed\n");
	
	mutex_unlock(&pn512_data->i2c_mutex);

	return value;
}

static int reg_write(unsigned char address, unsigned char value)
{
	unsigned char buf[4];
	int res;

	buf[0] = address;
	buf[1] = value;

	mutex_lock(&pn512_data->i2c_mutex);
	res = i2c_master_send(pn512_data->client, buf, 2);
	if(res<0)
		printk("i2c_master_send failed\n");
	mutex_unlock(&pn512_data->i2c_mutex);

	return res;
}

static int reg_setbit(unsigned char address, unsigned char mask)
{
	unsigned char value;

	value = reg_read(address);
	return reg_write(address, value | mask);  // set bit mask
}

static int reg_clearbit(unsigned char address, unsigned char mask)
{
	unsigned char value;

	value = reg_read(address);
	return reg_write(address, value & ~mask);  // clear bit mask
}

static int fifo_write(unsigned char *data, int length)
{
	int i;

	if (length > FIFO_MAX_SIZE)
		length = FIFO_MAX_SIZE;

	for (i = 0; i < length; i++) {
		reg_write(FIFODataReg, data[i]);
	}

	return length;
}

static int fifo_read(unsigned char *data, int length)
{
	int i;

	if (length > FIFO_MAX_SIZE)
		length = FIFO_MAX_SIZE;

	for (i = 0; i < length; i++) {
		data[i] = reg_read(FIFODataReg);
	}

	return length;
}

static int fifo_clear(void)
{
	reg_setbit(FIFOLevelReg, 0x80);//清除FIFO缓冲

	if (reg_read(FIFOLevelReg) == 0)
		return 0;
	else
		return -EPERM;
}
//----------------------------------------------------------------------
// Name:			Set_RF
// Parameters:		
//					mode：射频输出模式
//						0，关闭输出
//						1，仅打开TX1输出
//						2，仅打开TX2输出
//						3，TX1，TX2打开输出，TX2为反向输出
// Returns:			TRUE :成功
//					FALSE :失败
// Description:		设置射频输出
//----------------------------------------------------------------------
static int set_rfmode(unsigned char mode)
{
	int res;

	//获取当前射频输出模式
	res = reg_read(TxControlReg);
	if ((res & 0x03) == mode)
		return 0;

	switch (mode)
	{
	case PN512_RFMODE_OFF:
		res = reg_clearbit(TxControlReg, 0x00); //关闭TX1，TX2输出  //由0x03改为0x00
		break;
	case PN512_RFMODE_TX1ON:
		res = reg_clearbit(TxControlReg, 0x01); //仅打开TX1输出
		break;
	case PN512_RFMODE_TX2ON:
		res = reg_clearbit(TxControlReg, 0x02); //仅打开TX2输出
		break;
	case PN512_RFMODE_ALLON:
		res = reg_setbit(TxControlReg, 0x03);	//打开TX1，TX2输出
		break;
	}

	mdelay(100);

	return res;
}

//----------------------------------------------------------------------
// Name:			Pcd_SetTimer
// Parameters:		
//					delaytime：延时时间（单位为毫秒）
// Returns:			TRUE :成功
//					FALSE :失败
// Description:		设置接收延时
//----------------------------------------------------------------------
static void set_timer(unsigned int delaytime)
{
	unsigned int TimeReload;
	unsigned int Prescaler;

	Prescaler = 0;
	TimeReload = 0;
	while (Prescaler < 0xfff)
	{
		TimeReload = ((delaytime*(long)13560) - 1) / (Prescaler * 2 + 1);

		if (TimeReload < 0xffff)
			break;
		Prescaler++;
	}
	TimeReload = TimeReload & 0xFFFF;
	reg_setbit(TModeReg, Prescaler >> 8);
	reg_write(TPrescalerReg, Prescaler & 0xFF);
	reg_write(TReloadMSBReg, TimeReload >> 8);
	reg_write(TReloadLSBReg, TimeReload & 0xFF);
}

//----------------------------------------------------------------------
// Name:			Pcd_ConfigISOType
// Parameters:		
//					type：协议类型
//						0，ISO14443A协议；
//						1，ISO14443B协议；
// Returns:			TRUE :成功
//					FALSE :失败
// Description:		读卡器通信
//----------------------------------------------------------------------
static void set_isotype(unsigned char type)
{

	if (type == PN512_ISOTYPE_A)                     //ISO14443_A
	{
		reg_setbit(ControlReg, 0x10); //ControlReg 0x0C 设置reader模式
		reg_setbit(TxAutoReg, 0x40); //TxASKReg 0x15 设置100%ASK有效
		reg_write(TxModeReg, 0x00);  //TxModeReg 0x12 设置TX CRC无效，TX FRAMING =TYPE A
		reg_write(RxModeReg, 0x00); //RxModeReg 0x13 设置RX CRC无效，RX FRAMING =TYPE A
	}
	else if (type == PN512_ISOTYPE_B)                     //ISO14443_B
	{
		reg_write(ControlReg, 0x10);
		reg_write(TxModeReg, 0x83);//BIT1~0 = 2'b11:ISO/IEC 14443B
		reg_write(RxModeReg, 0x83);//BIT1~0 = 2'b11:ISO/IEC 14443B
		reg_write(TxAutoReg, 0x00);
		reg_write(RxThresholdReg, 0x55);
		reg_write(RFCfgReg, 0x48);//?????????????
		reg_write(TxBitPhaseReg, 0x87);//默认值	
		reg_write(GsNReg, 0x83);
		reg_write(CWGsPReg, 0x10);
		reg_write(GsNOffReg, 0x38);
		reg_write(ModGsPReg, 0x10);

	}
	else
		return;

	mdelay(1);
}

//----------------------------------------------------------------------
// Name:			pn512_hwreset
// Parameters:		
// Returns:			
// Description:		硬件复位
//----------------------------------------------------------------------
static void pn512_hwreset(void)
{
	gpio_set_value(pn512_data->npd_gpio, !pn512_data->npd_valid);
	udelay(500);
	gpio_set_value(pn512_data->npd_gpio, pn512_data->npd_valid);
	udelay(500);
	gpio_set_value(pn512_data->npd_gpio, !pn512_data->npd_valid);
	udelay(1000);
}

//----------------------------------------------------------------------
// Name:			pn512_swreset
// Parameters:		
// Returns:			
// Description:		软件复位
//----------------------------------------------------------------------
static void pn512_swreset(void)
{
	reg_write(CommandReg, SoftReset);
    reg_setbit(ControlReg, 0x10);
}

//----------------------------------------------------------------------
// Name:			pn512_powerdown
// Parameters:		
// Returns:			
// Description:		进入掉电模式
//----------------------------------------------------------------------
static void pn512_powerdown(int enable)
{
	if(enable)
		gpio_set_value(pn512_data->npd_gpio, pn512_data->npd_valid);
	else
		gpio_set_value(pn512_data->npd_gpio, !pn512_data->npd_valid);
	mdelay(1);
}

//----------------------------------------------------------------------
// Name:			Pcd_Comm
// Parameters:		
//					Command：通信操作命令；
//					pInData：发送数据数组；
//					InLenByte：发送数据数组字节长度；
//					pOutData：接收数据数组；
//					pOutLenBit：接收数据的位长度
// Returns:			TRUE :成功
//					FALSE :失败
// Description:		读卡器通信
//----------------------------------------------------------------------
static int pn512_comm(struct pn512_transfer_tx *p_tx, struct pn512_transfer_rx *p_rx)
{
	int res = -1;
	unsigned char irqEn = 0x00;
	unsigned char waitFor = 0x00;
	unsigned char lastBits;
	unsigned char tmp;
	unsigned int i;

	//清楚FIFO缓冲
	fifo_clear();

	//清除IRQ标记
	reg_write(ComIrqReg, 0x7F);
	//设置TIMER自动启动
	reg_write(TModeReg, 0x80);

	switch (p_tx->command)
	{
		case MFAuthent:          //Mifare认证 
			irqEn = 0x12;
			waitFor = 0x10;
			break;
		case Transceive:         //发送FIFO中的数据到天线，传输后激活接收电路
			irqEn = 0x77;
			waitFor = 0x30;
			break;
		default:
			break;
	}
	
	reg_write(ComIEnReg, irqEn | 0x80);
	//reg_clearbit(ComIrqReg, 0x80);
	reg_write(CommandReg, Idle);
	reg_setbit(FIFOLevelReg, 0x80);

	//写数据到FIFO
	fifo_write(p_tx->buf, p_tx->length);
	
	//开始发送
	reg_write(CommandReg, p_tx->command);
	if (p_tx->command == Transceive) {
		reg_setbit(BitFramingReg, 0x80);
	}

	/* 根据时钟频率调整，操作M1卡最大等待时间25ms*/
	i = 25;
	do {
		tmp = reg_read(ComIrqReg);
		i--;
		mdelay(1);
	} while ((i != 0) && !(tmp & 0x01) && !(tmp & waitFor));//i==0表示延时到了，tmp&0x01!=1表示PCDsettimer时间未到
															//tmp&waitFor!=1表示指令执行完成															
	reg_clearbit(BitFramingReg, 0x80);
	if (i != 0) 
	{
		if (!(reg_read(ErrorReg) & 0x1B)) 
		{
			res = 0;

			if (tmp & irqEn & 0x01)
				res = -EAGAIN;			//定时器超时，未接受到数据
			
			if (p_tx->command == Transceive)
			{
				tmp = reg_read(FIFOLevelReg);
				lastBits = reg_read(ControlReg) & 0x07;

				//接收到的数据位长度
				if (lastBits) 
					p_rx->bitlen = (tmp - 1) * 8 + lastBits;
				else 
					p_rx->bitlen = tmp * 8;

				//从FIFO读取数据
				res = fifo_read(p_rx->buf, tmp);
			}
		}
	}
	
	//关闭发送
	reg_clearbit(BitFramingReg, 0x80);

	return res;
}

//写扩展寄存器
uint8_t  SetReg_Ext(uint8_t  ExtRegAddr,uint8_t  ExtRegData)
{
    uint8_t  res;
    uint8_t  addr,regdata;

    addr = JREG_EXT_REG_ENTRANCE;                               /*扩展寄存器入口地址0x0f*/       
    regdata = JBIT_EXT_REG_WR_ADDR + ExtRegAddr;                /*写入扩展寄存器地址*/    
    res = reg_write(addr,regdata);
    if (res == FALSE) 
        return FALSE;

    addr = JREG_EXT_REG_ENTRANCE;
    regdata = JBIT_EXT_REG_WR_DATA + ExtRegData;                        
    res = reg_write(addr,regdata);
    if (res == FALSE) 
        return FALSE;
        
    return TRUE;    
}

//读扩展寄存器
uint8_t  GetReg_Ext(uint8_t  ExtRegAddr,uint8_t * ExtRegData)
{
    uint8_t  res;
    uint8_t  addr,regdata;

    addr = JREG_EXT_REG_ENTRANCE;               	/* 扩展寄存器0x0f地址               */
    regdata = JBIT_EXT_REG_RD_ADDR + ExtRegAddr;    /* JBIT_EXT_REG_RD_DATA写入二级地址 */
                                                    /* ExtRegAddr 扩展寄存器地址        */
    res = reg_write(addr,regdata);					/* 写入扩展寄存器地址               */
    if (res == FALSE) 
        return FALSE;

    addr = JREG_EXT_REG_ENTRANCE;  	/* 扩展寄存器0x0f地址 */
    regdata = reg_read(addr);    	/* 读出扩展寄存器数据 */
   
    *ExtRegData = regdata;

    return TRUE;    
}

void Lpcd_Set_IRQ_pin(void)
{
    reg_write(JREG_COMMIEN,BIT7);//IRQ引脚反相输出			 
	reg_write(JREG_DIVIEN,BIT7);//IRQ引脚CMOS输出模式（IRQ引脚不需要外接上拉电阻）   
} 

//校准备份
void Lpcd_Calibration_Backup(void)
{
	Lpcd.Calibration_Backup.Reference = Lpcd.Calibration.Reference;
	Lpcd.Calibration_Backup.Gain_Index = Lpcd.Calibration.Gain_Index;
	Lpcd.Calibration_Backup.Driver_Index = Lpcd.Calibration.Driver_Index;
	printk(KERN_INFO "-> Bakckup Success!\r\n");		
/* 	pr_err("\n-> Bakckup Lpcd.Calibration.Reference:0x%x!\n",Lpcd.Calibration_Backup.Reference);		
	pr_err("-> Bakckup Lpcd.Calibration.Driver_Index:0x%x!\n",Lpcd.Calibration_Backup.Driver_Index);		
	pr_err("-> Bakckup Lpcd.Calibration_Backup.Gain_Index:0x%x!\n\n",Lpcd.Calibration_Backup.Gain_Index);		
		 */
	return;
}

//LPCD设置触发阈值
unsigned char Lpcd_Set_Threshold(unsigned char lpcd_threshold_min,unsigned char lpcd_threshold_max)
{
	unsigned char temp;
       
	if(lpcd_threshold_max < lpcd_threshold_min)
	{
		temp = lpcd_threshold_min;
		lpcd_threshold_min = lpcd_threshold_max;
		lpcd_threshold_max = temp;
	}

	SetReg_Ext(JREG_LPCD_THRESHOLD_MIN_L,(lpcd_threshold_min & 0x3F));	//写入THRESHOLD_Min阈值低6位
	SetReg_Ext(JREG_LPCD_THRESHOLD_MIN_H,(lpcd_threshold_min>>6));		//写入THRESHOLD_Min阈值高2位
	SetReg_Ext(JREG_LPCD_THRESHOLD_MAX_L,(lpcd_threshold_max & 0x3F));	//写入THRESHOLD_Max阈值低6位
	SetReg_Ext(JREG_LPCD_THRESHOLD_MAX_H,(lpcd_threshold_max>>6));		//写入THRESHOLD_Max阈值高2位
		
	return TRUE;
}

//LPCD设置输出驱动
unsigned char Lpcd_Set_Driver(unsigned char lpcd_cwp,unsigned char lpcd_cwn,unsigned char lpcd_tx2_en)
{
    unsigned char reg_data;
	reg_data=reg_read(JREG_VERSION);
    if(reg_data == 0x88)//V03版本芯片
	{
		if(lpcd_cwn > 1)
			lpcd_cwn = 1;
		lpcd_cwn &= 0x01;
		lpcd_cwp &= 0x07;
		SetReg_Ext(JREG_LPCD_CTRL2,((lpcd_tx2_en << 4) + (lpcd_cwn << 3) + lpcd_cwp));//V03版本芯片
	}
    if(reg_data == 0x89)//V03以上版本芯片
	{
	  lpcd_cwn &= 0x0F;
	  lpcd_cwp &= 0x07;
	  SetReg_Ext(JREG_LPCD_CTRL2,((lpcd_tx2_en<<4) + lpcd_cwp));//V03以上版本芯片
	  reg_write(JREG_GSN, lpcd_cwn << 4); //V03以上版本芯片
	}
	
	return TRUE;
}
//LPCD设置基准信号的充电电流与充电电容
unsigned char Lpcd_Set_Reference(unsigned char lpcd_bias_current,unsigned char lpcd_reference)
{
	lpcd_reference &= 0x7F;
	lpcd_bias_current &= 0x07;
	SetReg_Ext(JREG_LPCD_BIAS_CURRENT,((lpcd_reference & 0x40)>>1) + (lpcd_bias_current & 0x07));       
	SetReg_Ext(JREG_LPCD_ADC_REFERECE,(lpcd_reference & 0x3F));
	return TRUE;
}

//扩展寄存器位操作
uint8_t  ModifyReg_Ext(uint8_t  ExtRegAddr,uint8_t  mask,uint8_t  set)
{
    uint8_t  status;
    uint8_t  regdata;
    
    status = GetReg_Ext(ExtRegAddr,&regdata);
    if(status == TRUE) {
		if(set)
			regdata |= mask;
		else 
			regdata &= ~(mask);
    }
    else
        return FALSE;
    status = SetReg_Ext(ExtRegAddr,regdata);
    return status;
}

//等待LPCD中断
unsigned char Lpcd_WaitFor_Irq(unsigned char IrqType)
{
	unsigned char ExtRegData;
	unsigned char TimeOutCount;
	
	TimeOutCount = 0;
	GetReg_Ext(JREG_LPCD_IRQ,&ExtRegData);

	for(TimeOutCount = LPCD_CALIBRA_IRQ_TIMEOUT;TimeOutCount > 0;TimeOutCount--)
	{
		udelay(1);	//延时1us
		GetReg_Ext(JREG_LPCD_IRQ,&ExtRegData);	
		if(ExtRegData & IrqType)
			return TRUE; 
	}        
	return FALSE;
}

//读取LPCD的数值
unsigned char Lpcd_Get_Value(unsigned char *value)
{
	unsigned char ExtRegData;
	GetReg_Ext(JREG_LPCD_ADC_RESULT_H,&ExtRegData);//读取幅度信息，高2位
	*value = ((ExtRegData & 0x3) << 6);
	GetReg_Ext(JREG_LPCD_ADC_RESULT_L,&ExtRegData);//读取幅度信息，低6位
	*value += (ExtRegData & 0x3F);  
	return TRUE;
}

//LPCD获取校准值
unsigned char Lpcd_Get_Calibration_Value(unsigned char *value)
{
	unsigned char result;
	
	SetReg_Ext(JREG_LPCD_MISC,BFL_JBIT_CALIBRATE_VMID_EN);//使能VMID电源，BFL_JBIT_AMP_EN_SEL = 1 提前使能AMP	Lpcd_Reset_Status();  //清除CalibraIRq标志
	SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_CLR + JBIT_LPCD_CALIBRATE_EN);//关闭校准模式
	SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_SET+JBIT_LPCD_CALIBRATE_EN);//使能校准模式
	result = Lpcd_WaitFor_Irq(JBIT_CALIBRATE_IRQ);//等待校准结束中断      
	ModifyReg_Ext(JREG_LPCD_MISC,BFL_JBIT_CALIBRATE_VMID_EN,0);//关闭VMID电源 
	SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_CLR + JBIT_LPCD_CALIBRATE_EN);//关闭校准模式
	Lpcd_Get_Value(&*value);
	Lpcd_Reset_Status(); 
	return result;
}
//LPCD设置幅度包络信号的放大、衰减增益
unsigned char Lpcd_Set_Gain(unsigned char lpcd_gain)
{
	lpcd_gain &= 0x1F;
	SetReg_Ext(JREG_LPCD_CTRL4,lpcd_gain);
	return TRUE;
}

//LPCD设置校准基准电压
unsigned char Lpcd_Calibrate_Reference(void)
{
    unsigned char i,result;      
    Lpcd_Reset_Status();
    for(i = 4 ;i < 0x7F;i ++)
    {
		Lpcd.Calibration.Reference = 0 + i;//
		Lpcd_Set_Reference(LPCD_BIAS_CURRENT,Lpcd.Calibration.Reference);//
		Lpcd.Calibration.Gain_Index = 10;//

		Lpcd_Set_Gain(LPCD_GAIN[Lpcd.Calibration.Gain_Index]);
		
		Lpcd_Set_Driver(7,15,1);
		Lpcd_Get_Calibration_Value(&Lpcd.Calibration.Value);//
		if((Lpcd.Calibration.Value == 0)&&(Lpcd.Calibration.Reference != 0))
		{
			printk("Calibra Reference TRUE!\n");	
			result = TRUE;
			break;//
		}
		if((Lpcd.Calibration.Value == 0)&&(Lpcd.Calibration.Reference == 0))
		{
			printk("Calibra Reference FALSE!\n");	
			result = FALSE; //
			break;
		}
    }
   return result;
}

//LPCD设置校准输出驱动
unsigned char Lpcd_Calibrate_Driver(void)
{
	unsigned char i,j;   

	Lpcd.Calibration.Gain_Index = LPCD_GAIN_INDEX ;     

	for(j = 0;j < 12;j++)
	{ 
		Lpcd_Set_Gain(LPCD_GAIN[Lpcd.Calibration.Gain_Index]);
	
		for(i = 2;i < 7;i ++ )
		{
			Lpcd.Calibration.Driver_Index = i;
			Lpcd_Set_Driver(LPCD_P_DRIVER[Lpcd.Calibration.Driver_Index],LPCD_N_DRIVER[Lpcd.Calibration.Driver_Index],LPCD_TX2RF_EN);//
			Lpcd_Get_Calibration_Value(&Lpcd.Calibration.Value);//设置Driver

			if((Lpcd.Calibration.Value > Lpcd.Calibration.Range_L)&&(Lpcd.Calibration.Value < Lpcd.Calibration.Range_H))
			{
				if((Lpcd.Calibration.Value - (Lpcd.Fullscale_Value>>2) > 0 )&&((Lpcd.Calibration.Value + (Lpcd.Fullscale_Value>>2) ) < Lpcd.Fullscale_Value))//
				{
					Lpcd.Calibration.Threshold_Max = Lpcd.Calibration.Value + pn512_data->lpcd_threshold;
					Lpcd.Calibration.Threshold_Min = Lpcd.Calibration.Value - pn512_data->lpcd_threshold;
					
/* 					Lpcd.Calibration.Threshold_Max = Lpcd.Calibration.Value + LPCD_THRESHOLD;
					Lpcd.Calibration.Threshold_Min = Lpcd.Calibration.Value - LPCD_THRESHOLD; */
					
/* 				 	pr_err("-> Lpcd.Calibration.Value = 0x%x\n",Lpcd.Calibration.Value); 
					pr_err("  Lpcd.Fullscale_Value = 0x%x\n",Lpcd.Fullscale_Value); 
					pr_err("  Lpcd.Calibration.Threshold_Max = 0x%x\n",Lpcd.Calibration.Threshold_Max); 
					pr_err("  Lpcd.Calibration.Threshold_Min = 0x%x\n",Lpcd.Calibration.Threshold_Min);  */
					Lpcd_Set_Threshold(Lpcd.Calibration.Threshold_Min,Lpcd.Calibration.Threshold_Max);
					
					printk(KERN_INFO "Calibra Driver TRUE!\n");	        
					return TRUE;
				}
			}	 
		}

		if(Lpcd.Calibration.Value > (Lpcd.Fullscale_Value - (Lpcd.Fullscale_Value>>1)))//
		{
			if(Lpcd.Calibration.Gain_Index == 11)
				break;
			else
				Lpcd.Calibration.Gain_Index++;
				printk(KERN_INFO "Gain_Index++\n");
		}
		
		if(Lpcd.Calibration.Value < (Lpcd.Fullscale_Value>>2))//
		{     
			if(Lpcd.Calibration.Gain_Index == 0)
				break;
			else
				Lpcd.Calibration.Gain_Index--;
				printk(KERN_INFO "Gain_Index--\n");
		}
	}		
	printk(KERN_INFO "Calibra Driver FALSE !\n");
	return FALSE;
}

//LPCD设置Timer
unsigned char Lpcd_Set_Timer(void)
{
	Lpcd.Timer1 = pn512_data->lpcd_cfg_array[0] & 0x0F;//TIMER1 = 0x01~0x0F
	Lpcd.Timer2 = pn512_data->lpcd_cfg_array[1] & 0x1F;//TIMER2 = 0x01~0x1F
	Lpcd.Timer3 = pn512_data->lpcd_cfg_array[2] & 0x1F;//TIMER3 = 0x03~0x1F
	Lpcd.TimerVmid = LPCD_TIMER_VMID_CFG & 0x1F;
	
/* 	// Lpcd.Timer1 = LPCD_TIMER1_CFG & 0x0F;//TIMER1 = 0x01~0x0F
	// Lpcd.Timer2 = LPCD_TIMER2_CFG & 0x1F;//TIMER2 = 0x01~0x1F
	// Lpcd.Timer3 = LPCD_TIMER3_CFG & 0x1F;//TIMER3 = 0x03~0x1F */
	
	
    if (Lpcd.Timer3 > 0xF) //Timer3Cfg用到5bit，选择16分频
    {
		Lpcd.Timer3_Offset = 0x05;
		Lpcd.Timer3_Div  = 2;			//16分频
		Lpcd.Fullscale_Value =  ((Lpcd.Timer3 - 1)<<3) - Lpcd.Timer3_Offset;	
    }
	else if(Lpcd.Timer3 > 0x7) //Timer3Cfg用到4bit，选择8分频
	{
		Lpcd.Timer3_Offset = 0x0E;
		Lpcd.Timer3_Div  = 1;			//8分频
		Lpcd.Fullscale_Value =  ((Lpcd.Timer3 - 1)<<4) - Lpcd.Timer3_Offset;	
	}
	else 
	{
		Lpcd.Timer3_Offset = 0x1F;
		Lpcd.Timer3_Div  = 0;			//4分频
		Lpcd.Fullscale_Value =  ((Lpcd.Timer3 - 1)<<5) - Lpcd.Timer3_Offset;	
	}
	Lpcd.Calibration.Range_L = pn512_data->lpcd_threshold;
	Lpcd.Calibration.Range_H = Lpcd.Fullscale_Value - pn512_data->lpcd_threshold; 
	
/* 	Lpcd.Calibration.Range_L = LPCD_THRESHOLD;
	Lpcd.Calibration.Range_H = Lpcd.Fullscale_Value - LPCD_THRESHOLD;//  */
	
	SetReg_Ext(JREG_LPCD_T1CFG,(Lpcd.Timer3_Div <<4) + Lpcd.Timer1);//配置Timer1Cfg寄存器
	SetReg_Ext(JREG_LPCD_T2CFG,Lpcd.Timer2);//配置Timer2Cfg寄存器
	SetReg_Ext(JREG_LPCD_T3CFG,Lpcd.Timer3);//配置Timer3Cfg寄存器	
	SetReg_Ext(JREG_LPCD_VMIDBD_CFG,Lpcd.TimerVmid);//配置VmidBdCfg寄存器
	
    return TRUE;
}

//LCPD校准程序
unsigned char Lpcd_Calibration_Event(void)
{
	unsigned char result;
	
	//Lpcd_Init_Register();		LPCD寄存器初始化
	reg_write(JREG_COMMIEN,0x80);//IRQ引脚反相输出			 
	reg_write(JREG_DIVIEN,0x80);//IRQ引脚CMOS输出模式（IRQ引脚不需要外接上拉电阻） 
	
	SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_SET + JBIT_LPCD_EN);					//使能LPCD功能	
	//SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_SET + JBIT_LPCD_SENSE_1);				//配置1次检测就产生中断
	SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_CLR + JBIT_LPCD_SENSE_1);//3次
	SetReg_Ext(JREG_LPCD_CTRL3,0 << 3);												//配置LPCD工作模式	
	SetReg_Ext(JREG_LPCD_AUTO_WUP_CFG,(LPCD_AUTO_WUP_EN << 3) + LPCD_AUTO_WUP_CFG );//配置自动唤醒寄存器
	
	Lpcd_Set_Timer();
	//Lpcd_Set_Aux()
	SetReg_Ext(0x13,0x00);
	SetReg_Ext(0x14,0x00);
	SetReg_Ext(0x15,0x00);	
	SetReg_Ext(0x16,0x00);	
	reg_write(JREG_TESTPINEN,0x80);//关闭D1；D2 输出

	Lpcd_Reset_Status();
	
	printk(KERN_INFO "Start Calibration!\n");    
	
	result = Lpcd_Calibrate_Reference();
    if(result == FALSE)
		return FALSE;
   
	result = Lpcd_Calibrate_Driver();
	if (result == TRUE){	
		printk(KERN_INFO "Calibration TRUE!\n");
/* 		pr_err("\n-> TRUE Lpcd.Calibration.Reference:0x%x!\n",Lpcd.Calibration_Backup.Reference);		
		pr_err("-> TRUE Lpcd.Calibration.Driver_Index:0x%x!\n",Lpcd.Calibration_Backup.Driver_Index);		
		pr_err("-> TRUE Lpcd.Calibration_Backup.Gain_Index:0x%x!\n\n",Lpcd.Calibration_Backup.Gain_Index); */
		return TRUE;
	} 
	else{
 		printk(KERN_INFO "Calibration FALSE!\n");
/*		pr_err("\n-> FALE Lpcd.Calibration.Reference:0x%x!\n",Lpcd.Calibration.Reference);		
		pr_err("-> FALE Lpcd.Calibration.Driver_Index:0x%x!\n",Lpcd.Calibration.Driver_Index);		
		pr_err("-> FALE Lpcd.Calibration_Backup.Gain_Index:0x%x!\n\n",Lpcd.Calibration.Gain_Index);	 */
		//Lpcd_Calibration_Restore校准复位
		Lpcd.Calibration.Reference = Lpcd.Calibration_Backup.Reference;
		Lpcd.Calibration.Gain_Index = Lpcd.Calibration_Backup.Gain_Index;
		Lpcd.Calibration.Driver_Index = Lpcd.Calibration_Backup.Driver_Index;
	}	 

	return FALSE;
}

//第一次校准
static	void start_calibration(void)
{	
	if(Lpcd_Calibration_Event()== 1)//进行FM175XX校准		
		Lpcd_Calibration_Backup();	
}

//LPCD读取中断标志
void Lpcd_Get_IRQ(unsigned char *irq)
{
  	GetReg_Ext(JREG_LPCD_IRQ, &(*irq));//读取LPCD中断寄存器

	printk(KERN_INFO "-> Lpcd.Irq = 0x%x",*irq); 
	if(Lpcd.Irq & JBIT_CARD_IN_IRQ)
	{
		Lpcd_Get_Value(&Lpcd.Value);
		printk(KERN_INFO "; Lpcd.Value = 0x%x\n\n",Lpcd.Value);
	}
	Lpcd_Reset_Status();
	return;
}

//禁用中断（关闭）
static void pn512_disable_irq(struct pn512_dev *p_dev)
{
	unsigned long flags;
	spin_lock_irqsave(&p_dev->irq_enabled_lock, flags);
	if (p_dev->irq_enabled) {
		p_dev->irq_enabled = FALSE;
		disable_irq_nosync(p_dev->client->irq);
		//disable_irq(p_dev->client->irq);
	}
	spin_unlock_irqrestore(&p_dev->irq_enabled_lock, flags);
}

//设置LPCD状态
static void pn512_lpcd_SetMod(unsigned char mode,struct pn512_dev *p_dev)
{
	unsigned char sirq;
	
	if(mode)
	{	
		pn512_powerdown(0);
		
		//IRQ_EVENT
		//Lpcd_Calibration_Event();//LPCD校准

		Lpcd_Set_Driver(LPCD_P_DRIVER[Lpcd.Calibration.Driver_Index],LPCD_N_DRIVER[Lpcd.Calibration.Driver_Index],LPCD_TX2RF_EN);//配置LPCD输出驱动
		Lpcd_Set_IRQ_pin();
		
		reg_write(ComIEnReg, 0x80);                                  
		reg_write(DivIEnReg, 0x80);
		reg_write(ComIrqReg,0x7f); 
		SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_SET + JBIT_LPCD_IE);	//读卡器打开LPCD中断 
		
		Lpcd_Get_IRQ(&sirq); 
		
		pn512_powerdown(1);

		pn512_data->irq_enabled = TRUE;
		enable_irq(p_dev->client->irq);

		//pr_err("LPCD Mode Entered!\n");
	}
	else
	{   
		disable_irq(p_dev->client->irq);
		pn512_data->irq_enabled = FALSE;
		
		pn512_powerdown(0);
		SetReg_Ext(JREG_LPCD_CTRL1,JBIT_BIT_CTRL_CLR + JBIT_LPCD_IE);	//关闭LPCD中断
		//pr_err("LPCD Mode Exited!\n");  
	}     
    return;
}


//中断处理函数
static irqreturn_t pn512_irq_handler(int irq, void *dev_id)
{
	struct pn512_dev *p_dev = dev_id;
	//disable_irq_nosync(p_dev->client->irq);

	//pr_err("nfc irq wake up %d\n", irq);  
	
	if(p_dev->irq_enabled){
		p_dev->flag = TRUE;
		wake_up_interruptible(&p_dev->read_wq);
		pr_info("nfc irq wake up\n");  
	} 
	
	//enable_irq(p_dev->client->irq);
	return IRQ_HANDLED;
}

static ssize_t pn512_dev_read(struct file *filp, char __user *buf,size_t count, loff_t *offset)
{
	//unsigned char tmp[FIFO_MAX_SIZE] = {'p','a','s','s','\0'};
	struct pn512_dev *p_dev = filp->private_data;
	int ret = 0;
	unsigned char irq;

 	p_dev->flag = FALSE;
	p_dev->irq_enabled = TRUE;
	
 	//等待中断唤醒超时1000ms，
	//返回值：0超时 -ERESTARTSYS(-512)被信号中断 1条件成立
	ret = wait_event_interruptible_timeout(p_dev->read_wq, p_dev->flag, msecs_to_jiffies(1000));

	// 中断唤醒后，读取LPCD中断寄存器，判断是否检测到IC卡
	if(ret > 0){
		pn512_powerdown(0);
		// 获取IRQ中断状态
		GetReg_Ext(JREG_LPCD_IRQ, &irq);//读取LPCD中断寄存器
		if(!(irq & (1<<0))){
			ret = 0;
		}
		Lpcd_Reset_Status();
		pn512_powerdown(1);
	}
	
	return ret;
	
/* 	//需要往上层传递数据再打开
	wait_event_interruptible(p_dev->read_wq,p_dev->flag);
	
	
 	 if(count >FIFO_MAX_SIZE)
		count = FIFO_MAX_SIZE;	 
	ret = copy_to_user(buf,tmp,count);
	count -= ret;
	p_dev->flag = FALSE; 
	return count;
	 */
}

static ssize_t pn512_dev_write(struct file *filp, const char __user *buf,size_t count, loff_t *offset)
{
// 	struct pn512_dev  *p_dev = filp->private_data;;
// 	
// 	if (count > FIFO_MAX_SIZE)
// 		count = FIFO_MAX_SIZE;
// 
// 	mutex_lock(&p_dev->i2c_mutex);
// 
// 	if (copy_from_user(p_dev->tx_buf, buf, count)) {
// 		pr_err("%s : failed to copy from user space\n", __func__);
// 		return -EFAULT;
// 	}
// 	p_dev->tx_length = count;
// 
// 	mutex_unlock(&p_dev->i2c_mutex);

	return count;
}

static int pn512_dev_open(struct inode *inode, struct file *filp)
{
	struct pn512_dev *p_dev = container_of(filp->private_data,struct pn512_dev,misc_dev);

	filp->private_data = p_dev;

	pr_debug("%s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	pn512_hwreset();//硬件上电复位

	return 0;
}

static int pn512_dev_close(struct inode *inode, struct file *filp)
{
	struct pn512_dev *p_dev = filp->private_data;

	mutex_lock(&pn512_data->io_mutex);
	if(p_dev->irq_enabled){
		p_dev->irq_enabled = FALSE;
		disable_irq(p_dev->client->irq);
	}
	mutex_unlock(&p_dev->io_mutex);
	//进入低功耗模式
	pn512_powerdown(1);
	
	return 0;
}

long  pn512_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct pn512_reg reg;
	int res = 0;
	
	//pr_err("%s :enter cmd = %u, arg = %ld\n", __func__, cmd, arg);

	mutex_lock(&pn512_data->io_mutex);
	switch (cmd){
	case PN512_IOC_REGWRITE:
		if(copy_from_user(&reg, (struct pn512_reg *)arg, sizeof(struct pn512_reg))) {
			pr_err("%s %d: failed to copy from user space\n", __func__, __LINE__);
			res = -EFAULT;
			goto err;
		}
		res = reg_write(reg.address, reg.value);
		break;
	case PN512_IOC_REGREAD:
		if (copy_from_user(&reg, (struct pn512_reg *)arg, sizeof(struct pn512_reg))) {
			pr_err("%s %d: failed to copy from user space\n", __func__, __LINE__);
			res = -EFAULT;
			goto err;
		}
		reg.value = reg_read(reg.address);
		if(copy_to_user((struct pn512_reg *)arg, &reg, sizeof(struct pn512_reg))) {
			pr_err("%s %d: failed to copy to user space\n", __func__, __LINE__);
			res = -EFAULT;
			goto err;
		}
		break;
	case PN512_IOC_REGSBIT:
		if (copy_from_user(&reg, (struct pn512_reg *)arg, sizeof(struct pn512_reg))) {
			pr_err("%s %d: failed to copy from user space\n", __func__, __LINE__);
			res = -EFAULT;
			goto err;
		}
		res = reg_setbit(reg.address, reg.value);
		break;
	case PN512_IOC_REGCBIT:
		if (copy_from_user(&reg, (struct pn512_reg *)arg, sizeof(struct pn512_reg))) {
			pr_err("%s %d: failed to copy from user space\n", __func__, __LINE__);
			res = -EFAULT;
			goto err;
		}
		res = reg_clearbit(reg.address, reg.value);
		break;
	case PN512_IOC_TXDATA:
		if (copy_from_user(&pn512_tx, (struct pn512_transfer_tx *)arg, sizeof(struct pn512_transfer_tx))) {
			pr_err("%s %d: failed to copy from user space\n", __func__, __LINE__);
			res = -EFAULT;
			goto err;
		}
		res = pn512_tx.length;
		break;
	case PN512_IOC_RXDATA:
		res = pn512_comm(&pn512_tx, &pn512_rx);
		if (copy_to_user((struct pn512_transfer_rx *)arg, &pn512_rx, sizeof(struct pn512_transfer_rx))) {
			pr_err("%s %d: failed to copy to user space\n", __func__, __LINE__);
			res = -EFAULT;
			goto err;
		}
		break;
	case PN512_IOC_IRQENABLE:
		if(!pn512_data->irq_enabled) {
			pr_err("### on");
			enable_irq(pn512_data->client->irq);
			pn512_data->irq_enabled = TRUE;
		}
		break;
	case PN512_IOC_IRQDISABLE:
		if(pn512_data->irq_enabled) {
			pr_err("### off");
			pn512_data->irq_enabled = FALSE;
			disable_irq(pn512_data->client->irq);
		}
		break;
	case PN512_IOC_ISOTYPE:
		set_isotype((unsigned char)arg);
		break;
	case PN512_IOC_RFMODE:
		set_rfmode((unsigned char)arg);
		break;
	case PN512_IOC_SETTIMER:
		set_timer((unsigned int)arg);
		break;
	case PN512_IOC_HWRESET:
		pn512_hwreset();
		break;
	case PN512_IOC_SWRESET:
		pn512_swreset();
		break;
	case PN512_IOC_SETPWD:
		pn512_powerdown((int)arg);
		break;
	case PN512_IOC_LPCD:
		pn512_lpcd_SetMod((unsigned char)arg,pn512_data);
		break;
	case PN512_IOC_STARTCAL:
		start_calibration();
		break;
	}

err:
	mutex_unlock(&pn512_data->io_mutex);

	return res;
}

static const struct file_operations pn512_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = pn512_dev_read,
	.write = pn512_dev_write,
	.open = pn512_dev_open,
	.release = pn512_dev_close,
	.unlocked_ioctl = pn512_dev_ioctl,
};


static int setup_gpio(struct pn512_dev *p_dev)
{
	int irq, err = -EIO;p_dev->irq_enabled = FALSE;pn512_data->flag = FALSE;
	
	err = gpio_request(p_dev->irq_gpio, "pn512-irq");
	if (err < 0) {
		pr_err("%s: irq gpio request, err=%d", __func__, err);
		goto err_irq_gpio_request;
	}
	err = gpio_direction_input(p_dev->irq_gpio);
	if (err < 0) {
		pr_err("%s: npd gpio direction, err=%d", __func__, err);
		goto err_irq_gpio_direction;
	}

	//获取GPIO的中断号
	irq = gpio_to_irq(p_dev->irq_gpio);
	if (irq <= 0)
	{
		pr_err("irq number is not specified, irq # = %d, int pin=%d\n", irq, p_dev->irq_gpio);
		return irq;
	}
	
	p_dev->client->irq = irq;//保存中断号

	//配置IO中断为下降沿触发
	err = request_irq(irq, pn512_irq_handler, IRQF_TRIGGER_FALLING, DEVICE_NAME, p_dev);
	if (err < 0)
	{
		pr_err("%s: request_irq(%d) failed for (%d)\n", __func__, irq, err);
		goto err_request_irq;
	}

	//enable_irq(irq);
	disable_irq(irq);
	pn512_data->irq_enabled = FALSE;
	
	//初始化管脚
	err = gpio_request(p_dev->npd_gpio, "pn512-npd");
	if (err < 0) {
		pr_err("%s: npd gpio request, err=%d", __func__, err);
		goto err_npd_gpio_request;
	}
	err = gpio_direction_output(p_dev->npd_gpio, 1);//拉高NPD
	if (err < 0) {
		pr_err("%s: npd gpio direction, err=%d", __func__, err);
		goto err_npd_gpio_direction;
	}

	return 0;

err_npd_gpio_direction:
	gpio_free(p_dev->npd_gpio);
err_npd_gpio_request:
	free_irq(p_dev->client->irq, p_dev);
err_request_irq:
err_irq_gpio_direction:
	gpio_free(p_dev->irq_gpio);
err_irq_gpio_request:

	return err;
}

static void free_gpio(struct pn512_dev *p_dev)
{
	free_irq(p_dev->client->irq, p_dev);
	gpio_free(p_dev->irq_gpio);
	gpio_free(p_dev->npd_gpio);
}

#ifdef CONFIG_OF
static int parse_dt(struct device *dev, struct pn512_dev *p_dev)
{
	struct device_node *node = dev->of_node;
	int ret;

	p_dev->npd_gpio = of_get_named_gpio_flags(node, "npd-gpio", 0, (enum of_gpio_flags *)&p_dev->npd_valid);
	if (p_dev->npd_gpio < 0) {
		dev_err(dev, "Unable to read npd-gpio\n");
		return -1;
	}

	p_dev->irq_gpio = of_get_named_gpio(node, "irq-gpio", 0);
	if (p_dev->irq_gpio < 0) {
		dev_err(dev, "Unable to read irq-gpio\n");
		return -2;
	}

	dev_err(dev, "###get irq gpio %d\n", p_dev->irq_gpio);

	ret = of_property_read_u32_array(node,"lpcd_cfg_array",p_dev->lpcd_cfg_array,3);
	if(ret)
		dev_err(dev, "get lpcd_cfg_array failed :%d\n",ret);

	ret = of_property_read_u32(node,"lpcd_threshold",&p_dev->lpcd_threshold);
	if(ret)
		dev_err(dev, "get dts lpcd_threshold failed :%d\n",ret);
	
	return 0;
}
#else
static int parse_dt(struct device *dev, struct pn512_dev *p_dev)
{
	dev_err(dev, "no platform data defined\n");

	return ERR_PTR(-EINVAL);
}
#endif

static int pn512_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct pn512_dev *p_dev;
	int res;
	
//DECLEAR_WAITQUEUE(wait,current);
	
	printk(KERN_INFO "%s: driver version = %s\n", __func__, DRIVER_VERSION);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

	pn512_data = kzalloc(sizeof(struct pn512_dev), GFP_KERNEL);
	if (pn512_data == NULL) {
		dev_err(&client->dev, "failed to allocate memory for module data\n");
		res = -ENOMEM;
		goto err_exit;
	}

	p_dev = pn512_data;
	p_dev->client = client;
	i2c_set_clientdata(client, p_dev);

	//从结构树获取参数
	res = parse_dt(&client->dev, p_dev);
	if (res)
	{
		dev_err(&client->dev, "%s: parse_dt res=%d\n", __func__, res);
		goto parse_dt_err;
	}

	/* init mutex and queues */
	init_waitqueue_head(&p_dev->read_wq);//等待队列
//sema_init (&p_dev->sem,1);//信号量
	mutex_init(&p_dev->i2c_mutex);
	mutex_init(&p_dev->io_mutex);
	spin_lock_init(&p_dev->irq_enabled_lock);
//add_wait_queue(&p_dev->read_wq,&wait);
	
	p_dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
	p_dev->misc_dev.name = DEVICE_NAME;
	p_dev->misc_dev.fops = &pn512_dev_fops;

	res = misc_register(&p_dev->misc_dev);
	if (res) {
		pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}
	
	wake_lock_init(&nfc_wake_lock, WAKE_LOCK_SUSPEND, "NFCWAKE");

	//Pinctrl设置
	p_dev->pctrl = devm_pinctrl_get(&client->dev);

	if (IS_ERR(p_dev->pctrl))
		dev_err(&client->dev, "pinctrl get failed\n");
	else
	{
		p_dev->pctrl_active = pinctrl_lookup_state(p_dev->pctrl, "default");
		if (IS_ERR(p_dev->pctrl_active))
			dev_err(&client->dev, "pinctrl get state failed\n");
		else
			pinctrl_select_state(p_dev->pctrl, p_dev->pctrl_active);
	}
	
	//设置GPIO
	res=setup_gpio(p_dev);

	//pn512_powerdown(1);
	printk(KERN_INFO  "%s: probe successfully\n", __func__);

	return 0;
	
err_misc_register:
	mutex_destroy(&p_dev->io_mutex);
	mutex_destroy(&p_dev->i2c_mutex);
	//remove_wait_queue(&p_dev->read_wq,&wait);
	kfree(p_dev);
parse_dt_err:
	kfree(p_dev);
err_exit:
	
	return res;
}

static int pn512_remove(struct i2c_client *client)
{
	struct pn512_dev *p_dev;
	p_dev = i2c_get_clientdata(client);
	wake_lock_destroy(&nfc_wake_lock);//销毁锁，解决卸载时内核崩溃
	free_gpio(p_dev);
	misc_deregister(&p_dev->misc_dev);
	mutex_destroy(&p_dev->io_mutex);
	mutex_destroy(&p_dev->i2c_mutex);
	kfree(p_dev);	
	return 0;
}

static const struct i2c_device_id pn512_id[] = {
	{ "pn512", 0 },
	{}
};

static struct of_device_id pn512_i2c_dt_match[] = {
	{ .compatible = "nxp,pn512", },
	{},
};

static struct i2c_driver pn512_driver = {
	.id_table = pn512_id,
	.probe = pn512_probe,
	.remove = pn512_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
		.of_match_table = pn512_i2c_dt_match,
	},
};

/*
* module load/unload record keeping
*/
static int __init pn512_dev_init(void)
{
	pr_info("Loading pn512 driver\n");
	return i2c_add_driver(&pn512_driver);
}

static void __exit pn512_dev_exit(void)
{
	pr_info("Unloading pn512 driver\n");
	i2c_del_driver(&pn512_driver);
}

module_init(pn512_dev_init);
module_exit(pn512_dev_exit);

MODULE_DESCRIPTION("NFC PN512 driver");
MODULE_LICENSE("GPL");
