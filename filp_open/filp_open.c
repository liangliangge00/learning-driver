#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/dcache.h>
#include <asm/fcntl.h>
#include <asm/processor.h>
#include <asm/uaccess.h>

static int __init hello_init(void)
{
	int ret;
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	char string1[15] = "hello world,";
	char string2[15] = "kernel file.";
	char buf[30];
	int len;

	printk(KERN_INFO "=====hello_init=====\n");
	fp = filp_open("/home/hly/mac.cfg", O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		printk(KERN_INFO "/hoem/hly/mac.cfg open failed,err = %d\n", ret);
		return ret;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = fp->f_pos;
	vfs_write(fp, string1, strlen(string1), &pos);
	fp->f_pos = pos;

	pos = fp->f_pos;
	vfs_write(fp, string2, strlen(string2), &pos);
	fp->f_pos = pos;

	memset(buf, 0, sizeof(buf));
	len = strlen(string1) + strlen(string2);
	pos = 0;
	ret = vfs_read(fp, buf, len, &pos);
	if (ret < len) {
		ret = -EINVAL;
		printk(KERN_INFO "vfs read failed\n");
		return ret;
	}
	printk(KERN_INFO "f_pos = %lld,pos = %lld,buf = %s\n",
		fp->f_pos, pos, buf);

	set_fs(fs);
	filp_close(fp, NULL);

	return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple filp_open() test Module.");
MODULE_ALIAS("A simplest module");
