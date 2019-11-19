#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>


#define DEVICE_DEBUG
#define FIRST_MAX	4
#define SECOND_MAX	3

#ifdef DEVICE_DEBUG
#undef dev_info
#define dev_info(fmt, args...)		\
	do {				\
			printk("[i2c-device][%s]"fmt, __func__, ##args);	\
	} while(0)
#else
#define dev_info(fmt, args...)
#endif

struct device_platform_data {
	const char *name;
	u32 reg;
	u32 first_num[FIRST_MAX];
	u32 second_num[SECOND_MAX];
	u32 test_gpio;
	u32 test_gpio_flag;
	u32 memsize;
};

struct device_drvdata {
	struct i2c_client *client;
	struct device_platform_data *pdata;
	struct mutex dev_i2c_lock;
	struct work_struct work;
	struct workqueue_struct *wq;
	char *mem;
};

static int device_get_dt_numbers(struct device *dev, char *name,
			struct device_platform_data *pdata, int size)
{
	u32 numbers[size];
	struct property *prop;
	struct device_node *np = dev->of_node;
	int temp_size,ret,i;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	temp_size = prop->length / sizeof(u32);
	if (size != temp_size) {
		dev_err(dev, "invalid property: %s\n", name);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, name, numbers, size);
	if (ret && (ret != -EINVAL)) {
		dev_err(dev, "unable to read property: %s\n", name);
		return ret;
	}

	if (!strcmp(name, "dev,first-numbers")) {
		for (i = 0; i < size; i++)
			pdata->first_num[i] = numbers[i];
	} else if (!strcmp(name, "dev,second-numbers")) {
		for (i = 0; i < size; i++)
			pdata->second_num[i] = numbers[i];
	} else {
		dev_err(dev, "unsupported property: %s\n", name);
		return -EINVAL;
	}

	return 0;
}

static struct device_platform_data *
device_parse_dt(struct device *dev)
{
	int ret;
	struct device_platform_data *pdata;
	struct device_node *np = dev->of_node;
	u32 temp;
	int i;

	if (!np) {
		ret = -ENODEV;
		goto fail1;
	}

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		ret = -ENOMEM;
		goto fail1;
	}
	
	ret = of_property_read_string(np, "dev,name", &pdata->name);
	if (ret) {
		dev_err(dev, "unable to read name\n");
		goto fail1;
	}
	printk("name = %s\n", pdata->name);
	
	ret = of_property_read_u32(np, "reg", &temp);
	if (!ret)
		pdata->reg = temp;
	else {
		dev_err(dev, "unable to read reg\n");
		goto fail1;
	}
	printk("reg = 0x%x\n", pdata->reg);

	ret = device_get_dt_numbers(dev, "dev,first-numbers",
				pdata, FIRST_MAX);
	if (ret) {
		dev_err(dev, "unable to read dev,first-numbers\n");
		goto fail1;
	}
	printk("first-numbers: ");
	for (i = 0; i < FIRST_MAX; i++) {
		printk("%d ", pdata->first_num[i]);
		if (i == (FIRST_MAX - 1))
			printk("\n");
	}

	ret = device_get_dt_numbers(dev, "dev,second-numbers",
				pdata, SECOND_MAX);
	if (ret) {
		dev_err(dev, "unable to read dev,second-numbers\n");
		goto fail1;
	}
	printk("second-numbers: ");
	for (i = 0; i < SECOND_MAX; i++) {
		printk("%d ", pdata->second_num[i]);
		if (i == (SECOND_MAX - 1))
			printk("\n");
	}

	pdata->test_gpio = of_get_named_gpio_flags(np, "dev,test-gpio",
				0, &pdata->test_gpio_flag);
	if (pdata->test_gpio < 0) {
		dev_err(dev, "invald gpio: %d\n", pdata->test_gpio);
		ret = pdata->test_gpio;
		goto fail1;
	}
	printk("test-gpio = %d,test-gpio-flag = %d\n",
			pdata->test_gpio, pdata->test_gpio_flag);

	ret = of_property_read_u32(np, "dev,memsize", &temp);
	if (!ret) {
		pdata->memsize = temp;
		printk("memsize = 0x%x\n", pdata->memsize);
	} else {
		dev_err(dev, "unable to read memory size\n");
		goto fail1;
	}
	
	return pdata;

fail1:
	return ERR_PTR(ret);
}

static ssize_t virtual_device_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct i2c_client *client = container_of(dev, 
					struct i2c_client, dev);
	struct device_drvdata *dev_data = i2c_get_clientdata(client);

	ret = snprintf(buf, PAGE_SIZE - 2, "%s", dev_data->mem);
	buf[ret++] = '\n';
	buf[ret] = '\0';
	
	return ret;
}

static ssize_t virtual_device_store(struct device *dev, 
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct i2c_client *client = container_of(dev,
					struct i2c_client, dev);
	struct device_drvdata *dev_data = i2c_get_clientdata(client);

	ret = sprintf(dev_data->mem, "%s", buf);
	return ret;
}
static DEVICE_ATTR(vir_dev, 0644, virtual_device_show,
			virtual_device_store);
	
static ssize_t test_attr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	char *string = "test-attr:hello world\n";

	ret = snprintf(buf, PAGE_SIZE - 2, "%s", string);
	buf[ret++] = '\n';
	buf[ret] = '\0';
	
	return ret;
}
static DEVICE_ATTR(test_dev, 0444, test_attr_show, NULL);

static struct attribute *virtual_device_attrs[] = {
	&dev_attr_vir_dev.attr,
	&dev_attr_test_dev.attr,
	NULL
};

static struct attribute_group vir_dev_grp = {
	.attrs = virtual_device_attrs,
};

static int virtual_device_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err = 0;
	struct device *dev = &client->dev;
	struct device_drvdata *dev_data;
	struct device_platform_data *pdata;
	int gpio_value;
	
	dev_info("==========device driver probe start==========\n");
	printk("client-name = %s,dev-name = %s\n", client->name,
				dev_name(&client->dev));
	
	if (dev->of_node) {
		pdata = device_parse_dt(dev);
		if (IS_ERR(pdata)) {
			dev_err(dev, "failed to parse device tree\n");
			err = -ENODEV;
			goto fail1;
		}
	} else
		pdata = dev->platform_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		dev_err(dev, "i2c adapter does not work\n");
		goto fail1;
	}
	
	dev_data = kzalloc(sizeof(struct device_drvdata), GFP_KERNEL);
	if (!dev_data)	{
		err = -ENOMEM;
		goto fail1;
	}
	
	/* allocate memory for virtual device */
	dev_data->mem = kzalloc(pdata->memsize, GFP_KERNEL);
	if (!dev_data->mem) {
		dev_err(dev, 
			"failed to allocate virtual device memory\n");
		err = -ENOMEM;
		goto fail2;
	}
	mutex_init(&dev_data->dev_i2c_lock);
	dev_data->client = client;
	dev_data->pdata = pdata;
	i2c_set_clientdata(client, dev_data);
	printk("i2c-client-addr = %x\n", client->addr);

	if (gpio_is_valid(pdata->test_gpio)) {
		err = gpio_request(pdata->test_gpio, "test-gpio");
		if (err) {
			dev_err(dev, "test-gpio request failed\n");
			goto fail3;
		}
		gpio_value = gpio_get_value(pdata->test_gpio);
		printk("default gpio value: %d\n", gpio_value);

		err = gpio_direction_output(pdata->test_gpio, 1);
		if (err) {
			dev_err(dev, "set gpio direction failed\n");
			goto fail4;
		}
		gpio_value = gpio_get_value(pdata->test_gpio);
		printk("current gpio value: %d\n", gpio_value);

		err = gpio_export(pdata->test_gpio, true);
		if (err) {
			dev_err(dev, "test-gpio export failed\n");
			goto fail4;
		}
		
	}

	/* create virtual device attribute */
	err = sysfs_create_group(&dev->kobj, &vir_dev_grp);
	if (err) {
		dev_err(dev, "unable to export virtual device attrs\n");
		goto fail4;
	}
	
	dev_info("==========device driver probe over==========\n");
	return 0;

fail4:
	gpio_free(pdata->test_gpio);
fail3:
	kfree(dev_data->mem);
fail2:
	kfree(dev_data);
fail1:
	return err;
}

static int virtual_device_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct device_drvdata *dev_data;
	struct device_platform_data *pdata;
	
	dev_info("==========device remove start==========\n");
	dev_data = i2c_get_clientdata(client);
	if (!dev_data) {
		dev_err(dev, "unable to get clientdata\n");
		return -EINVAL;
	}
	pdata = dev_data->pdata;

	if (gpio_is_valid(pdata->test_gpio))
		gpio_free(pdata->test_gpio);

	kfree(dev_data->mem);
	sysfs_remove_group(&dev->kobj, &vir_dev_grp);
	
	kfree(dev_data->pdata);
	kfree(dev_data);
	
	dev_info("==========device remove over==========\n");
	return 0;
}

static struct i2c_device_id device_id[] = {
	{ .name = "test-device", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, device_id);

static struct of_device_id device_match_table[] = {
	{ .compatible = "test,i2c-device",},
	{ },
};
MODULE_DEVICE_TABLE(of, device_match_table);

static struct i2c_driver virtual_device_driver = {
	.driver = {
		.name = "virtual_device_driver",
		.owner = THIS_MODULE,
		.of_match_table = device_match_table,
	},
	.probe = virtual_device_probe,
	.remove = virtual_device_remove,
	.id_table = device_id,
};

module_i2c_driver(virtual_device_driver);

MODULE_AUTHOR("HLY");
MODULE_DESCRIPTION("I2C Device Driver");
MODULE_LICENSE("GPL v2");
