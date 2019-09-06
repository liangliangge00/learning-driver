#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>

static int foo;
static int baz;
static int bar;

static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%d\n", foo);
}
		
static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr,
		 const char *buf, size_t count)
{
	int ret;

	ret = kstrtoint(buf, 10, &foo);
	if (ret < 0)
		return ret;

	return count;
}
static struct kobj_attribute foo_attr = 
	__ATTR(foo, 0644, foo_show, foo_store);

static ssize_t b_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	int var;

	if (strcmp(attr->attr.name, "baz") == 0)
		var = baz;
	else
		var = bar;
	
	return sprintf(buf, "%d\n", var);
}

static ssize_t b_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	int var, ret;

	ret = kstrtoint(buf, 10, &var);
	if (ret < 0)
		return ret;

	if (strcmp(attr->attr.name, "baz") == 0)
		baz = var;
	else
		bar = var;
	return count;
}
static struct kobj_attribute baz_attr = 
	__ATTR(baz, 0644, b_show, b_store);
static struct kobj_attribute bar_attr = 
	__ATTR(bar, 0644, b_show, b_store);

static struct attribute *attrs[] = {
	&foo_attr.attr,
	&baz_attr.attr,
	&bar_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *example_kobj;

static int __init kobject_example_init(void)
{
	int ret = 0;

	example_kobj = kobject_create_and_add("example_kobj", kernel_kobj);
	if (!example_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(example_kobj, &attr_group);
	if (ret)
		kobject_put(example_kobj);
	
	return ret;
}

static void __exit kobject_example_exit(void)
{
	kobject_put(example_kobj);
}

module_init(kobject_example_init);
module_exit(kobject_example_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("HLY");

