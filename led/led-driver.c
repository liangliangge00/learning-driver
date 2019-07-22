#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define LIGHT_MAJOR  231

struct light_dev {
    struct cdev cdev;
    unsigned char value;
};

struct light_dev *light_devp;

static int light_major = LIGHT_MAJOR;

static int __init light_init(void)
{

}

static void __exit light_exit(void)
{

}

module_init(light_init);
module_exit(light_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
