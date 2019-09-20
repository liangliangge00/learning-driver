#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>

static struct class_compat *myclass;

static int __init myclass_init(void)
{
	int ret;

	myclass = class_compat_register("myclass");
	if  (IS_ERR(myclass)) {
		ret = PTR_ERR(myclass);
		myclass = NULL;
		return ret;
	}

	printk("myclass init.\n");
	return 0;
}

static void __exit myclass_exit(void)
{
    class_compat_unregister(myclass);
	printk("myclass exit.\n");
}

module_init(myclass_init);
module_exit(myclass_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple Hello World Module.");
MODULE_ALIAS("A simplest module");
