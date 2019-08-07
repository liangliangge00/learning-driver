#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

static int major = 230;
static int minor = 0;
static dev_t devno;
static struct class *cls;
static struct device *test_device;

static int hello_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "hello open\n");
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = hello_open,
};

static int hello_init(void)
{
	int ret;
	printk(KERN_INFO "hello init\n");

	devno = MKDEV(major, minor);
	ret = register_chrdev(major, "hello", &fops);

	cls = class_create(THIS_MODULE, "myclass");
	if (IS_ERR(cls)) {
		unregister_chrdev(major, "hello");
		return -EBUSY;
	}
	
	test_device = device_create(cls, NULL, devno, NULL, "hello");
	if (IS_ERR(test_device)) {
		class_destroy(cls);
		unregister_chrdev(major, "hello");
		return -EBUSY;
	}

	return 0;
}

static void hello_exit(void)
{
	device_destroy(cls, devno);
	class_destroy(cls);
	unregister_chrdev(major, "hello");
	printk(KERN_INFO "hello exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_AUTHOR("HLY");
MODULE_DESCRIPTION("A simplest module");
MODULE_LICENSE("GPL v2");



