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

//#include <mach/ldo.h>
//#include <mach/gpio.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
//#define GSL_PROXIMITY_SENSOR
#ifdef GSL_PROXIMITY_SENSOR
#include <linux/sensors.h>
#endif

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#include "gsl_ts_10inch.h"
#include "gsl_ts_7inch.h"
//#include "../ts_func_test.h"
#include "gsl_ts_5inch.h"
#include "gsl_ts_10inch_turkey.h"
#include "gsl_ts_7inch_tpx920.h"
#include "gsl_ts_5inch_tps508.h"
#include "gsl_ts_8inch_tpx930.h"
#include "gsl_ts_8inch_d2.h"

//add by lpl
#define V20X_TP   0  //10 inch for 800*1280 resolution LCD
#define V10X_TP   1  //7 inch  for 800*1280 resolution LCD
#define TPS465_TP 2  //5 inch  for 720*1280 resolution LCD
#define TPX910_TP 3  //10 inch turkey for 800*1280 resolution LCD
#define TPX920_TP 4  //7 inch  for 1024*600 resolution LCD
#define TPS508_TP 5  //5 inch for 720*1280 resolution LCD
#define TPS930_TP 6	 //8 inch for 800*1280 resolution LCD(SDM450触摸)
#define D2_TP 	  7	 //8 inch for 800*1280 resolution LCD(D2主屏触摸)

#define GSL_IRQ_WAKE				//中断唤醒系统

#define GSL_DEBUG 
#define GSL_TIMER					 //定时器开关
#define TPD_PROC_DEBUG				//proc文件系统接口红开关
#define GSL9XX_VDDIO_1800			0
#define GSL_REPORT_POINT_SLOT

//#define GSL_GESTURE
//#define GSL_COMPATIBLE_GPIO
#define GSL_ALG_ID

/* define i2c addr and device name */
#define GSL_TS_ADDR 				0x40
#define GSL_TS_NAME					"GSL_TP"

/* define irq and reset gpio num */
#define GSL_RST_GPIO_NUM	0
#define GSL_IRQ_GPIO_NUM	1
#define GSL_IRQ_NUM			gpio_to_irq(GSL_IRQ_GPIO_NUM)
//#define GSL_IRQ_NUM		sprd_alloc_gpio_irq(GSL_IRQ_GPIO_NUM)
#define GSL_IRQ_NAME		"gsl_irq"
#define GSL_RST_NAME		"gsl_reset"

#define GSL_COORDS_ARR_SIZE	4
#define GSL_VTG_MIN_UV		2600000
#define GSL_VTG_MAX_UV		3300000
#define GSL_I2C_VTG_MIN_UV	1800000
#define GSL_I2C_VTG_MAX_UV	1800000
#define MAX_BUTTONS			4

#if 0
/*vdd power*/
#define GSL_POWER_ON()	do {\
			} while(0)
#endif

#define GSL_TIMER_CHECK_CIRCLE		200
#define GSL_PRESSURE				50

/* debug of time */
#define TPD_DEBUG_TIME				0x20141209

/*define screen of resolution ratio*/
//#define GSL_MAX_X		800//600   
//#define GSL_MAX_Y		1280//1024

/*i2c of adapter number*/

/*virtual keys*/
//#define TOUCH_VIRTUAL_KEYS

/*button of key*/
#define GSL_HAVE_TOUCH_KEY 			1
#if GSL_HAVE_TOUCH_KEY
	struct key_data {
		u16 key;
		u32 x_min;
		u32 x_max;
		u32 y_min;
		u32 y_max;
	};
	//#define GSL_KEY_NUM	 3	 
	#define GSL_KEY_NUM	 4
	struct key_data gsl_key_data[GSL_KEY_NUM] = {
		{KEY_MENU,80,210,1330,1400},
		{KEY_HOMEPAGE,615,780,1330,1400},
		{KEY_BACK,420,610,1330,1400},
		{KEY_SEARCH, 240, 370, 1330, 1400},
	};
	struct key_data gsl_key_data_v200[GSL_KEY_NUM] = {
		{KEY_MENU,80,210,1200,1400},
		{KEY_HOMEPAGE,615,780,1200,1400},
		{KEY_BACK,420,610,1200,1400},
		{KEY_SEARCH, 240, 370, 1200, 1400},
	};
	
#endif

struct gsl_touch_info {
	int x[10];
	int y[10];
	int id[10];
	int finger_num;
};

struct gsl_ts_platform_data {
	const char *name;
	u32 irq_gpio;
	u32 irq_gpio_flags;
	u32 reset_gpio;
	u32 reset_gpio_flags;
	u32 x_max;
	u32 y_max;
	u32 x_min;
	u32 y_min;
	u32 panel_minx;
	u32 panel_miny;
	u32 panel_maxx;
	u32 panel_maxy;
	u32 num_max_touches;
	u32 button_map[MAX_BUTTONS];
	u32 num_buttons;
	u32 hard_reset_delay_ms;
	u32 post_hard_reset_delay_ms;
};

struct gsl_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct regulator *vdd;
	struct regulator *vcc_i2c;
	struct mutex gsl_i2c_lock;
	struct gsl_ts_platform_data *pdata;
	
	#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
	#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend pm;
	#endif

	struct gsl_touch_info *cinfo;
	bool suspended;							//0 normal;1 the machine is suspend;
	bool updating_fw;						//0 normal;1 download the firmware;
	u32 gsl_up_flag;						//0 normal;1 have one up event;
	u32 gsl_point_state;					//the point down and up of state	

	#ifdef GSL_TIMER
	struct delayed_work		timer_work;
	struct workqueue_struct	*timer_wq;
	volatile int gsl_timer_flag;			//0:first test	1:second test 2:doing gsl_load_fw
	unsigned int gsl_timer_data;		
	#endif
	
	#if GSL_HAVE_TOUCH_KEY
	int gsl_key_state;
	#endif
	
	struct work_struct resume_work;
	u8 fw_version;
	u8 pannel_id;
//	struct ts_func_test_device ts_test_dev;
};

/* Print Information */
#ifdef GSL_DEBUG
#undef dev_info
#define dev_info(fmt, args...)   \
        do{                              \
                printk("[tp-gsl][%s]"fmt,__func__, ##args);     \
        }while(0)
#else
#define dev_info(fmt, args...)   //
#endif

static unsigned char gsl_cfg_index = 0;

#ifdef GSL_ALG_ID
extern unsigned int gsl_version_id(void);
extern void gsl_alg_id_main(struct gsl_touch_info *cinfo);
extern void gsl_DataInit(int *ret);
extern unsigned int gsl_mask_tiaoping(void);
extern int gsl_obtain_gesture(void);
extern void gsl_FunIICRead(unsigned int (*fun) (unsigned int *,unsigned int,unsigned int));

#endif

/*
0 GSLX68X_FW_DJ数据对应v20x触摸屏  
1 GSLX68X_FW_CE数据对应v10x触摸屏
2 GSLX68X_FW_FJ数据对应tps_465触摸屏
3 GSLX68X_FW_TJ数据对应tpx_910触摸屏
4 GSLX68X_FW_EJ数据对应tpx_920触摸屏
5 GSLX68X_FW_508数据对应tps_508触摸屏
6 GSLX68X_FW_930数据对应tps_930触摸屏
7 GSLX68X_FW_D2数据对应D2主屏触摸屏
*/
static struct fw_config_type gsl_cfg_table[10] = {

/*0*/{GSLX68X_FW_DJ,(sizeof(GSLX68X_FW_DJ)/sizeof(struct fw_data)),
	gsl_config_data_id_DJ,(sizeof(gsl_config_data_id_DJ)/4)},
/*1*/{GSLX68X_FW_CE,(sizeof(GSLX68X_FW_CE)/sizeof(struct fw_data)),
	gsl_config_data_id_CE,(sizeof(gsl_config_data_id_CE)/4)},
/*2*/{GSLX68X_FW_FJ,(sizeof(GSLX68X_FW_FJ)/sizeof(struct fw_data)),
	gsl_config_data_id_FJ,(sizeof(gsl_config_data_id_FJ)/4)},
/*3*/{GSLX68X_FW_TJ,(sizeof(GSLX68X_FW_TJ)/sizeof(struct fw_data)),
	gsl_config_data_id_TJ,(sizeof(gsl_config_data_id_TJ)/4)},
/*4*/{GSLX68X_FW_EJ,(sizeof(GSLX68X_FW_EJ)/sizeof(struct fw_data)),
	gsl_config_data_id_EJ,(sizeof(gsl_config_data_id_EJ)/4)},
/*5*/{GSLX68X_FW_508,(sizeof(GSLX68X_FW_508)/sizeof(struct fw_data)),
	gsl_config_data_id_508,(sizeof(gsl_config_data_id_508)/4)},
/*6*/{GSLX68X_FW_930,(sizeof(GSLX68X_FW_930)/sizeof(struct fw_data)),
	gsl_config_data_id_930,(sizeof(gsl_config_data_id_930)/4)},
/*7*/{GSLX68X_FW_D2,(sizeof(GSLX68X_FW_D2)/sizeof(struct fw_data)),
	gsl_config_data_id_d2,(sizeof(gsl_config_data_id_d2)/4)},
/*8*/{NULL,0,NULL,0},
/*9*/{NULL,0,NULL,0},

};
