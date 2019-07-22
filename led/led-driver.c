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

static const struct file_operations light_fops = {
    .owner = THIS_MODULE;

};

static void light_setup_cdev(struct light_dev *dev, int index)
{
    int err, devno = MKDEV(light_major, index);

    cdev_init(&dev->cdev, &light_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &light_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_NOTICE "Error %d adding LED %d",err, index);
    } 
}

static void light_gpio_init(void)
{
    ...
}

static int __init light_init(void)
{
    int ret;
    dev_t devno = MKDEV(light_major, 0);

    if (light_major) {
        ret = register_chrdev_region(devno, 1, "LED");
    } else {
        ret = alloc_chrdev_region(&devno, 0, 1, "LED");
        light_major = MAJOR(devno);
    }
    if (ret < 0)
        return ret;

    light_devp = kzalloc(sizeof(struct light_dev), GFP_KERNEL);
    if (!light_devp) {
        ret = -ENOMEM;
        goto fail_malloc;
    }

    light_setup_cdev(light_devp, 0);
    light_gpio_init();
    return ret;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;    
}

static void __exit light_exit(void)
{
    cdev_del(&light_devp->cdev);
    kfree(light_devp);
    unregister_chrdev_region(MKDEV(light_major, 0), 1);
}

module_init(light_init);
module_exit(light_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
