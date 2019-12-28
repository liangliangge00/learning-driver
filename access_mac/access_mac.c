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

#define MAC_LEN 17

static int __init access_mac_init(void)
{
	int i,ret;
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	char buf[20];
	unsigned int mac[6];

	printk(KERN_INFO "=====access_mac_init=====\n");
	fp = filp_open("/home/hly/mac.cfg", O_RDONLY, 0);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		printk(KERN_INFO "/hoem/hly/mac.cfg open failed,err = %d\n", ret);
		return ret;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;
	memset(buf, 0, sizeof(buf));
	ret = vfs_read(fp, buf, MAC_LEN, &pos);
	if (ret < MAC_LEN) {
		ret = -EINVAL;
		printk(KERN_INFO "vfs read mac address failed\n");
		goto ret;
	}
	
	printk(KERN_INFO "mac address: %s\n", buf);
	memset(mac, 0, sizeof(mac));
	ret = sscanf((char *)buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac, 
			mac+1, mac+2, mac+3, mac+4, mac+5);
	if (ret != 6) {
		ret = -EINVAL;
		printk(KERN_INFO "format string failed\n");
		goto ret;
	}
	for(i = 0; i < 6; i++) {
		printk(KERN_INFO "mac[%d] = %02x\n", i, mac[i]);
	}

	ret = 0;
ret:
	set_fs(fs);
	filp_close(fp, NULL);

	return ret;
}

static void __exit access_mac_exit(void)
{
    printk(KERN_INFO "access_mac_exit\n");
}

module_init(access_mac_init);
module_exit(access_mac_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple filp_open() test Module.");
MODULE_ALIAS("A simplest module");
