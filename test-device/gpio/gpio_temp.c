#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
...

struct gpio_drvdata {
    /* gpio号 */
    int gpio_num;
    ...
};

static int __init gpio_init(void)
{
   struct gpio_drvdata *ddata;
   int ret;
   
   ddata = kzalloc(sizeof(*ddata), GFP_KERNEL);
   if (!ddata)
	   return -ENOMEM;
	...
	
	/* gpio初始化 */
	if (gpio_is_valid(ddata->gpio_num)) {
		/* 申请gpio资源 */
		ret = gpio_request(ddata->gpio_num, "test-gpio");
		if (ret) {
			printk("failed to request gpio\n");
			return ret;
		}
		
		/* 设置gpio的方向(输出) */
		ret = gpio_direction_output(ddata->gpio_num, 0);
		if (ret) {
			printk("failed to set output direction\n");
			return ret;
		}
		
		/* 在sysfs中导出gpio(方向能改变) */
		ret = gpio_export(ddata->gpio_num, true);
		if (ret) {
			printk("failed to export gpio in sysfs\n");
			return ret;
		}
		
		/* 设置gpio电平值(高电平) */
		gpio_set_value(ddata->gpio_num, 1);
	}
	...
	
	return 0;
}

static void __exit gpio_exit(void)
{
	...
	/* 释放已经申请的gpio资源 */
	if (gpio_is_valid(ddata->gpio_num))
		gpio_free(ddata->gpio_num);
	...
}

module_init(gpio_init);
module_exit(gpio_exit);
