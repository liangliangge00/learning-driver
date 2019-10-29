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
};

struct device_data {
	struct i2c_client *client;
	struct device_platform_data *pdata;
	struct mutex dev_i2c_lock;
	struct work_struct work;
	struct workqueue_struct *wq;
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

static int device_parse_dt(struct device *dev, 
						struct device_platform_data *pdata)
{
	int ret;
	struct device_node *np = dev->of_node;
	u32 temp;
	int i;
	
	pdata->name = "";
	ret = of_property_read_string(np, "dev,name", &pdata->name);
	if (ret && (ret != -EINVAL)) {
		dev_err(dev, "unable to read name\n");
		return ret;
	}
	printk("name = %s\n", pdata->name);
	
	ret = of_property_read_u32(np, "reg", &temp);
	if (!ret)
		pdata->reg = temp;
	else {
		dev_err(dev, "unable to read reg\n");
		return ret;
	}
	printk("reg = %x\n", pdata->reg);

	ret = device_get_dt_numbers(dev, "dev,first-numbers",
				pdata, FIRST_MAX);
	if (ret) {
		dev_err(dev, "unable to read dev,first-numbers\n");
		return ret;
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
		return ret;
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
		return -EINVAL;
	}
	printk("test-gpio = %d,test-gpio-flag = %d\n",
			pdata->test_gpio, pdata->test_gpio_flag);
	
	return 0;
}

static int device_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err = 0;
	struct device_data *dev_data;
	struct device_platform_data *pdata;
	int gpio_value;
	
	dev_info("==========driver probe start==========\n");
	printk("client-name = %s,dev-name = %s\n", client->name,
								dev_name(&client->dev));
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct device_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "failed to allocate memory\n");
			return -ENOMEM;
		}
		
		err = device_parse_dt(&client->dev, pdata);
		if (err) {
			dev_err(&client->dev, "failed to parse device tree\n");
			err = -ENODEV;
			goto fail1;
		}
	} else
		pdata = client->dev.platform_data;
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		dev_err(&client->dev, "I2C does not work\n");
		goto fail1;
	}
	
	dev_data = kzalloc(sizeof(struct device_data), GFP_KERNEL);
	if (!dev_data)	{
		err = -ENOMEM;
		goto fail1;
	}
	mutex_init(&dev_data->dev_i2c_lock);
	dev_data->client = client;
	dev_data->pdata = pdata;
	i2c_set_clientdata(client, dev_data);
	printk("I2C-addr = %x\n", client->addr);

	if (gpio_is_valid(pdata->test_gpio)) {
		err = gpio_request(pdata->test_gpio, "test_gpio");
		if (err) {
			dev_err(&client->dev, "test-gpio request failed\n");
			goto fail2;
		}
		gpio_value = gpio_get_value(pdata->test_gpio);
		printk("default gpio value: %d\n", gpio_value);

		err = gpio_direction_output(pdata->test_gpio, 1);
		if (err) {
			dev_err(&client->dev, "set gpio direction failed\n");
			goto fail3;
		}
		gpio_value = gpio_get_value(pdata->test_gpio);
		printk("curren gpio value: %d\n", gpio_value);

		err = gpio_export(pdata->test_gpio, true);
		if (err) {
			dev_err(&client->dev, "test-gpio export failed\n");
			goto fail3;
		}
		
	}
	
	dev_info("==========driver probe over==========\n");
	return 0;

fail3:
	gpio_free(pdata->test_gpio);
fail2:	
	kfree(dev_data);
fail1:
	devm_kfree(&client->dev, pdata);
	return err;
}

static int device_remove(struct i2c_client *client)
{
	struct device_data *dev_data;
	struct device_platform_data *pdata;
	
	dev_info("==========device remove start==========\n");
	dev_data = i2c_get_clientdata(client);
	if (!dev_data) {
		dev_err(&client->dev, "unable to get clientdata\n");
		return -EINVAL;
	}
	pdata = dev_data->pdata;

	if (gpio_is_valid(pdata->test_gpio))
		gpio_free(pdata->test_gpio);
	
	devm_kfree(&client->dev, dev_data->pdata);
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

static struct i2c_driver device_driver = {
	.driver = {
		.name = "virtual_device_driver",
		.owner = THIS_MODULE,
		.of_match_table = device_match_table,
	},
	.probe = device_probe,
	.remove = device_remove,
	.id_table = device_id,
};

module_i2c_driver(device_driver);

MODULE_AUTHOR("HLY");
MODULE_DESCRIPTION("I2C Device Driver");
MODULE_LICENSE("GPL v2");
