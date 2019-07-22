#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define LIGHT_MAJOR  231
#define LIGHT_ON  1
#define LIGHT_OFF  0

struct light_dev {
    struct cdev cdev;
    unsigned char value;
};

struct light_dev *light_devp;

static int light_major = LIGHT_MAJOR;

static void light_gpio_init(void)
{
    ...
}

static void light_on(void)
{
    ...
}

static void light_off(void)
{
    ...
}

static ssize_t light_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    struct ligth_dev *dev = filp->private_data;
    if (copy_to_user(buf, &(dev->valus), 1)) 
        return -EFAULT;

    return 1;
}

static ssize_t light_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    struct light_dev *dev = filp->private_data;

    if (copy_from_user(&(dev->value), buf, 1))
        return -EFAULT;

    if (dev->value == 1)
        light_on();
    else
        light_off();

    return 1;
}

static long light_ioctl(struct file *filp, unsigned int cmd, unsigned  long arg)
{
    struct light_dev *dev = filp->private_data;

    switch (cmd) {
    case LIGHT_ON:
        dev->value = 1;
        light_on();
        break;
    case LIGHT_OFF:
        dev->value = 0;
        light_off();
        break;
    default:
        return -ENOTTY;
    }

    return 0;
}

static int light_open(struct inode *inode, struct file *filp)
{
    filp->private = light_devp;
    return 0;
}

static int light_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations light_fops = {
    .owner = THIS_MODULE,
    .read = light_read,
    .write = light_write,
    .unlocked_ioctl = light_ioctl,
    .open = light_open,
    .release = light_release,
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
