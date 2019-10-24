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
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/irq.h>

struct device_platform_data {
	const char *label;
	unsigned int gpio_num;
	unsigned int gpio_flag;
};

struct device_data {
	struct platform_device *pdev;
	struct input_dev *input_dev;
	struct device_platform_data *pdata;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct mutex dev_lock;
	int irq;
};

static struct device_data *dev_data = NULL;

static void device_report_work(struct work_struct *work)
{
	struct input_dev *input_dev = dev_data->input_dev;

	input_report_key(input_dev, KEY_7, 1);
	input_sync(input_dev);

	enable_irq(dev_data->irq);
}

static irqreturn_t device_interrupt(int irq, void *dev_id)
{
	disable_irq_nosync(dev_data->irq);

	if (!work_pending(&dev_data->work))
		queue_work(dev_data->wq, &dev_data->work);

	return IRQ_HANDLED;
}

static int 
device_request_input_dev(struct device_data *dev_data)
{
	struct input_dev *input_dev;
	struct platform_device *pdev = dev_data->pdev;
	int ret;

	/* allocate input device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "allocate input device failed\n");
		return -ENOMEM;
	}
	dev_data->input_dev = input_dev;

	input_dev->name = pdev->name;
	input_dev->dev.parent = &pdev->dev;
	input_dev->phys = "input-gpio";
	

	__set_bit(EV_KEY, input_dev->evbit);	//event bitmap
	__set_bit(KEY_7, input_dev->keybit);	//keycode bitmap

	/* request input device */
	ret = input_register_device(input_dev);
	if (ret)
		goto exit_register_input_dev_failed;

	return 0;
	
exit_register_input_dev_failed:
	input_free_device(input_dev);
	return ret;
}


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

static int input_gpio_probe(struct platform_device *pdev)
{
	struct device_platform_data *pdata;
	int ret;

	printk("\n==========input_gpio_probe start==========\n");
	printk("pdev-name = %s, dev-name = %s\n", pdev->name,
			dev_name(&pdev->dev));
	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev, 
			sizeof(struct device_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		ret = device_parse_dt(&pdev->dev, pdata);
		if (ret) {
			dev_err(&pdev->dev, "Failed to parse device tree\n");
			ret = -ENODEV;
			goto exit_parse_dt_err;
		}
	} else 
		pdata = pdev->dev.platform_data;

	dev_data = devm_kzalloc(&pdev->dev, 
		sizeof(struct device_data), GFP_KERNEL);
	if (!dev_data) {
		ret = -ENOMEM;
		goto exit_alloc_dev_data_err;
	}

	dev_data->pdev = pdev;
	dev_data->pdata = pdata;
	mutex_init(&dev_data->dev_lock);
	platform_set_drvdata(pdev, dev_data);

	if (gpio_is_valid(pdata->gpio_num)) {
		ret = gpio_request(pdata->gpio_num, pdata->label);
		if (ret) {
			dev_err(&pdev->dev, "Failed to request gpio number %d\n",
				pdata->gpio_num);
			goto exit_request_gpio_err;
		}

		ret = gpio_direction_input(pdata->gpio_num);
		if (ret) {
			dev_err(&pdev->dev, "Failed to set gpio input\n");
			goto exit_dir_input_err;
		}

		dev_data->irq = gpio_to_irq(pdata->gpio_num);
		if (dev_data->irq < 0) {
			dev_err(&pdev->dev, "Failed to tranform gpio number to irq number\n");
			ret = dev_data->irq;
			goto exit_tran_irq_num_err;
		}

		ret = gpio_export(pdata->gpio_num, true);
		if (ret) {
			dev_err(&pdev->dev, "Failed to export gpio number %d\n",
				pdata->gpio_num);
			goto exit_export_gpio_err;
		}
	}

	/* allocate and request input system */
	ret = device_request_input_dev(dev_data);
	if (ret) {
		dev_err(&pdev->dev, "request input device failed\n");
		goto exit_request_input_dev_err;
	}

	/* init work queue */
	INIT_WORK(&dev_data->work, device_report_work);
	dev_data->wq = create_singlethread_workqueue(pdev->name);
	if (!dev_data->wq) {
		dev_err(&pdev->dev, "Failed to create work queue\n");
		ret = -ESRCH;
		goto exit_create_workqueue_err;
	}

	/* request irq */
	ret = request_irq(dev_data->irq, device_interrupt, 
				IRQF_TRIGGER_HIGH, pdev->name, dev_data);
	if (ret) {
		dev_err(&pdev->dev, "request irq failed\n");
		goto exit_request_irq_err;
	}

	printk("==========input_gpio_probe over==========\n\n");
	return 0;


exit_request_irq_err:
	cancel_work_sync(&dev_data->work);
	destroy_workqueue(dev_data->wq);
exit_create_workqueue_err:
	input_unregister_device(dev_data->input_dev);
	input_free_device(dev_data->input_dev);
exit_request_input_dev_err:
exit_export_gpio_err:
exit_tran_irq_num_err:
exit_dir_input_err:
	gpio_free(pdata->gpio_num);
exit_request_gpio_err:
	devm_kfree(&pdev->dev, dev_data);
exit_alloc_dev_data_err:
exit_parse_dt_err:
	devm_kfree(&pdev->dev, pdata);

	return ret;
}

static int input_gpio_remove(struct platform_device *pdev)
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

static struct platform_device_id dev_match_table[] = {
	{ .name = "input-dev-gpio", .driver_data = 0, },
	{ },
};
MODULE_DEVICE_TABLE(platform, dev_match_table);

static struct of_device_id dev_of_match_table[] = {
	{ .compatible = "input-gpio",},
	{ },
};
MODULE_DEVICE_TABLE(of, dev_of_match_table);

static struct platform_driver input_gpio_driver = {
	.probe = input_gpio_probe,
	.remove = input_gpio_remove,
	.driver = {
		.name = "input-gpio",
		.owner = THIS_MODULE,
		.of_match_table = dev_of_match_table,
	},
	.id_table = dev_match_table,
};

module_platform_driver(input_gpio_driver);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
