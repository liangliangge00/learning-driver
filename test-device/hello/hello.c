#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/kernel.h>


#define DEVICE_DEBUG

#ifdef DEVICE_DEBUG
#undef dev_info
#define dev_info(fmt, args...)		\
		do {				\
				printk("[hello-device][%s]"fmt, __func__, ##args);	\
		} while(0)
#else
#define dev_info(fmt, args...)
#endif

static int __init hello_init(void)
{
	dev_info("==========hello enter==========\n");
	return 0;
}

static void __exit hello_exit(void)
{
	dev_info("==========hello exit==========\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("HLY");
MODULE_DESCRIPTION("Hello Device Driver");
MODULE_LICENSE("GPL v2");
