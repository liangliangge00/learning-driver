#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/mutex.h>

struct device_platform_data {
	const char *label;
	u32 gpio_num;
	u32 gpio_flag;
};

struct device_data {
	struct platform_device *pdev;
	struct device_platform_data *pdata;
	struct mutex dev_lock;
};

static int device_parse_dt(struct device *dev,
		struct device_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int ret;

	ret = of_property_read_string(np, "label", &pdata->label);
	if (ret && (ret != EINVAL)) {
		dev_err(dev, "Unable to read label\n");
		return ret;
	}
	printk("label = %s\n", pdata->label);

	pdata->gpio_num = of_get_named_gpio_flags(np, "gpios",
				0, &pdata->gpio_flag);
	if (pdata->gpio_num < 0) {
		dev_err(dev, "Invalid gpio number %d\n", pdata->gpio_num);
		return -EINVAL;
	}
	printk("gpio-num = %d,gpio-flag = %d\n", pdata->gpio_num,
				pdata->gpio_flag);
	
	return 0;
}

static int dev_gpio_probe(struct platform_device *pdev)
{
	struct device_data *dev_data;
	struct device_platform_data *pdata;
	int ret;
	int value;

	printk("\n==========dev_gpio_probe start==========\n");
	printk("pdev-name = %s, dev-name = %s\n", pdev->name,
			dev_name(&pdev->dev));
	
	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev, 
			sizeof(struct device_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "failed to allocate memory\n");
			return -ENOMEM;
		}

		ret = device_parse_dt(&pdev->dev, pdata);
		if (ret) {
			dev_err(&pdev->dev, "failed to parse device tree\n");
			ret = -ENODEV;
			goto err1;
		}
	} else 
		pdata = pdev->dev.platform_data;

	dev_data = devm_kzalloc(&pdev->dev, 
			sizeof(struct device_data), GFP_KERNEL);
	if (!dev_data) {
		ret = -ENOMEM;
		goto err1;
	}
	dev_data->pdev = pdev;
	dev_data->pdata = pdata;
	mutex_init(&dev_data->dev_lock);
	platform_set_drvdata(pdev, dev_data);

	if (gpio_is_valid(pdata->gpio_num)) {
		ret = gpio_request(pdata->gpio_num, pdata->label);
		if (ret) {
			dev_err(&pdev->dev, "failed to request gpio number %d\n",
				pdata->gpio_num);
			goto err2;
		}
		
		value = gpio_get_value(pdata->gpio_num);
		printk("default gpio value is %d\n", value);

		ret = gpio_direction_output(pdata->gpio_num, 1);
		if (ret) {
			dev_err(&pdev->dev, "failed to set gpio output\n");
			goto err3;
		}
		
		value = gpio_get_value(pdata->gpio_num);
		printk("current gpio value is %d\n", value);

		ret = gpio_export(pdata->gpio_num, true);
		if (ret) {
			dev_err(&pdev->dev, "failed to export gpio number %d\n",
				pdata->gpio_num);
			goto err3;
		}
	}

	printk("==========dev_gpio_probe over==========\n\n");
	return 0;
	
err3:
	gpio_free(pdata->gpio_num);
err2:
	devm_kfree(&pdev->dev, dev_data);
err1:
	devm_kfree(&pdev->dev, pdata);

	return ret;
}

static int dev_gpio_remove(struct platform_device *pdev)
{
	struct device_data *dev_data = platform_get_drvdata(pdev);
	struct device_platform_data *pdata = dev_data->pdata;

	if (gpio_is_valid(pdata->gpio_num))
		gpio_free(pdata->gpio_num);

	devm_kfree(&pdev->dev, pdata);
	pdata = NULL;
	devm_kfree(&pdev->dev, dev_data);
	dev_data = NULL;

	return 0;
}

static const struct platform_device_id dev_match_table[] = {
	{ .name = "gpio-node", .driver_data = 0 },
	{ },
};
MODULE_DEVICE_TABLE(platform, dev_match_table);

static struct of_device_id device_match_table[] = {
	{ .compatible = "gpio-node",},
	{ },
};
MODULE_DEVICE_TABLE(of, device_match_table);

static struct platform_driver dev_gpio_driver = {
	.probe = dev_gpio_probe,
	.remove = dev_gpio_remove,
	.driver = {
		.name = "gpio-node-test",
		.owner = THIS_MODULE,
		.of_match_table = device_match_table,
	},
	.id_table = dev_match_table,
};

module_platform_driver(dev_gpio_driver);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
