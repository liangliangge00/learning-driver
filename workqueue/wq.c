#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>

struct my_work_struct {
	int value;
	struct work_struct ws;
};

static struct my_work_struct test;
static struct workqueue_struct *wq;

void fun(struct work_struct *p_work)
{
	struct my_work_struct *p = container_of(p_work,
		struct my_work_struct, ws);

	printk("value = %d\n", p->value);
}

int __init workqueue_test_init(void)
{
	int ret;
	wq = create_workqueue("test_workqueue");
	if (IS_ERR(wq)) {
		ret = PTR_ERR(wq);
		wq = NULL;
		return ret;
	}

	INIT_WORK(&(test.ws), fun);
	ret = queue_work_on(1, wq, &(test.ws));
	if (!ret) { 
		printk("work was already on a queue\n");
		return -EAGAIN;
	}
	printk("workqueue test init\n");

	return 0;
}

void __exit workqueue_test_exit(void)
{
	destroy_workqueue(wq);

	printk("workqueue test exit\n");
}

module_init(workqueue_test_init);
module_exit(workqueue_test_exit);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simplest workqueue test");
