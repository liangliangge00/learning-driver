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
#include <linux/timer.h>

struct device_platform_data {
	const char *label;
	unsigned int gpio_num;
	unsigned int gpio_flag;
};

struct device_data {
	struct platform_device *pdev;
	struct input_dev *input_dev;
	struct device_platform_data *pdata;
	struct timer_list timer;
	struct work_struct work;
	struct workqueue_struct *wq;
	int irq;
};

static void timer_function(unsigned long data)
{
	struct device_data *dev_data = (struct device_data *)data;
	struct input_dev *input_dev = dev_data->input_dev;
	struct device_platform_data *pdata = dev_data->pdata;
	int gpio_value;

	gpio_value = gpio_get_value(pdata->gpio_num);
	if (gpio_value == 0) {
		printk("gpio-value = %d\n", gpio_value);
	
		input_report_key(input_dev, KEY_8, 1);
		input_sync(input_dev);
		input_report_key(input_dev, KEY_8, 0);
		input_sync(input_dev);
	}
	
	enable_irq(dev_data->irq);
}

static void device_report_work(struct work_struct *work)
{
	struct device_data *dev_data = container_of(work, 
				struct device_data, work);
	
	mod_timer(&dev_data->timer, jiffies + msecs_to_jiffies(100));
}

static irqreturn_t device_interrupt(int irq, void *dev_id)
{
	struct device_data *dev_data = (struct device_data *)dev_id;

	disable_irq_nosync(dev_data->irq);	//disable irq
	
	if (!work_pending(&dev_data->work))
		queue_work(dev_data->wq, &dev_data->work);

	return IRQ_HANDLED;
}

static int 
device_request_input_dev(struct device_data *dev_data,
		struct platform_device *pdev)
{
	struct input_dev *input_dev;
	struct device_platform_data *pdata = dev_data->pdata;
	int ret;

	/* allocate input device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "allocate input device failed\n");
		return -ENOMEM;
	}
	dev_data->input_dev = input_dev;

	input_dev->name = pdata->label;
	input_dev->dev.parent = &pdev->dev;
	input_dev->phys = "input-gpio";
	
	__set_bit(EV_KEY | EV_SYN, input_dev->evbit);	//event bitmap
	__set_bit(KEY_8, input_dev->keybit);	//keycode bitmap

	/* request input device */
	ret = input_register_device(input_dev);
	if (ret)
		goto error;

	return 0;
	
error:
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
		dev_err(dev, "unable to read label\n");
		return ret;
	}
	printk("label = %s\n", pdata->label);

	pdata->gpio_num = of_get_named_gpio_flags(np, "gpios",
				0, &pdata->gpio_flag);
	if (pdata->gpio_num < 0) {
		dev_err(dev, "invalid gpio number %d\n", pdata->gpio_num);
		return -EINVAL;
	}
	printk("gpio-num = %d,gpio-flag = %d\n", pdata->gpio_num,
				pdata->gpio_flag);
	
	return 0;
}

static int timer_gpio_probe(struct platform_device *pdev)
{
	struct device_data *dev_data;
	struct device_platform_data *pdata;
	int ret;

	printk("\n[%s]==========input_gpio_probe start==========\n",
			__func__);
	printk("pdev-name = %s,dev-name = %s\n", pdev->name,
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
			goto fail1;
		}
	} else 
		pdata = pdev->dev.platform_data;

	dev_data = devm_kzalloc(&pdev->dev, 
				sizeof(struct device_data), GFP_KERNEL);
	if (!dev_data) {
		ret = -ENOMEM;
		goto fail1;
	}

	dev_data->pdev = pdev;
	dev_data->pdata = pdata;
	platform_set_drvdata(pdev, dev_data);

	if (gpio_is_valid(pdata->gpio_num)) {
		ret = gpio_request(pdata->gpio_num, pdata->label);
		if (ret) {
			dev_err(&pdev->dev, "failed to request gpio number %d\n",
				pdata->gpio_num);
			goto fail2;
		}

		ret = gpio_direction_input(pdata->gpio_num);
		if (ret) {
			dev_err(&pdev->dev, "failed to set gpio input\n");
			goto fail3;
		}

		dev_data->irq = gpio_to_irq(pdata->gpio_num);
		if (dev_data->irq < 0) {
			dev_err(&pdev->dev, "failed to tranform gpio number to irq number\n");
			ret = dev_data->irq;
			goto fail3;
		}

		ret = gpio_export(pdata->gpio_num, true);
		if (ret) {
			dev_err(&pdev->dev, "failed to export gpio number %d\n",
				pdata->gpio_num);
			goto fail3;
		}
	}

	/* allocate and request input system */
	ret = device_request_input_dev(dev_data, pdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to request input device\n");
		goto fail3;
	}

	/* init timer function */
	setup_timer(&dev_data->timer, timer_function, (unsigned long)dev_data);
	add_timer(&dev_data->timer);

	/* init work queue */
	INIT_WORK(&dev_data->work, device_report_work);
	dev_data->wq = create_singlethread_workqueue(pdev->name);
	if (!dev_data->wq) {
		dev_err(&pdev->dev, "failed to create work queue\n");
		ret = -ESRCH;
		goto fail4;
	}

	/* request irq */
	ret = request_any_context_irq(dev_data->irq, device_interrupt, 
		IRQF_TRIGGER_FALLING, pdev->name, dev_data);
	if (ret < 0) {
		dev_err(&pdev->dev, "request irq %d failed\n", dev_data->irq);
		goto fail5;
	}

	printk("[%s]==========input_gpio_probe over==========\n\n", 
			__func__);
	
	return 0;

fail5:
	cancel_work_sync(&dev_data->work);
	destroy_workqueue(dev_data->wq);
fail4:
	input_unregister_device(dev_data->input_dev);
	input_free_device(dev_data->input_dev);
	del_timer(&dev_data->timer);
fail3:
	gpio_free(pdata->gpio_num);
fail2:
	devm_kfree(&pdev->dev, dev_data);
fail1:
	devm_kfree(&pdev->dev, pdata);
	
	return ret;
}

static int timer_gpio_remove(struct platform_device *pdev)
{
	struct device_data *dev_data = platform_get_drvdata(pdev);
	struct device_platform_data *pdata = dev_data->pdata;

	/* release gpio resource */
	if (gpio_is_valid(pdata->gpio_num))
		gpio_free(pdata->gpio_num);

	/* stop work and destroy workqueue */
	cancel_work_sync(&dev_data->work);
	destroy_workqueue(dev_data->wq);

	/* release input subsystem */
	input_unregister_device(dev_data->input_dev);
	input_free_device(dev_data->input_dev);

	/* release timer */
	del_timer(&dev_data->timer);
	
	/* free irq resource */
	free_irq(dev_data->irq, dev_data);

	devm_kfree(&pdev->dev, pdata);
	devm_kfree(&pdev->dev, dev_data);

	return 0;
}

static struct platform_device_id dev_match_table[] = {
	{ .name = "timer-dev-gpio", .driver_data = 0, },
	{ },
};
MODULE_DEVICE_TABLE(platform, dev_match_table);

static struct of_device_id dev_of_match_table[] = {
	{ .compatible = "timer-gpio",},
	{ },
};
MODULE_DEVICE_TABLE(of, dev_of_match_table);

static struct platform_driver timer_gpio_driver = {
	.probe = timer_gpio_probe,
	.remove = timer_gpio_remove,
	.driver = {
		.name = "timer-gpio",
		.owner = THIS_MODULE,
		.of_match_table = dev_of_match_table,
	},
	.id_table = dev_match_table,
};

module_platform_driver(timer_gpio_driver);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
