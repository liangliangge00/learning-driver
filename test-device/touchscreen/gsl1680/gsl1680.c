/* drivers/input/touchscreen/gslX68X.h
 * 
 * 2010 - 2013 SLIEAD Technology.
 *    
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the SLIEAD's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/byteorder/generic.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/platform_device.h>

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND) 
#include <linux/earlysuspend.h>
#endif

#include <linux/firmware.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h> 
#include <linux/of_gpio.h>

#include "gsl1680.h"

#ifdef GSL_REPORT_POINT_SLOT
    #include <linux/input/mt.h>
#endif
//subin #include <linux/productinfo.h>

/* Timer Function */
#ifdef GSL_TIMER
#define GSL_TIMER_CHECK_CIRCLE 	200
static struct delayed_work gsl_timer_check_work;
static struct workqueue_struct *gsl_timer_workqueue = NULL;
static char int_1st[4];
static char int_2nd[4];
#endif

/* Gesture Resume */
#ifdef GSL_GESTURE

#include <linux/wakelock.h>

typedef enum {
	GE_DISABLE = 0,
	GE_ENABLE = 1,
	GE_WAKEUP = 2,
	GE_NOWORK =3,
}GE_T;
static GE_T gsl_gesture_status = GE_DISABLE;
static volatile unsigned int gsl_gesture_flag = 1;
static char gsl_gesture_c = 0;
struct wake_lock gsl_wake_lock;
#endif

/* 
	If you want to use touchscreen wake up
	add "gsl,irq-wake = <1>" in your dtsi file.
	--shenxj add
*/
#ifdef GSL_IRQ_WAKE
#include <linux/pm.h>
#include <linux/time.h>

long tp_down_time;
char flag_tp_down = 0;
char gsl_irq_wake_en = 0;
char gsl_irq_wake_justnow = 0;
#endif

	
/* Process for Android Debug Bridge */
#ifdef TPD_PROC_DEBUG
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
static struct proc_dir_entry *gsl_config_proc = NULL;
#define GSL_CONFIG_PROC_FILE 	"gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag = 0;
#endif


#define GSL_TEST_TP
#ifdef GSL_TEST_TP
extern void gsl_write_test_config(unsigned int cmd,int value);
extern unsigned int gsl_read_test_config(unsigned int cmd);
extern int gsl_obtain_array_data_ogv(short *ogv,int i_max,int j_max);
extern int gsl_obtain_array_data_dac(unsigned int *dac,int i_max,int j_max);
extern int gsl_tp_module_test(char *buf,int size);
#define GSL_PARENT_PROC_NAME 	"touchscreen"
#define GSL_OPENHSORT_PROC_NAME 	"ctp_openshort_test"
#define GSL_RAWDATA_PROC_NAME 	"ctp_rawdata"
#endif

/*define global variable*/
static struct gsl_ts_data *ts_data = NULL;

static unsigned int GSL_MAX_X = 800;
static unsigned int GSL_MAX_Y = 1280;

static void gsl_sw_init(struct i2c_client *client);

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self, 
				unsigned long event, void *data);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void gsl_early_suspend(struct early_suspend *handler);
static void gsl_early_resume(struct early_suspend *handler);
#endif


#ifdef TOUCH_VIRTUAL_KEYS
static ssize_t virtual_keys_show(struct kobject *kobj, 
				struct kobj_attribute *attr, char *buf)
{

	return sprintf(buf,
         __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":120:900:40:40"
	 ":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":240:900:40:40"
	 ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":360:900:40:40"
	 "\n");
}

static struct kobj_attribute virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.GSL_TP",
		.mode = S_IRUGO,
	},
	.show = &virtual_keys_show,
};

static struct attribute *properties_attrs[] = {
	&virtual_keys_attr.attr,
	NULL
};

static struct attribute_group properties_attr_group = {
	.attrs = properties_attrs,
};

static void gsl_ts_virtual_keys_init(void)
{
	int ret;
	struct kobject *properties_kobj;

	dev_info("%s\n", __func__);

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		ret = sysfs_create_group(properties_kobj,
			&properties_attr_group);
	if (!properties_kobj || ret)
		pr_err("failed to create board_properties\n");
}
#endif

static int gsl_ts_read(struct i2c_client *client, 
					u8 reg, u8 *buf, u32 num)
{
	int err = 0;
	u8 temp = reg;
	mutex_lock(&ts_data->gsl_i2c_lock);
	if(temp < 0x80)
	{
		temp = (temp+8)&0x5c;
			i2c_master_send(client,&temp,1);	
			err = i2c_master_recv(client,&buf[0],4);
			temp = reg;
			i2c_master_send(client,&temp,1);	
			err = i2c_master_recv(client,&buf[0],4);
	}
	i2c_master_send(client,&reg,1);
	err = i2c_master_recv(client,&buf[0],num);
	mutex_unlock(&ts_data->gsl_i2c_lock);
	return (err == num)?1:-1;
}

static int gsl_read_interface(struct i2c_client *client, 
				u8 reg, u8 *buf, u32 num)
{

	int err = 0;
	u8 temp = reg;
	mutex_lock(&ts_data->gsl_i2c_lock);
	if(temp < 0x80)
	{
		temp = (temp+8)&0x5c;
		err = i2c_master_send(client,&temp,1);
		if(err < 0) {
			goto err_i2c_transfer;
		}
		err = i2c_master_recv(client,&buf[0],4);
		if(err < 0) {
			goto err_i2c_transfer;
		}
		temp = reg;
		err = i2c_master_send(client,&temp,1);
		if(err < 0) {
			goto err_i2c_transfer;
		}
		err = i2c_master_recv(client,&buf[0],4);
		if(err < 0) {
			goto err_i2c_transfer;
		}
	}
	err = i2c_master_send(client,&reg,1);
	if(err < 0) {
		goto err_i2c_transfer;
	}
	err = i2c_master_recv(client,&buf[0],num);
	if(err != num) {
		err = -1;
		goto err_i2c_transfer;
	}
	mutex_unlock(&ts_data->gsl_i2c_lock);
	return 1;
err_i2c_transfer:
	mutex_unlock(&ts_data->gsl_i2c_lock);
	return err;
}

static int gsl_write_interface(struct i2c_client *client, 
			const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];
	int err;
	u8 tmp_buf[num+1];
	tmp_buf[0] = reg;
	memcpy(tmp_buf + 1, buf, num);
	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = tmp_buf;
	//xfer_msg[0].timing = 400;

	mutex_lock(&ts_data->gsl_i2c_lock);
	err= i2c_transfer(client->adapter, xfer_msg, 1);
	mutex_unlock(&ts_data->gsl_i2c_lock);

	return err;	
}

#ifdef GSL_GESTURE
static unsigned int gsl_read_oneframe_data(unsigned int *data,
				unsigned int addr,unsigned int len)
{
	u8 buf[4];
	int i;
	printk("tp-gsl-gesture %s\n",__func__);
	printk("gsl_read_oneframe_data:::addr=%x,len=%x\n",addr,len);
	for(i=0;i<len/2;i++){
		buf[0] = ((addr+i*8)/0x80)&0xff;
		buf[1] = (((addr+i*8)/0x80)>>8)&0xff;
		buf[2] = (((addr+i*8)/0x80)>>16)&0xff;
		buf[3] = (((addr+i*8)/0x80)>>24)&0xff;
		gsl_write_interface(ts_data->client,0xf0,buf,4);
		gsl_read_interface(ts_data->client,(addr+i*8)%0x80,(char *)&data[i*2],8);
	}
	if(len%2){
		buf[0] = ((addr+len*4 - 4)/0x80)&0xff;
		buf[1] = (((addr+len*4 - 4)/0x80)>>8)&0xff;
		buf[2] = (((addr+len*4 - 4)/0x80)>>16)&0xff;
		buf[3] = (((addr+len*4 - 4)/0x80)>>24)&0xff;
		gsl_write_interface(ts_data->client,0xf0,buf,4);
		gsl_read_interface(ts_data->client,(addr+len*4 - 4)%0x80,(char *)&data[len-1],4);
	}
	#if 1
	for(i=0;i<len;i++){
	printk("gsl_read_oneframe_data =%x\n",data[i]);	
	//printk("gsl_read_oneframe_data =%x\n",data[len-1]);
	}
	#endif
	
	return len;
}
#endif

static void gsl_load_fw(struct i2c_client *client,
				const struct fw_data *GSL_DOWNLOAD_DATA,int data_len)
{
	u8 buf[4] = {0};
	//u8 send_flag = 1;
	u8 addr=0;
	u32 source_line = 0;
	u32 source_len = data_len;//ARRAY_SIZE(GSL_DOWNLOAD_DATA);

	dev_info("=============gsl_load_fw start==============\n");
	for (source_line = 0; source_line < source_len; source_line++) {
		/* init page trans, set the page val */
		addr = (u8)GSL_DOWNLOAD_DATA[source_line].offset;	//获取写的偏移地址
		memcpy(buf, &GSL_DOWNLOAD_DATA[source_line].val, 4);//将要写的4字节复制到缓冲区
		gsl_write_interface(client, addr, buf, 4);		//写入芯片寄存器
	}
	dev_info("=============gsl_load_fw end==============\n");
}

static void gsl_io_control(struct i2c_client *client)
{
#if GSL9XX_VDDIO_1800
	u8 buf[4] = {0};
	int i;
	for(i=0;i<5;i++){
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0xfe;
		buf[3] = 0x1;
		gsl_write_interface(client,0xf0,buf,4);
		buf[0] = 0x5;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0x80;
		gsl_write_interface(client,0x78,buf,4);
		msleep(5);
	}
	msleep(50);
#endif
}

static void gsl_start_core(struct i2c_client *client)
{
	//u8 tmp = 0x00;
	u8 buf[4] = {0};
	buf[0]=0;
	gsl_write_interface(client,0xe0,buf,4);
	
#ifdef GSL_ALG_ID
	gsl_DataInit(gsl_cfg_table[gsl_cfg_index].data_id);
#endif	
}

static void gsl_reset_core(struct i2c_client *client)
{
	u8 buf[4] = {0x00};
	
	buf[0] = 0x88;
	gsl_write_interface(client,0xe0,buf,4);
	msleep(5);

	buf[0] = 0x04;
	gsl_write_interface(client,0xe4,buf,4);
	msleep(5);
	
	buf[0] = 0;
	gsl_write_interface(client,0xbc,buf,4);
	msleep(5);

	gsl_io_control(client);
}

static void gsl_clear_reg(struct i2c_client *client)
{
	u8 buf[4]={0};
	//clear reg
	buf[0]=0x88;
	gsl_write_interface(client,0xe0,buf,4);
	msleep(20);
	buf[0]=0x3;
	gsl_write_interface(client,0x80,buf,4);
	msleep(5);
	buf[0]=0x4;
	gsl_write_interface(client,0xe4,buf,4);
	msleep(5);
	buf[0]=0x0;
	gsl_write_interface(client,0xe0,buf,4);
	msleep(20);
	//clear reg

}

#ifdef GSL_TEST_TP
void gsl_I2C_ROnePage(unsigned int addr, char *buf)
{
	u8 tmp_buf[4]={0};
	tmp_buf[3]=(u8)(addr>>24);
	tmp_buf[2]=(u8)(addr>>16);
	tmp_buf[1]=(u8)(addr>>8);
	tmp_buf[0]=(u8)(addr);
	gsl_write_interface(ts_data->client,0xf0,tmp_buf,4);
	gsl_ts_read(ts_data->client,0,buf,128);
}
EXPORT_SYMBOL(gsl_I2C_ROnePage);

void gsl_I2C_RTotal_Address(unsigned int addr,unsigned int *data)
{
	u8 tmp_buf[4]={0};	
	tmp_buf[3]=(u8)((addr/0x80)>>24);
	tmp_buf[2]=(u8)((addr/0x80)>>16);
	tmp_buf[1]=(u8)((addr/0x80)>>8);
	tmp_buf[0]=(u8)((addr/0x80));
	gsl_write_interface(ts_data->client,0xf0,tmp_buf,4);
	gsl_ts_read(ts_data->client,addr%0x80,tmp_buf,4);
	*data = tmp_buf[0]|(tmp_buf[1]<<8)|(tmp_buf[2]<<16)|(tmp_buf[3]<<24);
}
EXPORT_SYMBOL(gsl_I2C_RTotal_Address);
#endif

#ifdef TPD_PROC_DEBUG
static int char_to_int(char ch)
{
	if(ch>='0' && ch<='9')
		return (ch-'0');
	else
		return (ch-'a'+10);
}

//static int gsl_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
static int gsl_config_read_proc(struct seq_file *m,void *v)
{
	char temp_data[5] = {0};
	unsigned int tmp=0;
	
	if('v'==gsl_read[0]&&'s'==gsl_read[1])
	{
#ifdef GSL_ALG_ID
		tmp=gsl_version_id();
#else 
		tmp=0x20121215;
#endif
		seq_printf(m,"version:%x\n",tmp);
	}
	else if('r'==gsl_read[0]&&'e'==gsl_read[1])
	{
		if('i'==gsl_read[3])
		{
#ifdef GSL_ALG_ID 
			tmp=(gsl_data_proc[5]<<8) | gsl_data_proc[4];
			seq_printf(m,"gsl_config_data_id[%d] = ",tmp);
			if(tmp>=0&&tmp<gsl_cfg_table[gsl_cfg_index].data_size)
				seq_printf(m,"%d\n",gsl_cfg_table[gsl_cfg_index].data_id[tmp]); 
#endif
		}
		else 
		{
			printk("GSL***%s*****1 = %02x, 2 = %02x, 3= %02x , 4 = %02x,***\n",__func__,
				gsl_data_proc[0],gsl_data_proc[1],gsl_data_proc[2],gsl_data_proc[3]);

			tmp = (gsl_data_proc[7]<<24) + (gsl_data_proc[6]<<16) + (gsl_data_proc[5]<<8) + gsl_data_proc[4];
			if(tmp >= 1 && gsl_data_proc[0] == 0x7c)
			{
				tmp -= 1;
				gsl_data_proc[7] = (tmp>>24) & 0xff;
				gsl_data_proc[6] = (tmp>>16) & 0xff;
				gsl_data_proc[5] = (tmp>> 8) & 0xff;
				gsl_data_proc[4] = (tmp>> 0) & 0xff;
			}
			gsl_write_interface(ts_data->client,0Xf0,&gsl_data_proc[4],4);

			if(gsl_data_proc[0] < 0x80)
				gsl_ts_read(ts_data->client,gsl_data_proc[0],temp_data,4);

			gsl_ts_read(ts_data->client,gsl_data_proc[0],temp_data,4);

			printk("GSL***%s*****1 = %02x, 2 = %02x, 3= %02x , 4 = %02x,***\n",
					__func__, temp_data[0], temp_data[1], temp_data[2],	temp_data[3]);
			seq_printf(m,"offset : {0x%02x,0x",gsl_data_proc[0]);
			seq_printf(m,"%02x",temp_data[3]);
			seq_printf(m,"%02x",temp_data[2]);
			seq_printf(m,"%02x",temp_data[1]);
			seq_printf(m,"%02x};\n",temp_data[0]);
		}
	}
//	*eof = 1;
	return (0);
}

static ssize_t gsl_config_write_proc(struct file *file, 
		const char __user *buffer, size_t count, loff_t *data)
{
	u8 buf[8] = {0};
	char temp_buf[CONFIG_LEN];
	char *path_buf;
	int tmp = 0;
	int tmp1 = 0;
	dev_info("[tp-gsl][%s] \n",__func__);


	if(count > 512)
	{
		dev_info("size not match [%d:%d]\n", CONFIG_LEN, (int)count);
        return -EFAULT;
	}
	path_buf=kzalloc(count,GFP_KERNEL);
	if(!path_buf)
	{
		printk("alloc path_buf memory error \n");
		return -1;
	}
	if(copy_from_user(path_buf, buffer, count))
	{
		dev_info("copy from user fail\n");
		goto exit_write_proc_out;
	}
	memcpy(temp_buf,path_buf,(count<CONFIG_LEN?count:CONFIG_LEN));
	dev_info("[tp-gsl][%s][%s]\n",__func__,temp_buf);
	printk("[tp-GSL][%s][%s]\n",__func__,temp_buf);	

	buf[3]=char_to_int(temp_buf[14])<<4 | char_to_int(temp_buf[15]);	
	buf[2]=char_to_int(temp_buf[16])<<4 | char_to_int(temp_buf[17]);
	buf[1]=char_to_int(temp_buf[18])<<4 | char_to_int(temp_buf[19]);
	buf[0]=char_to_int(temp_buf[20])<<4 | char_to_int(temp_buf[21]);

	printk("GSL*****%s**buf[0] = %d,buf[1] = %d,buf[2] = %d,buf[3] = %d,"
		"buf[4] = %d,buf[5] = %d ,buf[6] = %d, buf[7] = %d **\n",
			__func__, buf[0], buf[1], buf[2], buf[3], 
				buf[4], buf[5], buf[6], buf[7]);	
		
	buf[7]=char_to_int(temp_buf[5])<<4 | char_to_int(temp_buf[6]);
	buf[6]=char_to_int(temp_buf[7])<<4 | char_to_int(temp_buf[8]);
	buf[5]=char_to_int(temp_buf[9])<<4 | char_to_int(temp_buf[10]);
	buf[4]=char_to_int(temp_buf[11])<<4 | char_to_int(temp_buf[12]);
	if('v'==temp_buf[0]&& 's'==temp_buf[1])//version //vs
	{
		memcpy(gsl_read,temp_buf,4);
		printk("gsl version\n");
	}
	else if('s'==temp_buf[0]&& 't'==temp_buf[1])//start //st
	{
	#ifdef GSL_TIMER	
		cancel_delayed_work_sync(&gsl_timer_check_work);
	#endif
		gsl_proc_flag = 1;
		gsl_reset_core(ts_data->client);
	}
	else if('e'==temp_buf[0]&&'n'==temp_buf[1])//end //en
	{
		msleep(20);
		gsl_reset_core(ts_data->client);
		gsl_start_core(ts_data->client);
		gsl_proc_flag = 0;
	}
	else if('r'==temp_buf[0]&&'e'==temp_buf[1])//read buf //
	{
		memcpy(gsl_read,temp_buf,4);
		memcpy(gsl_data_proc,buf,8);
	}
	else if('w'==temp_buf[0]&&'r'==temp_buf[1])//write buf
	{
		gsl_write_interface(ts_data->client,buf[4],buf,4);
	}
#ifdef GSL_ALG_ID
	else if('i'==temp_buf[0]&&'d'==temp_buf[1])//write id config //
	{
		tmp1=(buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4];
		tmp=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		if(tmp1>=0 && tmp1<gsl_cfg_table[gsl_cfg_index].data_size)
		{
			gsl_cfg_table[gsl_cfg_index].data_id[tmp1] = tmp;
		}
	}
#endif
exit_write_proc_out:
	kfree(path_buf);
	return count;
}

static int gsl_server_list_open(struct inode *inode,struct file *file)
{
	return single_open(file,gsl_config_read_proc,NULL);
}

static const struct file_operations gsl_seq_fops = {
	.open = gsl_server_list_open,
	.read = seq_read,
	.release = single_release,
	.write = gsl_config_write_proc,
	.owner = THIS_MODULE,
};
#endif

#ifdef GSL_TIMER
static void gsl_timer_check_func(struct work_struct *work)
{
	struct gsl_ts_data *ts = ts_data;	//获取封装设备结构体
	struct i2c_client *gsl_client = ts->client;
	static int i2c_lock_flag = 0;
	char read_buf[4]  = {0};
	char init_chip_flag = 0;
	int i,flag;
	//dev_info("----------------gsl_monitor_worker-----------------\n");	

	if(i2c_lock_flag != 0)
		return;
	else
		i2c_lock_flag = 1;

	/* check 0xb4 register,check interrupt if ok */
	gsl_read_interface(gsl_client, 0xb4, read_buf, 4);
	memcpy(int_2nd, int_1st, 4);
	memcpy(int_1st, read_buf, 4);

	if(int_1st[3] == int_2nd[3] && int_1st[2] == int_2nd[2] &&
		int_1st[1] == int_2nd[1] && int_1st[0] == int_2nd[0]) {
		/*
		printk("======int_1st: %x %x %x %x , int_2nd: %x %x %x %x ======\n",
			int_1st[3], int_1st[2], int_1st[1], int_1st[0], 
			int_2nd[3], int_2nd[2],int_2nd[1],int_2nd[0]);*/
		
		init_chip_flag = 1;
		goto queue_monitor_work;
	}
	
	/*check 0xb0 register,check firmware if ok*/
	for(i = 0; i < 5; i++) {
		gsl_read_interface(gsl_client, 0xb0, read_buf, 4);
		
		/*
		printk("[%s]:0xb0 before judgment = {0x%02x%02x%02x%02x} \n",
				__func__, read_buf[3], read_buf[2], 
					read_buf[1], read_buf[0]);*/
		
		if(read_buf[3] != 0x5a || read_buf[2] != 0x5a || 
			read_buf[1] != 0x5a || read_buf[0] != 0x5a) {
			/*
			printk("[%s]:0xb0 after judgment = {0x%02x%02x%02x%02x} \n",
				__func__, read_buf[3], read_buf[2], 
					read_buf[1], read_buf[0]);*/
			
			flag = 1;
		} else {
			flag = 0;
			break;
		}

	}
	if(flag == 1) {
		init_chip_flag = 1;
		goto queue_monitor_work;
	}
	
	/* check 0xbc register,check dac if normal */
	
	for(i = 0; i < 5; i++) {
		gsl_read_interface(gsl_client, 0xbc, read_buf, 4);
		/*
		printk("[%s]:0xbc before judgment = {0x%02x%02x%02x%02x} \n",
			__func__, read_buf[3], read_buf[2], 
				read_buf[1], read_buf[0]);*/

		if(read_buf[3] != 0 || read_buf[2] != 0 || 
			read_buf[1] != 0 || read_buf[0] != 0) {
			/*
			printk("[%s]:0xbc after judgment = {0x%02x%02x%02x%02x} \n",
				__func__, read_buf[3], read_buf[2], 
					read_buf[1], read_buf[0]);*/

			flag = 1;
		} else {
			flag = 0;
			break;
		}
	}
	if(flag == 1) {
		gsl_reset_core(gsl_client);
		gsl_start_core(gsl_client);
		init_chip_flag = 0;
	}
queue_monitor_work:
	if(init_chip_flag) {
		gsl_sw_init(gsl_client);
		memset(int_1st, 0xff, sizeof(int_1st));
	}
	
	if(!ts_data->suspended) {
		queue_delayed_work(gsl_timer_workqueue, &gsl_timer_check_work, 200);
	}
	i2c_lock_flag = 0;

}
#endif

static int gsl_compatible_id(struct i2c_client *client)
{
	u8 buf[4];
	int i,err;
	
	for(i = 0; i < 5; i++) {
		err = gsl_read_interface(client, 0xfc, buf, 4);
		dev_info(" 0xfc = {0x%02x%02x%02x%02x}\n", buf[3], buf[2],
					buf[1], buf[0]);
		if(!(err < 0)) {
			err = 1;
			break;	
		}
	}
	return err;	
}

#ifdef GSL_COMPATIBLE_GPIO
static int gsl_read_TotalAdr(struct i2c_client *client,u32 addr,u32 *data)
{
	u8 buf[4];
	int err;
	buf[3]=(u8)((addr/0x80)>>24);
	buf[2]=(u8)((addr/0x80)>>16);
	buf[1]=(u8)((addr/0x80)>>8);
	buf[0]=(u8)((addr/0x80));
	gsl_write_interface(client,0xf0,buf,4);
	err = gsl_read_interface(client,addr%0x80,buf,4);
	if(err > 0){
		*data = (buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
	}
	return err;
}
static int gsl_write_TotalAdr(struct i2c_client *client,u32 addr,u32 *data)
{
	int err;
	u8 buf[4];
	u32 value = *data;
	buf[3]=(u8)((addr/0x80)>>24);
	buf[2]=(u8)((addr/0x80)>>16);
	buf[1]=(u8)((addr/0x80)>>8);
	buf[0]=(u8)((addr/0x80));
	gsl_write_interface(client,0xf0,buf,4);
	buf[3]=(u8)((value)>>24);
	buf[2]=(u8)((value)>>16);
	buf[1]=(u8)((value)>>8);
	buf[0]=(u8)((value));
	err = gsl_write_interface(client,addr%0x80,buf,4);
	return err;
}

static int gsl_gpio_idt_tp(struct i2c_client *client)
{
	int i;
	u32 value = 0;
	u8 rstate;
	value = 0x1;
	gsl_write_TotalAdr(client,0xff000018,&value);
	value = 0x0;
	gsl_write_TotalAdr(client,0xff020000,&value);
	for(i=0;i<3;i++){
		gsl_read_TotalAdr(client,0xff020004,&value);
	}
	rstate = value & 0x1;
	
	if(rstate == 1){
		 gsl_cfg_index  = TPS465_TP; 
	}
	else if(rstate == 0){
		gsl_cfg_index = TPS465_TP;   //\B4\B4\CA\C0
	}
	else 
		gsl_cfg_index = TPS465_TP;

	dev_info("[tpd-gsl][%s] [rstate]=[%d]\n",__func__,rstate);
	dev_info("gsl_cfg_index=%d\n",gsl_cfg_index);
	printk("gsl_cfg_index=%d\n",gsl_cfg_index);
	return 1;
}
#endif

#ifdef GSL_TEST_TP
static ssize_t gsl_test_show(void)
{
	static int gsl_test_flag = 0; 
	char *tmp_buf;
	int err;
	int result = 0;
	printk("[%s]:enter gsl_test_show start::gsl_test_flag  = %d\n", __func__, 
			gsl_test_flag);
	if(gsl_test_flag == 1) {
		return 0;	
	}
	gsl_test_flag = 1;
	tmp_buf = kzalloc(3*1024,GFP_KERNEL);
	if(!tmp_buf){
		printk("[%s]:kzalloc kernel fail\n",__func__);
		return 0;
		}
	
	printk("[%s]:tp module test begin\n",__func__);
	
	err = gsl_tp_module_test(tmp_buf,3*1024);

	printk("[%s]:enter gsl_test_show end\n",__func__);
	
	if(err > 0){
		printk("[%s]:tp test pass\n",__func__);
		result = 1;

	}else{
		printk("[%s]:tp test failure\n",__func__);
		result = 0;
	}
	kfree(tmp_buf);
	gsl_test_flag = 0; 
	return result;
}

static ssize_t gsl_openshort_proc_write(struct file *filp, 
		const char __user *userbuf, size_t count, loff_t *ppos)
{
	return -1;
}

static ssize_t gsl_openshort_proc_read(struct file *file, 
		char __user *buf, size_t count, loff_t *ppos)
{
	//char *ptr = buf;
	int test_result  = 0;
	if(*ppos)
	{
		printk("[%s]:tp test again return\n",__func__);
		return 0;
	}
	*ppos += 16;
	test_result = gsl_test_show();
	memset(buf,'\0',16);
	count = 16;
	if(1 == test_result)
	{
		printk("[%s]:tp test pass\n",__func__);
		return sprintf(buf, "result=%d\n", 1);
	}
	else
	{
		printk("[%s]:tp test failure\n",__func__);
		return sprintf(buf, "result=%d\n", 0);
	}
	//return count;
}

static ssize_t gsl_rawdata_proc_write(struct file *filp, 
		const char __user *userbuf,size_t count, loff_t *ppos)
{
	return -1;
}

static ssize_t gsl_rawdata_proc_read(struct file *file, 
			char __user *buf,size_t count, loff_t *ppos)
{
//	int i,number=0;
	int i,ret;
	static short* gsl_ogv;
	ssize_t read_buf_chars = 0; 
	gsl_ogv = kzalloc(26*14*2,GFP_KERNEL);
	if(!gsl_ogv){
		return -1;
	}  

	//printk("[%s]:rawdata proc node read!\n",__func__);

#if 0
	if( number != 0 )
		return -1;
	else
		number++;
#endif	

#if 1
	if(*ppos)
	{
		printk("[%s]:tp test again return\n",__func__);
		return 0;
	}
#endif
	ret=gsl_test_show();
	gsl_obtain_array_data_ogv(gsl_ogv,26,14);

	for(i=0;i<26*14;i++)
	{
		read_buf_chars += sprintf(&(buf[read_buf_chars])," _%u_ ",gsl_ogv[i]);
		if(!((i+1)%10))
		{
			buf[read_buf_chars++] = '\n';
		}

	}

	buf[read_buf_chars-1] = '\n';

	//printk("[%s]:rawdata proc node end!\n",__func__);
	
	//*ppos += count;
	//return count;
	*ppos += read_buf_chars; 


	return read_buf_chars; 
}

static const struct file_operations gsl_rawdata_procs_fops =
{
	.write = gsl_rawdata_proc_write,
	.read = gsl_rawdata_proc_read,
	.open = simple_open,
	.owner = THIS_MODULE,
};

static const struct file_operations gsl_openshort_procs_fops =
{
    .write = gsl_openshort_proc_write,
    .read = gsl_openshort_proc_read,
    .open = simple_open,
    .owner = THIS_MODULE,
};

void create_ctp_proc(void)
{
	struct proc_dir_entry *gsl_device_proc = NULL;
	struct proc_dir_entry *gsl_openshort_proc = NULL;
	struct proc_dir_entry *gsl_rawdata_proc = NULL;

	gsl_device_proc = proc_mkdir(GSL_PARENT_PROC_NAME, NULL);
		if(gsl_device_proc == NULL)
    	{
        	printk("[%s]: create parent_proc fail\n",__func__);
        	return;
    	}

	gsl_openshort_proc = proc_create(GSL_OPENHSORT_PROC_NAME, 0666, 
			gsl_device_proc, &gsl_openshort_procs_fops);

    	if (gsl_openshort_proc == NULL)
    	{
        	printk("[%s]: create openshort_proc fail\n",__func__);
    	}
		
	gsl_rawdata_proc = proc_create(GSL_RAWDATA_PROC_NAME, 0777, 
				gsl_device_proc, &gsl_rawdata_procs_fops);
    	if (gsl_rawdata_proc == NULL)
    	{
        	printk("[%s]: create ctp_rawdata_proc fail\n",__func__);
    	}
}
#endif

//电源初始化
static int gsl_ts_power_init(struct gsl_ts_data *ts_data, bool init)
{
	int rc;

	if (init) {
		ts_data->vdd = regulator_get(&ts_data->client->dev, "vdd");//dts文件中添加该关键字
		if (IS_ERR(ts_data->vdd)) {
			rc = PTR_ERR(ts_data->vdd);
			dev_err(&ts_data->client->dev,
				"Regulator get failed vdd rc=%d\n", rc);
			return rc;
		}

		if (regulator_count_voltages(ts_data->vdd) > 0) {
			rc = regulator_set_voltage(ts_data->vdd,
							GSL_VTG_MIN_UV,
							GSL_VTG_MAX_UV);//设置gsl1680电压2.6V~3.3V
			if (rc) {
				dev_err(&ts_data->client->dev,
					"Regulator set_vtg failed vdd rc=%d\n",
					rc);
				goto reg_vdd_put;
			}
		}

		ts_data->vcc_i2c = regulator_get(&ts_data->client->dev, "vcc-i2c");//dts文件中添加该关键字
		if (IS_ERR(ts_data->vcc_i2c)) {
			rc = PTR_ERR(ts_data->vcc_i2c);
			dev_err(&ts_data->client->dev,
				"Regulator get failed vcc-i2c rc=%d\n", rc);
			goto reg_vdd_set_vtg;
		}

		if (regulator_count_voltages(ts_data->vcc_i2c) > 0) {
			rc = regulator_set_voltage(ts_data->vcc_i2c,
						GSL_I2C_VTG_MIN_UV,
						GSL_I2C_VTG_MAX_UV);//设置i2c电压1.8V
			if (rc) {
				dev_err(&ts_data->client->dev,
				"Regulator set_vtg failed vcc-i2c rc=%d\n", rc);
				goto reg_vcc_i2c_put;

			}
		}
	} else {
		if (regulator_count_voltages(ts_data->vdd) > 0)
			regulator_set_voltage(ts_data->vdd, 0,
							GSL_VTG_MAX_UV);//设置vdd电压0-3.3V

		regulator_put(ts_data->vdd);

		if (regulator_count_voltages(ts_data->vcc_i2c) > 0)
			regulator_set_voltage(ts_data->vcc_i2c, 0,
						GSL_I2C_VTG_MAX_UV);//设置i2c电压0-1.8V

		regulator_put(ts_data->vcc_i2c);
	}

	return 0;

reg_vcc_i2c_put:
	regulator_put(ts_data->vcc_i2c);
reg_vdd_set_vtg:
	if (regulator_count_voltages(ts_data->vdd) > 0)
		regulator_set_voltage(ts_data->vdd, 0, GSL_VTG_MAX_UV);
reg_vdd_put:
	regulator_put(ts_data->vdd);
	return rc;
}

//电源开启
static int gsl_ts_power_on(struct gsl_ts_data *ts_data, bool on)
{
	int rc;

	if (!on)
		goto power_off;

	rc = regulator_enable(ts_data->vdd);	//使能vdd这个regulator
	if (rc) {
		dev_err(&ts_data->client->dev,
			"Regulator vdd enable failed rc=%d\n", rc);
		return rc;
	}

	rc = regulator_enable(ts_data->vcc_i2c);	//使能vdd-i2c这个regulator
	if (rc) {
		dev_err(&ts_data->client->dev,
			"Regulator vcc_i2c enable failed rc=%d\n", rc);
		regulator_disable(ts_data->vdd);
	}

	return rc;

power_off:
	rc = regulator_disable(ts_data->vdd);
	if (rc) {
		dev_err(&ts_data->client->dev,
			"Regulator vdd disable failed rc=%d\n", rc);
		return rc;
	}

	rc = regulator_disable(ts_data->vcc_i2c);
	if (rc) {
		dev_err(&ts_data->client->dev,
			"Regulator vcc_i2c disable failed rc=%d\n", rc);
		rc = regulator_enable(ts_data->vdd);
	}

	return rc;
}

//固件初始化
static void gsl_sw_init(struct i2c_client *client)
{
	if(1 == ts_data->updating_fw)
		return;
	ts_data->updating_fw = 1;

	//复位芯片
	gpio_set_value(ts_data->pdata->reset_gpio, 0);
	msleep(20);
	gpio_set_value(ts_data->pdata->reset_gpio, 1);
	msleep(20);

	gsl_clear_reg(client);
	gsl_reset_core(client);

	gsl_load_fw(client,gsl_cfg_table[gsl_cfg_index].fw,
		gsl_cfg_table[gsl_cfg_index].fw_size);

	gsl_start_core(client);

	ts_data->updating_fw = 0;
}

static void check_mem_data(struct i2c_client *client)
{

	u8 read_buf[4]  = {0};
	msleep(30);
	gsl_read_interface(client, 0xb0, read_buf, 4);
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a 
		|| read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		printk("0xb4 ={0x%02x%02x%02x%02x}\n",
			read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		gsl_sw_init(client);
	}
}

#define GSL_CHIP_NAME	"gslx68x"
static ssize_t gsl_sysfs_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	//ssize_t len=0;
	int count = 0;
	u8 buf_tmp[4];
	u32 tmp;
	//char *ptr = buf;
	count += scnprintf(buf, PAGE_SIZE, "sileadinc:");
	count += scnprintf(buf + count, PAGE_SIZE - count, GSL_CHIP_NAME);

#ifdef GSL_TIMER
	count += scnprintf(buf + count, PAGE_SIZE - count, ":0001-1:");
#else
	count += scnprintf(buf + count, PAGE_SIZE - count, ":0001-0:");
#endif

#ifdef TPD_PROC_DEBUG
	count += scnprintf(buf + count, PAGE_SIZE - count, "0002-1:");
#else
	count += scnprintf(buf + count, PAGE_SIZE - count, "0002-0:");
#endif

#ifdef GSL_PROXIMITY_SENSOR
	count += scnprintf(buf + count, PAGE_SIZE - count, "0003-1:");
#else
	count += scnprintf(buf + count, PAGE_SIZE - count, "0003-0:");
#endif

#ifdef GSL_DEBUG
	count += scnprintf(buf + count, PAGE_SIZE - count, "0004-1:");
#else
	count += scnprintf(buf + count, PAGE_SIZE - count, "0004-0:");
#endif

#ifdef GSL_ALG_ID
	tmp = gsl_version_id();
	count += scnprintf(buf + count, PAGE_SIZE - count, "%08x:", tmp);
	count += scnprintf(buf + count, PAGE_SIZE - count, "%08x:",
		gsl_cfg_table[gsl_cfg_index].data_id[0]);
#endif

	buf_tmp[0] = 0x3; 
	buf_tmp[1] = 0; 
	buf_tmp[2] = 0;
	buf_tmp[3] = 0;
	gsl_write_interface(ts_data->client, 0xf0, buf_tmp, 4);
	gsl_read_interface(ts_data->client, 0, buf_tmp, 4);
	count += scnprintf(buf + count, PAGE_SIZE - count, 
				"%02x%02x%02x%02x\n",
		buf_tmp[3], buf_tmp[2], buf_tmp[1], buf_tmp[0]);

    return count;
}

static DEVICE_ATTR(version, 0444, gsl_sysfs_version_show, NULL);

#ifdef GSL_GESTURE
static void gsl_enter_doze(struct gsl_ts_data *ts)
{
	u8 buf[4] = {0};
#if 0
	u32 tmp;
	gsl_reset_core(ts->client);
	temp = ARRAY_SIZE(GSLX68X_FW_GESTURE);
	gsl_load_fw(ts->client,GSLX68X_FW_GESTURE,temp);
	gsl_start_core(ts->client);
	msleep(1000);		
#endif

	buf[0] = 0xa;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;
	gsl_write_interface(ts->client,0xf0,buf,4);
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0x1;
	buf[3] = 0x5a;
	gsl_write_interface(ts->client,0x8,buf,4);
	//gsl_gesture_status = GE_NOWORK;
	msleep(10);
	gsl_gesture_status = GE_ENABLE;

}
static void gsl_quit_doze(struct gsl_ts_data *ts)
{
	u8 buf[4] = {0};
	//u32 tmp;

	gsl_gesture_status = GE_DISABLE;
		
	gpio_set_value(GSL_RST_GPIO_NUM,0);
	msleep(20);
	gpio_set_value(GSL_RST_GPIO_NUM,1);
	msleep(5);
	
	buf[0] = 0xa;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;
	gsl_write_interface(ts->client,0xf0,buf,4);
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0x5a;
	gsl_write_interface(ts->client,0x8,buf,4);
	msleep(10);

#if 0
	gsl_reset_core(ts_data->client);
	temp = ARRAY_SIZE(GSLX68X_FW_CONFIG);
	//gsl_load_fw();
	gsl_load_fw(ts_data->client,GSLX68X_FW_CONFIG,temp);
	gsl_start_core(ts_data->client);
#endif
}

static ssize_t gsl_sysfs_tpgesture_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 count = 0;
	count += scnprintf(buf,PAGE_SIZE,"tp gesture is on/off:\n");
	if(gsl_gesture_flag == 1){
		count += scnprintf(buf+count,PAGE_SIZE-count,
				" on \n");
	}else if(gsl_gesture_flag == 0){
		count += scnprintf(buf+count,PAGE_SIZE-count,
				" off \n");
	}
	count += scnprintf(buf+count,PAGE_SIZE-count,"tp gesture:");
	count += scnprintf(buf+count,PAGE_SIZE-count,
			"%c\n",gsl_gesture_c);
    	return count;
}
static ssize_t gsl_sysfs_tpgesturet_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
#if 1
	if(buf[0] == '0') {
		gsl_gesture_flag = 0;  
	} else if(buf[0] == '1') {
		gsl_gesture_flag = 1;
	}
#endif
	return count;
}
static DEVICE_ATTR(gesture, 0666, gsl_sysfs_tpgesture_show, 
					gsl_sysfs_tpgesturet_store);
#endif

static struct attribute *gsl_attrs[] = {

	&dev_attr_version.attr,
		
#ifdef GSL_GESTURE
	&dev_attr_gesture.attr,
#endif

#ifdef GSL_PROXIMITY_SENSOR
	&dev_attr_proximity_sensor_enable.attr,
	&dev_attr_proximity_sensor_status.attr,
#endif

	NULL
};

static const struct attribute_group gsl_attr_group = {
	.attrs = gsl_attrs,
};

#if GSL_HAVE_TOUCH_KEY
static int gsl_report_key(struct input_dev *idev, int x, int y)
{
	int i;

	for(i=0; i < GSL_KEY_NUM; i++) {
		if(gsl_cfg_index == V10X_TP) {   //v100 
			if(x > gsl_key_data[i].x_min &&
				x < gsl_key_data[i].x_max &&
				y > gsl_key_data[i].y_min &&
				y < gsl_key_data[i].y_max) {
				
				ts_data->gsl_key_state = i+1;				
				input_report_key(idev, gsl_key_data[i].key, 1);
				input_sync(idev);
				return 1;
			}
		} else if(gsl_cfg_index==V20X_TP) { //v200
			if(x > gsl_key_data_v200[i].x_min &&
				x < gsl_key_data_v200[i].x_max &&
				y > gsl_key_data_v200[i].y_min &&
				y < gsl_key_data_v200[i].y_max) {
				
				ts_data->gsl_key_state = i+1;				
				input_report_key(idev, gsl_key_data_v200[i].key, 1);
				input_sync(idev);
				return 1;
			}
		} else
			dev_info("TPS465_TP");
	}
	return 0;
}
#endif

static void gsl_report_point(struct input_dev *idev, 
			struct gsl_touch_info *cinfo)
{
	int i; 
	u32 gsl_point_state = 0;
	u32 temp = 0;

	//dev_info("==========report point start==========\n");

	if(cinfo->finger_num > 0 && cinfo->finger_num < 6) {
		//dev_info("finger_num = %d\n", cinfo->finger_num);
		ts_data->gsl_up_flag = 0;
		gsl_point_state = 0;
	
#if GSL_HAVE_TOUCH_KEY
		//dev_info("define GSL_HAVE_TOUCH_KEY\n");
		if(1 == cinfo->finger_num) {
			if(cinfo->x[0] > GSL_MAX_X || cinfo->y[0] > GSL_MAX_Y) {
				gsl_report_key(idev, cinfo->x[0], cinfo->y[0]);
				return;		
			}
		}
#endif

#ifdef GSL_IRQ_WAKE
		//dev_info("define GSL_IRQ_WAKE\n");
		if(gsl_irq_wake_justnow > 0 && gsl_irq_wake_en == 1) 
			gsl_irq_wake_justnow--;
		else {
			for(i=0; i < cinfo->finger_num; i++) {
				gsl_point_state |= (0x1 << cinfo->id[i]);	

			#ifdef GSL_REPORT_POINT_SLOT
				//dev_info("define GSL_REPORT_POINT_SLOT\n");
				input_mt_slot(idev, cinfo->id[i] - 1);
				input_report_abs(idev, ABS_MT_TRACKING_ID, cinfo->id[i]-1);
				input_report_abs(idev, ABS_MT_TOUCH_MAJOR, GSL_PRESSURE);
				input_report_abs(idev, ABS_MT_POSITION_X, cinfo->x[i]);
				input_report_abs(idev, ABS_MT_POSITION_Y, cinfo->y[i]);	
				input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 1);		
			#else				
				input_report_key(idev, BTN_TOUCH, 1);
				input_report_abs(idev, ABS_MT_TRACKING_ID, cinfo->id[i]-1);
				input_report_abs(idev, ABS_MT_TOUCH_MAJOR, GSL_PRESSURE);
				input_report_abs(idev, ABS_MT_POSITION_X, cinfo->x[i]);
				input_report_abs(idev, ABS_MT_POSITION_Y, cinfo->y[i]);	
				input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(idev);		
			#endif
			}
		}
#endif //GSL_IRQ_WAKE

	} else if(cinfo->finger_num == 0) {
		gsl_point_state = 0;
	//	ts_data->gsl_point_state = 0;
		if(1 == ts_data->gsl_up_flag) {
			return;
		}
		ts_data->gsl_up_flag = 1;
		
		#if GSL_HAVE_TOUCH_KEY
		if(ts_data->gsl_key_state > 0) {
			if(ts_data->gsl_key_state < GSL_KEY_NUM + 1) {
				if(gsl_cfg_index == V10X_TP)					
					input_report_key(idev, 
						gsl_key_data[ts_data->gsl_key_state - 1].key, 0);
				else if(gsl_cfg_index == V20X_TP)
					input_report_key(idev, 
						gsl_key_data_v200[ts_data->gsl_key_state - 1].key, 0);					
				else
					printk("==========TPS465_TP==========\n");
				input_sync(idev);				
			}
		}
		#endif

		#ifndef GSL_REPORT_POINT_SLOT
		input_report_key(idev, BTN_TOUCH, 0);
		input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
		input_mt_sync(idev);
		#endif
	}

	temp = gsl_point_state & ts_data->gsl_point_state;
	temp = (~temp) & ts_data->gsl_point_state;

#ifdef GSL_REPORT_POINT_SLOT
	for(i = 1; i < 6; i++) {
		if(temp & (0x1 << i)) {
			input_mt_slot(idev, i-1);
			input_report_abs(idev, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(idev, MT_TOOL_FINGER, false);
		}
	}
#endif	
	
	ts_data->gsl_point_state = gsl_point_state;
	input_sync(idev);
	
}

//中断函数里面倒计时一段时间后启动该工作
static void gsl_report_work(struct work_struct *work)
{
	int rc,tmp;
	u8 buf[44] = {0};
	struct gsl_touch_info *cinfo = ts_data->cinfo;
	struct i2c_client *client = ts_data->client;	//i2c客户端设备
	struct input_dev *idev = ts_data->input_dev;	//input输入子设备
	int tmp1=0;
	
#ifdef GSL_GESTURE
	unsigned int test_count = 0;
#endif

#ifdef GSL_IRQ_WAKE
	if(gsl_irq_wake_en == 1) {
		if (flag_tp_down) {
			flag_tp_down = 0;

			//产生一个电源按键的点击事件，唤醒系统			
			input_report_key(idev, KEY_POWER, 1);
			input_sync(idev);
			input_report_key(idev, KEY_POWER, 0);
			input_sync(idev);			
			gsl_irq_wake_justnow = 6;

			goto schedule;
		}
	}
#endif

	if(1 == ts_data->updating_fw)
		goto schedule;
	
#ifdef TPD_PROC_DEBUG
		if(gsl_proc_flag == 1){
			return;
		}
#endif
	
	/* Gesture Resume */
#ifdef GSL_GESTURE
	printk("GSL:::0x80=%02x%02x%02x%02x[%d]\n", buf[3], buf[2], 
							buf[1], buf[0], test_count++);
	printk("GSL:::0x84=%02x%02x%02x%02x\n",buf[7], buf[6], 
							buf[5], buf[4]);
	printk("GSL:::0x88=%02x%02x%02x%02x\n",buf[11], buf[10], 
							buf[9], buf[8]);
#endif
	
	/* read data from DATA_REG */
	rc = gsl_read_interface(client, 0x80, buf, 44);	//从地址0x80开始读取44个字节
	if (rc < 0) {
		dev_err(&client->dev, "[gsl] I2C read failed\n");
		goto schedule;
	}

	if (buf[0] == 0xff) {
		goto schedule;
	}

	cinfo->finger_num = buf[0];	//将触摸的手指个数保存
	for(tmp = 0; tmp < (cinfo->finger_num > 10 ? 10 : cinfo->finger_num); tmp++) {
		cinfo->y[tmp] = (buf[tmp * 4 + 4] | ((buf[tmp * 4 + 5]) << 8));
		cinfo->x[tmp] = (buf[tmp * 4 + 6] | ((buf[tmp * 4 + 7] & 0x0f) << 8));
		cinfo->id[tmp] = buf[tmp * 4 + 7] >> 4;
		//dev_info("y[%d] = %d,x[%d] = %d,id[%d] = %d\n", tmp, cinfo->y[tmp], 
		//			tmp, cinfo->x[tmp], tmp, cinfo->id[tmp]);
	}
	
#ifdef GSL_ALG_ID
	cinfo->finger_num = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | (buf[0]);
	gsl_alg_id_main(cinfo);
	tmp1 = gsl_mask_tiaoping();
	
	if(tmp1 > 0 && tmp1 < 0xffffffff) {
		buf[0] = 0xa;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;
		gsl_write_interface(client, 0xf0, buf,4);
		buf[0] = (u8)(tmp1 & 0xff);
		buf[1] = (u8)((tmp1 >> 8) & 0xff);
		buf[2] = (u8)((tmp1 >> 16) & 0xff);
		buf[3] = (u8)((tmp1 >> 24) & 0xff);
		gsl_write_interface(client, 0x8, buf, 4);
	}
#endif

#ifdef GSL_GESTURE
		printk("gsl_gesture_status = %d,gsl_gesture_flag = %d[%d]\n",
			gsl_gesture_status, gsl_gesture_flag, test_count++);
	
		if(GE_ENABLE == gsl_gesture_status && gsl_gesture_flag == 1) {
			int tmp_c;
			u8 key_data = 0;
			tmp_c = gsl_obtain_gesture();
			printk("gsl_obtain_gesture():tmp_c = 0x%x[%d]\n", tmp_c, test_count++);
			dev_info("gsl_obtain_gesture():tmp_c = 0x%x\n", tmp_c);
			switch(tmp_c) {
			case (int)'C':
				key_data = KEY_C;
				break;
			case (int)'E':
				key_data = KEY_E;
				break;
			case (int)'W':
				key_data = KEY_W;
				break;
			case (int)'O':
				key_data = KEY_O;
				break;
			case (int)'M':
				key_data = KEY_M;
				break;
			case (int)'Z':
				key_data = KEY_Z;
				break;
			case (int)'V':
				key_data = KEY_V;
				break;
			case (int)'S':
				key_data = KEY_S;
				break;
			case (int)'*':	
				key_data = KEY_POWER;
				break;/* double click */
				case (int)0xa1fa:
				key_data = KEY_F1;
				break;/* right */
			case (int)0xa1fd:
				key_data = KEY_F2;
				break;/* down */
			case (int)0xa1fc:	
				key_data = KEY_F3;
				break;/* up */
			case (int)0xa1fb:	/* left */
				key_data = KEY_F4;
				break;	
			
			}
	
			if(key_data != 0) {
				gsl_gesture_c = (char)(tmp_c & 0xff);
				gsl_gesture_status = GE_WAKEUP;
				printk("gsl_obtain_gesture():tmp_c=%c\n", gsl_gesture_c);
				//input_report_key(tpd->dev,key_data,1);				
				input_report_key(idev, KEY_POWER, 1);
				input_sync(idev);
				//input_report_key(tpd->dev,key_data,0);
				input_report_key(idev, KEY_POWER, 0);
				input_sync(idev);
			}
			return;
		}
#endif
	
	gsl_report_point(idev, cinfo);

schedule:
	enable_irq(client->irq);

}

static int gsl_request_input_dev(struct gsl_ts_data *ts_data)

{
	struct input_dev *input_dev = ts_data->input_dev;
	int err;
	
#if GSL_HAVE_TOUCH_KEY
	int i;
#endif

	/* set input parameter */	
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	
#ifdef GSL_REPORT_POINT_SLOT
	__set_bit(EV_REP, input_dev->evbit);
//subin	input_mt_init_slots(input_dev,5);
	input_mt_init_slots(input_dev, 5, 0);
#else 
	__set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 5, 0, 0);
#endif

	__set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	__set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	
#ifdef TOUCH_VIRTUAL_KEYS
	__set_bit(KEY_MENU,  input_dev->keybit);
	__set_bit(KEY_BACK,  input_dev->keybit);
	__set_bit(KEY_HOME,  input_dev->keybit);    
#endif

#if GSL_HAVE_TOUCH_KEY
	for(i=0; i < GSL_KEY_NUM; i++) {
		if(gsl_cfg_index == V10X_TP)
			__set_bit(gsl_key_data[i].key, input_dev->keybit);
	    else if(gsl_cfg_index == V20X_TP)
			__set_bit(gsl_key_data_v200[i].key, input_dev->keybit);
	}
#endif
	
#ifdef GSL_IRQ_WAKE	//支持触摸唤醒系统
	if(gsl_irq_wake_en == 1)
		__set_bit(KEY_POWER, input_dev->keybit);
#endif // GSL_IRQ_WAKE

	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, GSL_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, GSL_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	input_dev->name = GSL_TS_NAME;		//dev_name(&client->dev) lpl

	/* register input device */
	err = input_register_device(input_dev);	//在系统中注册input device子系统
	if (err) {
		goto err_register_input_device_fail;
	}
	return 0;
err_register_input_device_fail:
	input_free_device(input_dev);
	return err;
}

static int gsl_read_version(void)
{
	u8 buf[4] = {0};
	int err = 0;
	buf[0] = 0x03;
	
	err = gsl_write_interface(ts_data->client, 0xf0, buf, 4);
	if(err < 0) {
		return err;
	}
	gsl_ts_read(ts_data->client, 0x04, buf, 4);
	if(err < 0) {
		return err;
	}
	ts_data->fw_version = buf[2];
	dev_info("fw version = %d\n", ts_data->fw_version);
	
	return 0;
}

//中断服务函数
static irqreturn_t gsl_ts_interrupt(int irq, void *dev_id)
{
	struct i2c_client *client = ts_data->client;
	
	//dev_info("==========gslX68X_ts_interrupt==========\n");
	
	disable_irq_nosync(client->irq);	//失效irq

#ifdef GSL_GESTURE
	if(gsl_gesture_status == GE_ENABLE && gsl_gesture_flag == 1) {
		wake_lock_timeout(&gsl_wake_lock, msecs_to_jiffies(2000));
	}
#endif
	
	if (!work_pending(&ts_data->work)) {	//判断是否有工作任务挂起
		queue_work(ts_data->wq, &ts_data->work);	//将work任务添加到wq工作队列中
	}
	
	return IRQ_HANDLED;
}

#if defined(CONFIG_FB)
static void gsl_ts_suspend(void)
{
	struct i2c_client *client = ts_data->client;
	u32 tmp;
	
	/* version information */
	printk("[tp-gsl]the last time of debug:%x\n",TPD_DEBUG_TIME);

	if(1 == ts_data->updating_fw)
		return;
	ts_data->suspended = 1;
	
#ifdef GSL_ALG_ID
	tmp = gsl_version_id();	
	printk("[tp-gsl]the version of alg_id:%x\n",tmp);
#endif

#ifdef GSL_TIMER	
	cancel_delayed_work_sync(&gsl_timer_check_work);
#endif

/*Guesture Resume*/
#ifdef GSL_GESTURE
		if(gsl_gesture_flag == 1){
			gsl_enter_doze(ts_data);
			return;
		}
#endif

#ifndef GSL_IRQ_WAKE
	#ifndef GSL_GESTURE
	disable_irq_nosync(client->irq);
	gpio_set_value(ts_data->pdata->reset_gpio, 0);
	#endif
#endif

#ifdef GSL_IRQ_WAKE
	if(gsl_irq_wake_en == 1)
		flag_tp_down = 1;
	else
	{
		disable_irq_nosync(client->irq);
		gpio_set_value(ts_data->pdata->reset_gpio, 0);
	}
#endif // GSL_IRQ_WAKE


	return;
}

static void gsl_ts_resume(void)
{	
	struct i2c_client *client = ts_data->client;
	
	dev_info("==========gslX68X_ts_resume==========\n");
	if(1 == ts_data->updating_fw) {
		ts_data->suspended = 0;
		return;
	}

	/* Gesture Resume */
#ifdef GSL_GESTURE
		if(gsl_gesture_flag == 1) {
			gsl_quit_doze(ts_data);
		}
#endif

	gpio_set_value(ts_data->pdata->reset_gpio, 1);
	msleep(20);
	gsl_reset_core(client);

	/*Gesture Resume*/
#ifdef GSL_GESTURE	
#ifdef GSL_ALG_ID
	gsl_DataInit(gsl_cfg_table[gsl_cfg_index].data_id);
#endif
#endif

#ifdef GSL_IRQ_WAKE
	if(gsl_irq_wake_en == 1) {
		//disable_irq_wake(client->irq);
		flag_tp_down = 0;
	}
#endif
	
	gsl_start_core(client);
	msleep(20);
	check_mem_data(client);
	//enable_irq(client->irq);
		
#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1) {
		return;
	}
#endif
	
#ifdef GSL_TIMER
	queue_delayed_work(gsl_timer_workqueue, 
		&gsl_timer_check_work, GSL_TIMER_CHECK_CIRCLE);
#endif

	ts_data->suspended = 0;
	return;

}

static void gsl_ts_resume_work(struct work_struct *work)
{
	gsl_ts_resume();
}

static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	
	if (evdata && evdata->data && event == FB_EVENT_BLANK ){
		blank = evdata->data;
		
		if (*blank == FB_BLANK_UNBLANK) {			
			if (!work_pending(&ts_data->resume_work)){
				schedule_work(&ts_data->resume_work);				
			}
		}else if (*blank == FB_BLANK_POWERDOWN) {
			cancel_work_sync(&ts_data->resume_work);
			gsl_ts_suspend();
		}
	}

	return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void gsl_early_suspend(struct early_suspend *handler)
{
	u32 tmp;
	struct i2c_client *client = ts_data->client;
	dev_info("==gslX68X_ts_suspend=\n");
	//version info
	printk("[tp-gsl]the last time of debug:%x\n",TPD_DEBUG_TIME);

	if(1==ts_data->updating_fw)
		return;

	ts_data->suspended = 1;

#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		return;
	}
#endif

#ifdef GSL_ALG_ID
	tmp = gsl_version_id();	
	printk("[tp-gsl]the version of alg_id:%x\n",tmp);
#endif
#ifdef GSL_TIMER	
	cancel_delayed_work_sync(&gsl_timer_check_work);
#endif	

	disable_irq_nosync(client->irq);
	gpio_set_value(GSL_RST_GPIO_NUM, 0);
}

static void gsl_early_resume(struct early_suspend *handler)
{	
	struct i2c_client *client = ts_data->client;
	dev_info("==gslX68X_ts_resume=\n");
	if(1==ts_data->updating_fw){
		ts_data->suspended = 0;
		return;
	}

	gpio_set_value(GSL_RST_GPIO_NUM, 1);
	msleep(20);
	gsl_reset_core(client);
	gsl_start_core(client);
	msleep(20);
	check_mem_data(client);
	enable_irq(client->irq);
#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		return;
	}
#endif
	
#ifdef GSL_TIMER
	queue_delayed_work(gsl_timer_workqueue, 
		&gsl_timer_check_work, GSL_TIMER_CHECK_CIRCLE);
#endif
	
	ts_data->suspended = 0;

}
#endif

#if defined(CONFIG_FB)
static int gsl_get_dt_coords(struct device *dev, char *name,
				struct gsl_ts_platform_data *pdata)
{
	u32 coords[GSL_COORDS_ARR_SIZE];
	struct property *prop;
	struct device_node *np = dev->of_node;
	int coords_size, rc;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	coords_size = prop->length / sizeof(u32);
	if (coords_size != GSL_COORDS_ARR_SIZE) {
		dev_err(dev, "invalid %s\n", name);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(np, name, coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read %s\n", name);
		return rc;
	}

	if (!strcmp(name, "gsl,panel-coords")) {//gsl,panel-coords
		pdata->panel_minx = coords[0];
		pdata->panel_miny = coords[1];
		pdata->panel_maxx = coords[2];
		pdata->panel_maxy = coords[3];
	} else if (!strcmp(name, "gsl,display-coords")) {//gsl,display-coords
		pdata->x_min = coords[0];
		pdata->y_min = coords[1];
		pdata->x_max = coords[2];
		pdata->y_max = coords[3];
	} else {
		dev_err(dev, "unsupported property %s\n", name);
		return -EINVAL;
	}

	return 0;
}

static int gsl_parse_dt(struct device *dev, 
				struct gsl_ts_platform_data *pdata)
{
	int i = 0;
	int rc;
	struct device_node *np = dev->of_node;
	struct property *prop;
	u32 temp_val;

	dev_info("==========parse dt start==========\n");
	pdata->name = "gsl";
	rc = of_property_read_string(np, "gsl,name", &pdata->name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read name\n");
		return rc;
	}
	printk("name = %s\n", pdata->name);

	rc = gsl_get_dt_coords(dev, "gsl,panel-coords", pdata);
	if (rc && (rc != -EINVAL))
		return rc;
	printk("panel: minx = %d,miny = %d,maxx = %d,maxy = %d\n",
			pdata->panel_minx, pdata->panel_miny, 
			pdata->panel_maxx, pdata->panel_maxy);

	rc = gsl_get_dt_coords(dev, "gsl,display-coords", pdata);
	if (rc)
		return rc;
	printk("display: minx = %d,miny = %d,maxx = %d,maxy = %d\n", 
			pdata->x_min, pdata->y_min,
			pdata->x_max, pdata->y_max);

	rc = of_property_read_u32(np, "gsl,hard-reset-delay-ms",
								&temp_val);
	if (!rc)
		pdata->hard_reset_delay_ms = temp_val;
	else
		return rc;
	printk("hard-reset-delay-ms = %d\n", 
				pdata->hard_reset_delay_ms);

	rc = of_property_read_u32(np, "gsl,post-hard-reset-delay-ms",
								&temp_val);
	if (!rc)
		pdata->post_hard_reset_delay_ms = temp_val;
	else
		return rc;
	printk("post-hard-reset-delay-ms = %d\n", 
				pdata->post_hard_reset_delay_ms);

	/* reset, irq gpio info */
	pdata->reset_gpio = of_get_named_gpio_flags(np, "gsl,reset-gpio",
				0, &pdata->reset_gpio_flags);
	if (pdata->reset_gpio < 0)
		return pdata->reset_gpio;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "gsl,irq-gpio",
				0, &pdata->irq_gpio_flags);
	if (pdata->irq_gpio < 0)
		return pdata->irq_gpio;
	printk("reset-irq-gpio-info: irq-gpio = %d,irq-flag = %d,"
				"reset-gpio = %d,reset-flag = %d\n",
				pdata->irq_gpio, pdata->irq_gpio_flags,
				pdata->reset_gpio, pdata->reset_gpio_flags);

	rc = of_property_read_u32(np, "gsl,num-max-touches", &temp_val);
	if (!rc)
		pdata->num_max_touches = temp_val;
	else
		return rc;
	printk("num-max-touches = %d\n", pdata->num_max_touches);

	prop = of_find_property(np, "gsl,button-map", NULL);
	if (prop) {
		pdata->num_buttons = prop->length / sizeof(temp_val);
		if (pdata->num_buttons > MAX_BUTTONS)
			return -EINVAL;

		rc = of_property_read_u32_array(np,
				"gsl,button-map", pdata->button_map,
			pdata->num_buttons);
		if (rc) {
			dev_err(dev, "Unable to read key codes\n");
			return rc;
		}
	}
	printk("button-map: ");
	for (i = 0; i < MAX_BUTTONS; i++)
		printk("%d ", pdata->button_map[i]);
	printk("\n");
	
	//添加配置索引号参数
	rc = of_property_read_u32(np, "gsl,cfg-index", &temp_val);
	if (!rc) {
		gsl_cfg_index = temp_val;
		
		//根据配置设置分辨率 add by jiangxh 20181129
		if(gsl_cfg_index == TPS465_TP || gsl_cfg_index == TPS508_TP) {
			GSL_MAX_X = 720;
			GSL_MAX_Y = 1280;
		}		
		else if(gsl_cfg_index == TPX920_TP) {
			GSL_MAX_X = 1024;
			GSL_MAX_Y = 600;
		}
		else {
			GSL_MAX_X = 800;
			GSL_MAX_Y = 1280;
		}			
	}
	else
		return rc;
	printk("gsl_cfg_index = %d,res-max-x = %d,res-max-y = %d\n",
				gsl_cfg_index, GSL_MAX_X, GSL_MAX_Y);
	

#ifdef GSL_IRQ_WAKE
	rc = of_property_read_u32(np, "gsl,irq-wake", &temp_val);
	if(!rc) {
		gsl_irq_wake_en = temp_val;
	}
#endif
	printk("irq-wake = %d\n", gsl_irq_wake_en);
	dev_info("==========parse dt over==========\n");

	return 0;
}
#else
static int gsl_parse_dt(struct device *dev, 
				struct gsl_ts_platform_data *pdata)
{
	return -ENODEV;
}
#endif

//驱动探测函数
static int gsl_ts_probe(struct i2c_client *client, 
			const struct i2c_device_id *id)
{    
	int err = 0;
	struct input_dev *input_dev;
    struct gsl_ts_platform_data *pdata;

	dev_info("======================gsl_ts_probe start======================\n");
	
	printk("client-name = %s,dev-name = %s\n", client->name,
					dev_name(&client->dev));
	
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct gsl_ts_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		err = gsl_parse_dt(&client->dev, pdata);
		if (err) {
			dev_err(&client->dev, "Failed to parse dt\n");
			err = -ENOMEM;
			goto exit_parse_dt_err;
		}
	}
	else
		pdata = client->dev.platform_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
        dev_err(&client->dev, "I2c doesn't work\n");
		goto exit_check_functionality_failed;
	}

	ts_data = kzalloc(sizeof(struct gsl_ts_data), GFP_KERNEL);
	if (ts_data == NULL) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	ts_data->suspended = 0;
	ts_data->updating_fw = 0;
	ts_data->gsl_up_flag = 0;
	ts_data->gsl_point_state = 0;
	
#if GSL_HAVE_TOUCH_KEY
	ts_data->gsl_key_state = 0;
#endif

	ts_data->cinfo = kzalloc(sizeof(struct gsl_touch_info), GFP_KERNEL);
	if(ts_data->cinfo == NULL) {
		err = -ENOMEM;
		goto exit_alloc_cinfo_failed;
	}
	mutex_init(&ts_data->gsl_i2c_lock);//互斥锁初始化

	/* allocate input device */
	input_dev = input_allocate_device();//为输入子设备分配内存
	if (!input_dev) {
		err = -ENOMEM;
		goto err_allocate_input_device_fail;
	}

	//设置输入子设备的成员
	input_dev->name = client->name;
	input_dev->phys = "I2C";
	input_dev->dev.parent = &client->dev;
	input_dev->id.bustype = BUS_I2C;

	ts_data->input_dev = input_dev;
	ts_data->client = client;
	ts_data->pdata = pdata;
	i2c_set_clientdata(client, ts_data);
	dev_info("I2C addr = 0x%x\n", client->addr);//输出从设备地址

	err = gsl_ts_power_init(ts_data, true);
	if (err) {
		dev_err(&client->dev, "Silead power init failed\n");
		goto exit_power_init_err;
	}

	err = gsl_ts_power_on(ts_data, true);
	if (err) {
		dev_err(&client->dev, "Silead power on failed\n");
		goto exit_power_on_err;
	}

	//中断IO和复位IO申请设置
	if (gpio_is_valid(pdata->irq_gpio)) {
			err = gpio_request(pdata->irq_gpio, GSL_IRQ_NAME);//申请irq-gpio
			if (err) {
				dev_err(&client->dev, "irq gpio request failed\n");
				goto exit_request_irq_gpio_err;
			}
			err = gpio_direction_input(pdata->irq_gpio);//将irq-gpio方向设置为输入
			if (err) {
				dev_err(&client->dev,
					"set_direction for irq gpio failed\n");
				goto exit_set_irq_gpio_err;
			}
			client->irq = gpio_to_irq(pdata->irq_gpio);//获取irq-gpio的irq number
			printk("i2c-client-irq = %d\n", client->irq);

			#if 0
			err = gpio_export(pdata->irq_gpio, true);
			if (err) {
				dev_err(&client->dev, "irq gpio export failed\n");
				goto exit_set_irq_gpio_err;
			}
			#endif
	}

	if (gpio_is_valid(pdata->reset_gpio)) {
		err = gpio_request(pdata->reset_gpio, GSL_RST_NAME);//申请rest-gpio
		if (err) {
			dev_err(&client->dev, "reset gpio request failed");
			goto exit_request_reset_gpio_err;
		}
		err = gpio_direction_output(pdata->reset_gpio, 0);//将rest-gpio方向设置为输出，低电平
		if (err) {
			dev_err(&client->dev,
				"set direction for reset gpio failed\n");
			goto exit_set_reset_gpio_err;
		}
		msleep(20);
		gpio_set_value_cansleep(pdata->reset_gpio, 1);//reset gpio设置高电平

		#if 0
		err = gpio_export(pdata->reset_gpio, true);
		if (err) {
			dev_err(&client->dev, "reset gpio export failed\n");
			goto exit_set_reset_gpio_err;
		}
		#endif
	}

	/* make sure CTP already finish startup process */
	msleep(100);

	err = gsl_read_version();
	if(err < 0) {
		goto exit_i2c_transfer_fail; 
	}
	
	err = gsl_compatible_id(client);
	if(err < 0) {
		goto exit_i2c_transfer_fail; 
	}

#ifdef GSL_COMPATIBLE_GPIO
	gsl_gpio_idt_tp(client);  //no use this funtion
#endif

	/* request input system */	
	err = gsl_request_input_dev(ts_data);
	if(err < 0) {
		goto exit_i2c_transfer_fail;	
	}

	/* register early suspend */
#if defined(CONFIG_FB)
	dev_info("define CONFIG_FB\n");

	INIT_WORK(&ts_data->resume_work, gsl_ts_resume_work);	//初始化resume_work工作任务
	ts_data->fb_notif.notifier_call = fb_notifier_callback;
	err = fb_register_client(&ts_data->fb_notif);
	if (err)
		dev_err(&ts_data->client->dev,
			"Unable to register fb_notifier: %d\n", err);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	dev_info("define CONFIG_HAS_EARLYSUSPEND\n");

	ts_data->pm.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts_data->pm.suspend = gsl_early_suspend;
	ts_data->pm.resume = gsl_early_resume;
	register_early_suspend(&ts_data->pm);
#endif

	/* init work queue */
	INIT_WORK(&ts_data->work, gsl_report_work);	//初始化work工作任务
	ts_data->wq = create_singlethread_workqueue(dev_name(&client->dev));//创建wq工作队列
	if (!ts_data->wq) {	//判断进程是否创建成功
		err = -ESRCH;
		goto exit_create_singlethread;
	}

	/* request irq */
	err = request_irq(client->irq, gsl_ts_interrupt, IRQF_TRIGGER_FALLING, 
					client->name, ts_data);
	if (err < 0) {
		dev_err(&client->dev, "gslX68X_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	
	disable_irq_nosync(client->irq);//禁止irq中断

	/* gesture resume */
#ifdef GSL_GESTURE
	wake_lock_init(&gsl_wake_lock, WAKE_LOCK_SUSPEND, "gsl_wakelock");
	gsl_FunIICRead(gsl_read_oneframe_data);
#endif
	
	/* gsl of software init */
	gsl_sw_init(client);
	msleep(20);
	check_mem_data(client);

#ifdef TOUCH_VIRTUAL_KEYS
	gsl_ts_virtual_keys_init();
#endif

#ifdef TPD_PROC_DEBUG
	gsl_config_proc = proc_create(GSL_CONFIG_PROC_FILE, 0666, NULL, &gsl_seq_fops);
	
	if (gsl_config_proc == NULL) {
		printk("create_proc_entry %s failed\n", GSL_CONFIG_PROC_FILE);
	} else {
		printk("create proc entry %s success\n", GSL_CONFIG_PROC_FILE);
		//gsl_config_proc->read_proc = gsl_config_read_proc;
		//gsl_config_proc->write_proc = gsl_config_write_proc;
	}

	gsl_proc_flag = 0;
#endif
	
#ifdef GSL_TIMER
	INIT_DELAYED_WORK(&gsl_timer_check_work, gsl_timer_check_func);
	gsl_timer_workqueue = create_workqueue("gsl_timer_check");
	queue_delayed_work(gsl_timer_workqueue, &gsl_timer_check_work, 
					GSL_TIMER_CHECK_CIRCLE);
#endif
	
	err = sysfs_create_group(&client->dev.kobj, &gsl_attr_group);//创建属性文件

#ifdef GSL_GESTURE
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_POWER);
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_C);
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_E);
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_O);
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_W);
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_M);
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_Z);
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_V);
	input_set_capability(ts_data->input_dev, EV_KEY, KEY_S);
#endif
	   
#ifdef GSL_TEST_TP
	create_ctp_proc();
#endif

	gsl_read_version();
	enable_irq(client->irq);	//使能irq

#ifdef GSL_IRQ_WAKE
	if(gsl_irq_wake_en == 1)
		enable_irq_wake(client->irq);	//使能电源管理唤醒
#endif

#ifdef GSL_GESTURE
	enable_irq_wake(client->irq);
#endif

	dev_info("======================gsl_ts_probe over======================\n");
	return 0;

exit_irq_request_failed:
	cancel_work_sync(&ts_data->work);
	destroy_workqueue(ts_data->wq);
exit_create_singlethread:
#if defined(CONFIG_FB)
	if (fb_unregister_client(&ts_data->fb_notif))
		dev_err(&client->dev,
			"Error occurred while unregistering fb_notifier.\n");

#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&ts_data->pm);
#endif
 
	input_unregister_device(ts_data->input_dev);
	input_free_device(ts_data->input_dev);
exit_i2c_transfer_fail:
exit_set_reset_gpio_err:
	if (gpio_is_valid(ts_data->pdata->reset_gpio))
		gpio_free(ts_data->pdata->reset_gpio);
exit_request_reset_gpio_err:
exit_set_irq_gpio_err:
	if (gpio_is_valid(ts_data->pdata->irq_gpio))
		gpio_free(ts_data->pdata->irq_gpio);
exit_request_irq_gpio_err:
	gsl_ts_power_on(ts_data, false);
exit_power_on_err:
	gsl_ts_power_init(ts_data, false);
exit_power_init_err:
err_allocate_input_device_fail:
	i2c_set_clientdata(client, NULL);
	kfree(ts_data->cinfo);
exit_alloc_cinfo_failed:
	kfree(ts_data);
exit_alloc_data_failed:
exit_check_functionality_failed:
exit_parse_dt_err:
	devm_kfree(&client->dev, pdata);

	return err;
}

static int  gsl_ts_remove(struct i2c_client *client)
{
	struct gsl_ts_platform_data *pdata = ts_data->pdata;

	printk("\n");
	dev_info("======================remove start======================\n");

#ifdef TPD_PROC_DEBUG
	if(gsl_config_proc != NULL)
		remove_proc_entry(GSL_CONFIG_PROC_FILE, NULL);
#endif
	
	if (fb_unregister_client(&ts_data->fb_notif))
			dev_err(&client->dev,"Error occurred while unregistering fb_notifier.\n");

	free_irq(client->irq, ts_data);
	input_unregister_device(ts_data->input_dev);
	input_free_device(ts_data->input_dev);

	if (gpio_is_valid(pdata->irq_gpio))
		gpio_free(pdata->irq_gpio);
	if (gpio_is_valid(pdata->reset_gpio))
		gpio_free(pdata->reset_gpio);
	
	cancel_work_sync(&ts_data->work);
	destroy_workqueue(ts_data->wq);
	i2c_set_clientdata(client, NULL);

	kfree(ts_data->cinfo);
	kfree(ts_data);
	devm_kfree(&client->dev, pdata);

	dev_info("======================remove over======================\n");
	printk("\n");

	return 0;
}


static const struct i2c_device_id gsl_ts_id[] = {
	{ GSL_TS_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, gsl_ts_id);


#if defined(CONFIG_FB)
static struct of_device_id gsl_match_table[] = {
	{ .compatible = "silead,gsl-tp",},
	{ },
};
#else
#define gsl_match_table NULL
#endif

static struct i2c_driver gsl_ts_driver = {
	.driver = {
		.name = GSL_TS_NAME,
        .owner    = THIS_MODULE,
		.of_match_table = gsl_match_table,
	},
	.probe = gsl_ts_probe,
	.remove = gsl_ts_remove,
	.id_table	= gsl_ts_id,
};
 
module_i2c_driver(gsl_ts_driver);

MODULE_AUTHOR("sileadinc");
MODULE_DESCRIPTION("GSL Series Driver");
MODULE_LICENSE("GPL");

