#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/mutex.h>

#define FALSE 	0
#define TRUE 	1

struct timer_led_drvdata {
	const char *dev_name;
	const char *gpio_label;
	int led_gpio;
	enum of_gpio_flags led_flag;
	
	struct timer_list timer;
	unsigned int timer_peroid;
	bool timer_state;
	
	bool led_state;
	
	struct mutex mutex_lock;
};

static void timer_led_function(unsigned long data)
{
	struct timer_led_drvdata *pdata = (struct timer_led_drvdata *)data;

	if (pdata->led_state) {
		gpio_set_value(pdata->led_gpio, FALSE);
		pdata->led_state = FALSE;
	} else {
		gpio_set_value(pdata->led_gpio, TRUE);
		pdata->led_state = TRUE;
	}

	mod_timer(&pdata->timer, jiffies + msecs_to_jiffies(pdata->timer_peroid));
}

static ssize_t ctrl_show(struct device *dev, 
	struct device_attribute *attr, char *buf)
{
	int ret;
	struct timer_led_drvdata *pdata = dev_get_drvdata(dev);

	if (pdata->timer_state)
		ret = snprintf(buf, PAGE_SIZE - 2, "enable");
	else
		ret = snprintf(buf, PAGE_SIZE - 2, "disable");
	
	buf[ret++] = '\n';
	buf[ret] = '\0';

	return ret;
}

static ssize_t ctrl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct timer_led_drvdata *pdata = dev_get_drvdata(dev);
	struct timer_list *timer = &pdata->timer;

	mutex_lock(&pdata->mutex_lock);
	if (0 == strncmp(buf, "enable", strlen("enable"))) {
		if (!pdata->timer_state) {
			timer->expires = jiffies + msecs_to_jiffies(pdata->timer_peroid);
			add_timer(timer);
			pdata->timer_state = TRUE;
			goto ret;
		}
	} else if (0 == strncmp(buf, "disable", strlen("disable"))) {
		if (pdata->timer_state) {
			if (gpio_get_value(pdata->led_gpio)) {
				gpio_set_value(pdata->led_gpio, FALSE);
				pdata->led_state = FALSE;
			}
			
			del_timer_sync(timer);
			pdata->timer_state = FALSE;
			goto ret;
		}
	}
	mutex_unlock(&pdata->mutex_lock);
	return 0;
	
ret:
	mutex_unlock(&pdata->mutex_lock);
	return strlen(buf);
}
static DEVICE_ATTR(ctrl, 0644, ctrl_show, ctrl_store);

static ssize_t gpio_show(struct device *dev, 
	struct device_attribute *attr, char *buf)
{
	int ret;
	struct timer_led_drvdata *pdata = dev_get_drvdata(dev);
	
	ret = snprintf(buf, PAGE_SIZE - 2, "timer-led-gpio: GPIO_%d",
			pdata->led_gpio - 911);
	buf[ret++] = '\n';
	buf[ret] = '\0';

	return ret;
}
static DEVICE_ATTR(gpio, 0444, gpio_show, NULL);

static ssize_t timer_peroid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	struct timer_led_drvdata *pdata = dev_get_drvdata(dev);

	ret = snprintf(buf, PAGE_SIZE - 2, "%d",
		pdata->timer_peroid);
	buf[ret++] = '\n';
	buf[ret] = '\0';

	return ret;
}

static ssize_t timer_peroid_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct timer_led_drvdata *pdata = dev_get_drvdata(dev);
	int ret;

	ret = kstrtouint(buf, 0, &pdata->timer_peroid);
	if (ret < 0) {
		dev_err(dev, "failed to convert string for timer peroid\n");
		return -EINVAL;
	}
	
	return strlen(buf);
}
static DEVICE_ATTR(timer_peroid, 0644, timer_peroid_show, 
			timer_peroid_store);

static struct attribute *timer_led_attr[] = {
	&dev_attr_ctrl.attr,
	&dev_attr_gpio.attr,
	&dev_attr_timer_peroid.attr,
	NULL
}; 

static const struct attribute_group attr_group = {
	.attrs = timer_led_attr,
};

static int timer_led_probe(struct platform_device *pdev)
{
	int ret;
	struct timer_led_drvdata *pdata;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	
	printk("[%s]==========timer_led driver probe start==========\n", __func__);
	if (!np)
		return -ENODEV;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;
	platform_set_drvdata(pdev, pdata);

	/* parse device tree node */
	ret = of_property_read_string(np, "dev,name", &pdata->dev_name);
	if (ret) {
		dev_err(dev, "failed to read property of dev,name\n");
		goto fail1;
	}

	ret = of_property_read_string(np, "gpio-label", &pdata->gpio_label);
	if (ret) {
		dev_err(dev, "failed to read property of gpio-label\n");
		goto fail1;
	}

	pdata->led_gpio = of_get_named_gpio_flags(np, "gpios", 0, &pdata->led_flag);
	if (pdata->led_gpio < 0) {
		dev_err(dev, "failed to read property of gpio\n");
		goto fail1;
	}

	/* init gpio */
	if (gpio_is_valid(pdata->led_gpio)) {
		ret = gpio_request_one(pdata->led_gpio, 
			pdata->led_flag, pdata->gpio_label);
		if (ret) {
			dev_err(dev, "failed to request the gpio\n");
			goto fail1;
		}

		ret = gpio_direction_output(pdata->led_gpio, 0);
		if (ret) {
			dev_err(dev, "failed to set gpio direction output\n");
			goto fail2;
		}

		ret = gpio_export(pdata->led_gpio, false);
		if (ret) {
			dev_err(dev, "failed to export gpio in sysfs\n");
			goto fail2;
		}
	} else {
		dev_err(dev, "the gpio of timer-led is not valid\n");
		goto fail1;
	}

	mutex_init(&pdata->mutex_lock);

	/* timer init here */
	init_timer(&pdata->timer);
	setup_timer(&pdata->timer, timer_led_function, (unsigned long)pdata);
	
	pdata->timer_state = FALSE;
	pdata->timer_peroid = 1000;
	pdata->led_state = FALSE;

	/* create attribute files */
	ret = sysfs_create_group(&dev->kobj, &attr_group);
	if (ret) {
		dev_err(dev, "Failed to create attribute files\n");
		goto fail2;
	}

	printk("[%s]==========timer_led driver probe over==========\n", __func__);
	return 0;
	
fail2:
	gpio_free(pdata->led_gpio);
fail1:
	kfree(pdata);
	return ret;
}

static int timer_led_remove(struct platform_device *pdev)
{
	struct timer_led_drvdata *pdata = platform_get_drvdata(pdev);

	if (gpio_is_valid(pdata->led_gpio))
		gpio_free(pdata->led_gpio);

	del_timer_sync(&pdata->timer);
	kfree(pdata);

	return 0;
}

static struct of_device_id timer_led_of_match[] = {
	{ .compatible = "timer-led", },
	{ },
};

static struct platform_driver timer_led_driver = {
	.probe = timer_led_probe,
	.remove = timer_led_remove,
	.driver = {
		.name = "timer_led_driver",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(timer_led_of_match),
	},
};

module_platform_driver(timer_led_driver);

MODULE_AUTHOR("HLY");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Driver for the timer led");
