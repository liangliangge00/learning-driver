#include <linux/module.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/fs.h>

static char mybuf[100] = "mydevice";
static int major;
static struct class *myclass;

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

static struct file_operations myfops = {
	.owner = THIS_MODULE,
};

static int __init mydevice_init(void)
{
	int ret;
	struct device *mydevice;
	
	major = register_chrdev(0, "mydevice", &myfops);
	if (major < 0) {
		ret = major;
		return ret;
	}
	
	myclass = class_create(THIS_MODULE, "myclass");
	if (IS_ERR(myclass)) {
		ret = PTR_ERR(myclass);
		goto fail;
	}
	
	mydevice = device_create(myclass, NULL, MKDEV(major, 0), NULL, "simple-device");
	if (IS_ERR(mydevice)) {
		class_destroy(myclass);
		ret = PTR_ERR(mydevice);
		goto fail;
	}

	ret = sysfs_create_file(&mydevice->kobj, &dev_attr_test_device.attr);
	if (ret < 0)
		return ret;

	return 0;

fail:
	unregister_chrdev(major, "mydevice");
	return ret;
}

static void __exit mydevice_exit(void)
{
	device_destroy(myclass, MKDEV(major, 0));
	class_destroy(myclass);
	unregister_chrdev(major, "mydevice");
}

module_init(mydevice_init);
module_exit(mydevice_exit);

MODULE_DESCRIPTION("A simplest driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("HLY");

