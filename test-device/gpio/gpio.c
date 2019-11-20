#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/sysfs.h>

struct gpio_platform_data {
	const char *label;
	unsigned int gpio_num;
	enum of_gpio_flags gpio_flag;
};

struct gpio_drvdata {
	struct gpio_platform_data *pdata;
	
	bool gpio_state;
};

static ssize_t ctrl_show(struct device *dev, 
	struct device_attribute *attr, char *buf)
{
	struct gpio_drvdata *ddata = dev_get_drvdata(dev);
	int ret;

	if (ddata->gpio_state)
		ret = snprintf(buf, PAGE_SIZE - 2, "%s", "enable");
	else
		ret = snprintf(buf, PAGE_SIZE - 2, "%s", "disable");
	
	buf[ret++] = '\n';
	buf[ret] = '\0';

	return ret;
}

static ssize_t ctrl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct gpio_drvdata *ddata = dev_get_drvdata(dev);
	bool state = ddata->gpio_state;

	if (!strncmp(buf, "enable", strlen("enable"))) {
		if (!state) {
			gpio_set_value(ddata->pdata->gpio_num, !state);
			ddata->gpio_state = !state;
			goto ret;
		}
	} else if (!strncmp(buf, "disable", strlen("disable"))) {
		if (state) {
			gpio_set_value(ddata->pdata->gpio_num, !state);
			ddata->gpio_state = !state;
			goto ret;
		}
	}

	return 0;

ret:
	return strlen(buf);
}
static DEVICE_ATTR(ctrl, 0644, ctrl_show, ctrl_store);

static ssize_t gpio_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gpio_drvdata *ddata = dev_get_drvdata(dev);
	int ret;

	ret = snprintf(buf, PAGE_SIZE - 2, "gpio-number: GPIO_%d",
		ddata->pdata->gpio_num - 911);
	buf[ret++] = '\n';
	buf[ret] = '\0';
	
	return ret;
}
static DEVICE_ATTR(gpio, 0444, gpio_show, NULL);

static struct attribute *gpio_attrs[] = {
	&dev_attr_ctrl.attr,
	&dev_attr_gpio.attr,
	NULL
};

static struct attribute_group attr_grp = {
	.attrs = gpio_attrs,
};

static struct gpio_platform_data *
gpio_parse_dt(struct device *dev)
{
	int ret;
	struct device_node *np = dev->of_node;
	struct gpio_platform_data *pdata;
	
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "failed to alloc memory of platform data\n");
		return NULL;
	}

	ret = of_property_read_string(np, "label", &pdata->label);
	if (ret) {
		dev_err(dev, "failed to read property of lable\n");
		goto fail;
	}

	pdata->gpio_num = of_get_named_gpio_flags(np, "gpios",
				0, &pdata->gpio_flag);
	if (pdata->gpio_num < 0) {
		dev_err(dev, "invalid gpio number %d\n", pdata->gpio_num);
		ret = pdata->gpio_num;
		goto fail;
	}

	return pdata;

fail:
	kfree(pdata);
	return ERR_PTR(ret);
}

static int gpio_probe(struct platform_device *pdev)
{
	struct gpio_drvdata *ddata;
	struct gpio_platform_data *pdata;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	int ret;

	printk("[%s]==========gpio_probe start==========\n", __func__);
	
	if (!np) {
		dev_err(dev, "failed to find device node of gpio device\n");
		return -ENODEV;
	}

	ddata = kzalloc(sizeof(*ddata), GFP_KERNEL);
	if (!ddata) {
		dev_err(dev, "failed to alloc memory for driver data\n");
		return -ENOMEM;
	}

	pdata = gpio_parse_dt(dev);
	if (IS_ERR(pdata)) {
		dev_err(dev, "failed to parse device node\n");
		ret = PTR_ERR(pdata);
		goto fail1;
	}

	if (gpio_is_valid(pdata->gpio_num)) {
		ret = gpio_request(pdata->gpio_num, pdata->label);
		if (ret) {
			dev_err(dev, "failed to request gpio number %d\n",
				pdata->gpio_num);
			goto fail2;
		}
		
		ret = gpio_direction_output(pdata->gpio_num, 0);
		if (ret) {
			dev_err(dev, "failed to set gpio direction for output\n");
			goto fail3;
		}
		
		ret = gpio_export(pdata->gpio_num, false);
		if (ret) {
			dev_err(dev, "failed to export gpio %d\n", pdata->gpio_num);
			goto fail3;
		}
	}

	ddata->gpio_state = false;
	ddata->pdata = pdata;
	platform_set_drvdata(pdev, ddata);

	ret = sysfs_create_group(&dev->kobj, &attr_grp);
	if (ret) {
		dev_err(dev, "failed to create sysfs files\n");
		goto fail3;
	}

	printk("[%s]==========gpio_probe over==========\n", __func__);
	return 0;
	
fail3:
	gpio_free(pdata->gpio_num);
fail2:
	kfree(pdata);
fail1:
	kfree(ddata);
	return ret;
}

static int gpio_remove(struct platform_device *pdev)
{
	struct gpio_drvdata *ddata = platform_get_drvdata(pdev);
	struct gpio_platform_data *pdata = ddata->pdata;

	sysfs_remove_group(&pdev->dev.kobj, &attr_grp);
	
	if (gpio_is_valid(pdata->gpio_num))
		gpio_free(pdata->gpio_num);

	kfree(pdata);
	pdata = NULL;
	
	kfree(ddata);
	ddata = NULL;

	return 0;
}

static struct of_device_id device_match_table[] = {
	{ .compatible = "dev-gpio",},
	{ },
};
MODULE_DEVICE_TABLE(of, device_match_table);

static struct platform_driver dev_gpio_driver = {
	.probe = gpio_probe,
	.remove = gpio_remove,
	.driver = {
		.name = "dev-gpio",
		.owner = THIS_MODULE,
		.of_match_table = device_match_table,
	},
};

module_platform_driver(dev_gpio_driver);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
