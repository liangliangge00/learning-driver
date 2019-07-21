#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALMEM_SIZE  0x1000
#define MEM_CLEAR  0x1
#define GLOBALMEM_MAJOR  230

static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);

struct globalmem_dev {
    struct cdev cdev;
    unsigned char mem[GLOBALMEM_SIZE];
}

struct globalmem_dev *globalmem_devp;

static void globalmem_setup_cdev(struct globalmem_dev *devp, int index)
{
    int err, devno = MKDEV(globalmem_major, index);

    
}

static int __init globalmem_init(void)
{

}

static void __exit globalmem_exit(void)
{

}

module_init(globalmem_init);
module_exit(globalmem_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");

