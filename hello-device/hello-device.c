#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

static int major = 230;
static int minor = 0;
static dev_t devno;

static struct class *hello_class;
static struct device *hello_device;

static int hello_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "hello-device open.\n");
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = hello_open,
};

static int __init hello_init(void)
{
	int ret;
	printk(KERN_INFO "hello init.\n");

	devno = MKDEV(major, minor);
	ret = register_chrdev(major, "hello-device", &fops);

	hello_class = class_create(THIS_MODULE, "hello-class");
	if (IS_ERR(hello_class)) {
		unregister_chrdev(major, "hello-device");
		return -EBUSY;
	}
	
	hello_device = device_create(hello_class, NULL, devno, NULL, "hello-device");
	if (IS_ERR(hello_device)) {
		class_destroy(hello_class);
		unregister_chrdev(major, "hello-device");
		return -EBUSY;
	}

	return 0;
}

static void __exit hello_exit(void)
{
	device_destroy(hello_class, devno);
	class_destroy(hello_class);
	unregister_chrdev(major, "hello-device");
	printk(KERN_INFO "hello exit.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("HLY");
MODULE_DESCRIPTION("A simple hello-device driver");
MODULE_LICENSE("GPL v2");

