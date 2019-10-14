#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>

static char mybuf[100] = "I am mybus.";
static struct bus_type mybus_type = {
	.name = "mybus",
};

static ssize_t mybus_show(struct bus_type *bus, char *buf)
{
	return sprintf(buf, "%s\n", mybuf);
}

static ssize_t mybus_store(struct bus_type *bus, const char *buf, size_t count)
{
	sprintf(mybuf, "%s", buf);
	return count;
}
static BUS_ATTR(mybus, 0644, &mybus_show, &mybus_store);

static struct device_driver mydriver = {
	.name = "mydriver",
	.bus = &mybus_type,
	.owner = THIS_MODULE,
};

static int __init bus_driver_init(void)
{
	int ret;

	ret = bus_register(&mybus_type);
	if (ret)
		return ret;
	ret = bus_create_file(&mybus_type, &bus_attr_mybus);
	if (ret)
		return ret;
	ret = driver_register(&mydriver);
	if (ret)
		return ret;

	printk("bus_driver init.\n");
	return 0;
}

static void __exit bus_driver_exit(void)
{
    driver_unregister(&mydriver);
	bus_remove_file(&mybus_type, &bus_attr_mybus);
	bus_unregister(&mybus_type);
	printk("bus_driver exit.\n");
}

module_init(bus_driver_init);
module_exit(bus_driver_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple driver module.");
