#include <linux/module.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/fs.h>

static char mybuf[100] = "I am a simplest device.";
static struct class *myclass;
static struct device *mydevice;

static ssize_t mydevice_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%s\n", mybuf);
}

static ssize_t mydevice_store(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	sprintf(mybuf, "%s", buf);

	return count;
}
static DEVICE_ATTR(test_device, 0644, mydevice_show, mydevice_store);


static int __init mydevice_init(void)
{
	int ret;
	
	myclass = class_create(THIS_MODULE, "myclass");
	if (IS_ERR(myclass)) {
		ret = PTR_ERR(myclass);
		myclass = NULL;
		return ret;
	}
	
	mydevice = device_create(myclass, NULL, MKDEV(0, 0), NULL, "mydevice");
	if (IS_ERR(mydevice)) {
		class_destroy(myclass);
		ret = PTR_ERR(mydevice);
		mydevice = NULL;
		return ret;
	}

	ret = device_create_file(mydevice, &dev_attr_test_device);
	if (ret < 0)
		return ret;

	return 0;
}

static void __exit mydevice_exit(void)
{
	device_destroy(myclass, MKDEV(0, 0));
	class_destroy(myclass);
}

module_init(mydevice_init);
module_exit(mydevice_exit);

MODULE_DESCRIPTION("A simplest driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("HLY");

