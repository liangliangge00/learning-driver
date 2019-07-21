#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

struct xxx_dev_t {
    struct cdev cdev;
    ...
}

struct file_operations xxx_fops {
    .owner = THIS_MODULE;
    .read = xxx_read;
    .write = xxx_write;
    .unlocked_ioctl = xxx_ioctl;
    ...
}

static int __init xxx_init(void)
{
    ...
    cdev_init(&xxx_dev.cdev, &xxx_fops);
    xxx_dev.cdev.owner = THIS_MODULE;

    if (xxx_major) {
        register_chrdev_region(xxx_dev_no, 1, DEV_NAME);
    } else {
        alloc_chrdev_region(&xxx_dev_no, 0, 1, DEV_NAME);
    }
    ret = cdev_add(&xxx_dev.cdev, xxx_dev_no, 1);
    ...
    
    return 0;
}

static void __exit xxx_exit(void)
{
    unregister_chrdev_region(xxx_dev_no, 1);
    cdev_del(&xxx_dev.cdev);
    ...
}

ssize_t xxx_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ...
    copy_to_user(buf, ..., ...);
    ...
}

ssize_t xxx_write(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ...
    copy_from_user(..., buf, ...);
    ...
}

long xxx_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    ...
    switch (cmd) {
    case XXX_CMD1:
        ...
        break;
    case XXX_CMD2:
        ...
        break;
    default:
        return -ENOTTY;
    }
    
    return 0;
}


module_init(xxx_init);
module_exit(xxx_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");

