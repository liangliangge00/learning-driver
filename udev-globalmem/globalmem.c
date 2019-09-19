#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define GLOBALMEM_SIZE  0x1000
#define MEM_CLEAR  0x1
#define GLOBALMEM_MAJOR  230

static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);


unsigned char mem[GLOBALMEM_SIZE];	//4k memory

static struct class *globalmem_class;
static struct device *globalmem_device;

static int globalmem_open(struct inode *inode, struct file *filp)
{
	printk("globalmem->open() is called.\n");
    filp->private_data = mem;
    return 0;
}

static int globalmem_release(struct inode *inode, struct file *filp)
{
   printk("globalmem->close() is called.\n");
   return 0;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    unsigned char *mem = filp->private_data;
	
	printk("globalmem->ioctl() is called.\n");
    switch (cmd) {
    case MEM_CLEAR:
        memset(mem, 0, GLOBALMEM_SIZE);
        printk("globalmem->globalmem is set to zero.\n");
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    int ret = 0;
    unsigned char *mem = filp->private_data;

	printk("globalmem->read() is called.\n");
    if (p > GLOBALMEM_SIZE)
        return 0;
    if (count > GLOBALMEM_SIZE - p)
        count = GLOBALMEM_SIZE - p;

    if (copy_to_user(buf, mem + p, count)) {
        ret = -EFAULT;
    } else {
        *ppos += count;
        ret = count;

        printk("globalmem->read %u bytes(s) from %lu.\n", count, p);
    }

    return ret;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    int ret = 0;
    unsigned char *mem = filp->private_data;

	printk("globalmem->write() is called.\n");
    if (p > GLOBALMEM_SIZE)
        return 0;
    if (count > GLOBALMEM_SIZE - p)
        count = GLOBALMEM_SIZE - p;

    if (copy_from_user(mem + p, buf, count)) {
        return -EFAULT;
    } else {
        *ppos += count;
        ret = count;

        printk("globalmem->write %u bytes(s) from %lu.\n", count, p);
    }

    return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
    loff_t ret = 0;

	printk("globalmem->lseek() is called.\n");
    switch (orig) {
    case 0:
        if (offset < 0) {
            ret = -EINVAL;
            break;
        }
        if ((unsigned int)offset > GLOBALMEM_SIZE) {
            ret = -EINVAL;
            break;
        }
        filp->f_pos = (unsigned int)offset;
        ret = filp->f_pos;
        break;
    case 1:
        if ((filp->f_pos + offset) > GLOBALMEM_SIZE) {
            ret = -EINVAL;
            break;
        }
        if ((filp->f_pos + offset) < 0) {
            ret = -EINVAL;
            break;
        }
        filp->f_pos += offset;
        ret = filp->f_pos;
        break;
    default:
        ret = -EINVAL;
        break;
    }
	
    return ret;
}

static const struct file_operations globalmem_fops = {
    .owner = THIS_MODULE,
	.open = globalmem_open,
	.release = globalmem_release,
	.read = globalmem_read,
	.write = globalmem_write,
    .llseek = globalmem_llseek,
    .unlocked_ioctl = globalmem_ioctl, 
};

static int __init globalmem_init(void)
{
    int ret;
    dev_t devno;
	
	devno = MKDEV(globalmem_major, 0);
	ret = register_chrdev(globalmem_major, "globalmem", &globalmem_fops);
	printk("globalmem->register_chrdev()->ret = %d.\n", ret);
    if (ret < 0)
        return ret;
	
	globalmem_class = class_create(THIS_MODULE, "globalmem-class");	//The name of calss
	if (IS_ERR(globalmem_class)) {
		ret = -EBUSY;
		goto fail;
	}

	/* mknod /dev/globalmem */
	globalmem_device = device_create(globalmem_class, NULL, devno, NULL, "globalmem");
	if (IS_ERR(globalmem_device)) {
		class_destroy(globalmem_class);
		ret = -EBUSY;
		goto fail;
	}
	
	printk("globalmem->globalmem init.\n");
    return 0;

fail:
	unregister_chrdev(globalmem_major, "globalmem");
    return ret;
}

static void __exit globalmem_exit(void)
{
	dev_t devno;
	
	devno = MKDEV(globalmem_major, 0);
	device_destroy(globalmem_class, devno);	//delete device
	class_destroy(globalmem_class);	//delete class
	unregister_chrdev(globalmem_major, "globalmem");
	printk("globalmem->globalmem exit.\n");
}

module_init(globalmem_init);
module_exit(globalmem_exit);

MODULE_AUTHOR("HLY");
MODULE_DESCRIPTION("A simple globalmem driver");
MODULE_LICENSE("GPL v2");

