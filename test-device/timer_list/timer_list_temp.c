#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
	...
	...
		
struct device_drvdata {
	struct timer_list timer;
	...
};

/* 定时器超时调用此函数 */
static void timer_function(unsigned long data)
{
	struct device_drvdata *pdata = (struct device_drvdata *)data;
	
	/* 定时器的处理代码 */
	...
		
	/* 重新设置超时值并启动定时器 */
	mod_timer(pdata->timer, jiffies + msecs_to_jiffies(1000));
}

static int __init device_init(void)
{
	struct device_drvdata *pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;
	
	/* 设备的其它处理代码 */
	...
	
	/* 定时器初始化 */
	init_timer(&pdata->timer);
	/* 设置超时时间 */
	pdata->timer.expires = jiffies + msecs_to_jiffies(2000);
	/* 设置定时器超时调用函数以及传递的参数 */
	setup_timer(&pdata->timer, timer_function, (unsigned long)pdata);
	/* 启动定时器 */
	add_timer(&pdata->timer);
	
	....
	return 0;
}

static void __exit device_exit(void)
{
	/* 设备的其它处理代码 */
	...
		
	/* 删除定时器 */
	del_timer(&pdata->timer);
	
	...
}

module_init(device_init);
module_exit(device_exit);
