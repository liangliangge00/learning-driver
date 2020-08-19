/* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the SLIEAD's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/timer.h>
#include "wiegand.h"

struct wiegand_data {
	struct class *dev_class;
	struct device *dev;

	struct pinctrl *pctrl;
	struct pinctrl_state *pctrl_active;

	unsigned int data0_gpio;
	unsigned int data0_valid;
	unsigned int data1_gpio;
	unsigned int data1_valid;
	
	//方波周期 bit_width + bit_interval
	unsigned int bit_width;				//方波低电平保持时间,us
	unsigned int bit_interval;			//方波之间间隔时间,us
	//信号总时长 (bit_width + bit_interval) * bit_length
	unsigned int bit_length;			//方波数

	bool ctrl_mode_flag;

	unsigned int irq0;
	unsigned int irq1;
	bool irq0_flag;
	bool irq1_flag;
	spinlock_t irq_lock;

	bool new_flag;
	unsigned char data_index;
	char rev_data[DATA_MAX];

	struct platform_device *pdev;
	struct timer_list wiegand_timer;
	bool timer_is_running;

	struct mutex io_lock;
	struct mutex rev_data_lock;
};

static irqreturn_t wiegand_irq_handler_d0(int irq, void *dev_id);
static irqreturn_t wiegand_irq_handler_d1(int irq, void *dev_id);
static void wiegand_irq_disable(struct wiegand_data *p_data);
static void wiegand_irq_enable(struct wiegand_data *p_data);


#ifdef CONFIG_OF
static int parse_dt(struct device *dev, struct wiegand_data *p_data)
{
	int ret;
	struct device_node *node = dev->of_node;
	u32 u32_temp;
	unsigned int values[2];

	/*
	p_data->data0_gpio = of_get_named_gpio_flags(node, "data0-gpio", 0, 
		(enum of_gpio_flags *)&p_data->data0_valid);
	if (p_data->data0_gpio < 0) {
		dev_err(dev, "Unable to read data0-gpio\n");
		return -1;
	}

	p_data->data1_gpio = of_get_named_gpio_flags(node, "data1-gpio", 0, 
		(enum of_gpio_flags *)&p_data->data1_valid);
	if (p_data->data1_gpio < 0) {
		dev_err(dev, "Unable to read data1-gpio\n");
		return -1;
	}*/

	/* get data0-gpio pin */
	ret = of_property_read_u32_array(node, "data0-gpio", values, 2);
	if (ret) {
		dev_err(dev, "Unable to read data0-gpio\n");
		return ret;
	}
	p_data->data0_gpio = values[0];
	p_data->data0_valid = values[1];

	/* get data1-gpio pin */
	ret = of_property_read_u32_array(node, "data1-gpio", values, 2);
	if (ret) {
		dev_err(dev, "Unable to read data1-gpio\n");
		return ret;
	}
	p_data->data1_gpio = values[0];
	p_data->data1_valid = values[1];
	
	ret = of_property_read_u32(node, "bit-width", &u32_temp);
	if (!ret)
		p_data->bit_width = u32_temp;
	else {
		dev_err(dev, "Unable to read bit-width\n");
		p_data->bit_width = 400;
	}

	ret = of_property_read_u32(node, "bit-interval", &u32_temp);
	if (!ret)
		p_data->bit_interval = u32_temp;
	else {
		dev_err(dev, "Unable to read bit-interval\n");
		p_data->bit_interval = 1600;
	}
	
	ret = of_property_read_u32(node, "bit-length", &u32_temp);
	if (!ret)
		p_data->bit_length = u32_temp <= DATA_MAX ? u32_temp : DATA_MAX;
	else {
		dev_err(dev, "Unable to read bit-length\n");
		p_data->bit_interval = 26;
	}

	printk("data0-gpio = %d, data0-valid = %d\n"
		"data1-gpio = %d, data1-valid = %d\n"
		"bit-width = %d, bit-interval = %d, bit-length = %d\n",
		p_data->data0_gpio, p_data->data0_valid,
		p_data->data1_gpio, p_data->data1_valid,
		p_data->bit_width, p_data->bit_interval, p_data->bit_length);

	return 0;
}
#else
static int parse_dt(struct device *dev, struct wiegand_data *p_data)
{
	dev_err(dev, "no platform data defined\n");

	return ERR_PTR(-EINVAL);
}
#endif

static int setup_gpio_input(struct wiegand_data *p_data)
{
	int err = -EIO;

	wiegand_debug("%s: setup gpio direction input", __func__);

	err = gpio_request(p_data->data0_gpio, "wiegand_data0");
	if (err < 0) {
		printk(KERN_ERR "%s: data0 gpio request, err = %d\n", __func__, err);
		goto err_out;
	}
	
	err = gpio_direction_input(p_data->data0_gpio);
	if (err < 0) {
		printk(KERN_ERR "%s: data0 gpio direction, err = %d\n", __func__, err);
		goto err_data0_gpio;
	}
	wiegand_debug("%s: get data0_gpio input value, value = %d", __func__, 
		gpio_get_value(p_data->data0_gpio));

	err = gpio_request(p_data->data1_gpio, "wiegand_data1");
	if (err < 0) {
		printk(KERN_ERR "%s: data1 gpio request, err = %d\n", __func__, err);
		goto err_data0_gpio;
	}

	err = gpio_direction_input(p_data->data1_gpio);
	if (err < 0) {
		printk(KERN_ERR "%s: data1 gpio direction, err = %d\n", __func__, err);
		goto err_data1_gpio;
	}
	wiegand_debug("%s: get data1_gpio input value, value = %d", __func__, 
		gpio_get_value(p_data->data1_gpio));

	return 0;

err_data1_gpio:
	gpio_free(p_data->data1_gpio);
err_data0_gpio:
	gpio_free(p_data->data0_gpio);
err_out:
	return err;
}

static int setup_gpio_output(struct wiegand_data *p_data)
{
	int err = -EIO;

	wiegand_debug("%s: setup gpio direction output", __func__);

	err = gpio_request(p_data->data0_gpio, "wiegand_data0");
	if (err < 0) {
		printk(KERN_ERR "%s: data0 gpio request, err = %d\n", __func__, err);
		goto err_out;
	}
	
	err = gpio_direction_output(p_data->data0_gpio, p_data->data0_valid);
	if (err < 0) {
		printk(KERN_ERR "%s: data0 gpio direction, err = %d\n", __func__, err);
		goto err_data0_gpio;
	}

	err = gpio_request(p_data->data1_gpio, "wiegand_data1");
	if (err < 0) {
		printk(KERN_ERR "%s: data1 gpio request, err = %d\n", __func__, err);
		goto err_data0_gpio;
	}
	
	err = gpio_direction_output(p_data->data1_gpio, p_data->data1_valid);
	if (err < 0) {
		printk(KERN_ERR "%s: data1 gpio direction, err = %d\n", __func__, err);
		goto err_data1_gpio;
	}

	return 0;

err_data1_gpio:
	gpio_free(p_data->data1_gpio);
err_data0_gpio:
	gpio_free(p_data->data0_gpio);
err_out:
	return err;
}

static void free_gpio(struct wiegand_data *p_data)
{
	wiegand_debug("%s: free gpio resource", __func__);

	gpio_free(p_data->data1_gpio);
	gpio_free(p_data->data0_gpio);
}

static int setup_gpio_irq(struct wiegand_data *p_data)
{
	int err = -EIO;
	
	wiegand_debug("%s: request gpio irq resource", __func__);

	p_data->irq0 = gpio_to_irq(p_data->data0_gpio);
	if (p_data->irq0 < 0) {
		err = p_data->irq0;
		printk(KERN_ERR "--->%s[%d]err[%d]\n", __func__, __LINE__, err);
		goto err_detect_irq_num_failed;
	}

	p_data->irq1 = gpio_to_irq(p_data->data1_gpio);
	if (p_data->irq1 < 0) {
		err = p_data->irq1;
		printk(KERN_ERR "--->%s[%d]err[%d]\n", __func__, __LINE__, err);
		goto err_detect_irq_num_failed;
	}
	wiegand_debug("%s: irq0 = %d, irq1 = %d", __func__, p_data->irq0, p_data->irq1);

	err = request_irq(p_data->irq0, wiegand_irq_handler_d0,
			IRQF_TRIGGER_FALLING, p_data->pdev->name, p_data);
	if (err < 0) {
		printk(KERN_ERR "--->%s[%d]err[%d]\n", __func__, __LINE__, err);
		goto err_request_irq0;
	}
	wiegand_debug("%s: data0_gpio irq0 has requested", __func__);
	
	err = request_irq(p_data->irq1, wiegand_irq_handler_d1,
			IRQF_TRIGGER_FALLING, p_data->pdev->name, p_data);
	if (err < 0) {
		printk(KERN_ERR "--->%s[%d]err[%d]\n", __func__, __LINE__, err);
		goto err_request_irq1;
	}
	wiegand_debug("%s: data1_gpio irq1 has requested", __func__);

	return 0;

err_request_irq1:
	free_irq(p_data->irq0, p_data);
err_request_irq0:
err_detect_irq_num_failed:
	return err;
}

static void free_gpio_irq(struct wiegand_data *p_data)
{
	free_irq(p_data->irq0, p_data);
	free_irq(p_data->irq1, p_data);

	p_data->irq0 = -1;
	p_data->irq1 = -1;
}

static int reset_gpio(struct wiegand_data *p_data, int out)
{
	if(out == 1) {
		if((p_data->irq0 != -1) && (p_data->irq1 != -1))
			free_gpio_irq(p_data);

		free_gpio(p_data);
		setup_gpio_output(p_data);
	} else if(out == 0) {
		free_gpio(p_data);
		setup_gpio_input(p_data);
		setup_gpio_irq(p_data);
	}
	
	return 0;
}

static void wiegand_irq_disable(struct wiegand_data *p_data)
{
    unsigned long irqflags = 0;

    spin_lock_irqsave(&p_data->irq_lock, irqflags);
    disable_irq_nosync(p_data->irq0);
	disable_irq_nosync(p_data->irq1);
    spin_unlock_irqrestore(&p_data->irq_lock, irqflags);
}

static void wiegand_irq_enable(struct       wiegand_data *p_data)
{
    unsigned long irqflags = 0;

    spin_lock_irqsave(&p_data->irq_lock, irqflags);
    enable_irq(p_data->irq0);
	enable_irq(p_data->irq1);
    spin_unlock_irqrestore(&p_data->irq_lock, irqflags);
}

static void send_bit(struct wiegand_data *p_data, bool bit)
{
	if (bit) {
		gpio_set_value(p_data->data1_gpio, 0);
		udelay(p_data->bit_width);
		gpio_set_value(p_data->data1_gpio, 1);
	} else {
		gpio_set_value(p_data->data0_gpio, 0);
		udelay(p_data->bit_width);
		gpio_set_value(p_data->data0_gpio, 1);
	}
}

static int send_data(struct wiegand_data *p_data, int length, uint64_t data)
{
	long long i = 0;
	uint32_t data_h = 0;
	uint32_t data_l = 0;
	
	if (length > DATA_MAX)
		return -1;

	if (length > 32) {
		data_h = (data >> 32) & 0xffffffff;
		data_l = data & 0xffffffff;

		/* send hight 32 bit */
		for (i = length - 32 - 1; i >= 0; i--) {
			if (data_h & (1 << i))
				send_bit(p_data, 1);
			else
				send_bit(p_data, 0);
			udelay(p_data->bit_interval);
		}

		/* send low 32 bit */
		for (i = 31; i >= 0; i--) {
			if (data_l & (1 << i))
				send_bit(p_data, 1);
			else
				send_bit(p_data, 0);
			udelay(p_data->bit_interval);
		}
	} else {
		for (i = length - 1; i >= 0; i--) {
			if (data & (1 << i))
				send_bit(p_data, 1);
			else
				send_bit(p_data, 0);
			udelay(p_data->bit_interval);
		}

	}
	
	return 0;
}

static ssize_t bit_length_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	
	return scnprintf(buf, PAGE_SIZE, "%d\n", p_data->bit_length);
}

static ssize_t bit_length_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	uint32_t len = 0;

	sscanf(buf, "%d", &len);
	if (len > 0 && len <= DATA_MAX)
		p_data->bit_length = len;
	else
		printk(KERN_ERR "%s, invalid value %s\n", __func__, buf);

	return size;
}
static DEVICE_ATTR(bit_length, 0644, bit_length_show, bit_length_store);

static ssize_t bit_width_show(struct device *dev, 
	struct device_attribute *attr, char *buf)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", p_data->bit_width);
}

static ssize_t bit_width_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	uint32_t time = 0;

	sscanf(buf, "%d", &time);
	if (time > 0)
		p_data->bit_width = time;
	else
		printk(KERN_ERR "%s, invalid value %s\n", __func__, buf);

	return size;
}
static DEVICE_ATTR(bit_width, 0644, bit_width_show, bit_width_store);

static ssize_t bit_interval_show(struct device *dev, 
	struct device_attribute *attr, char *buf)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", p_data->bit_interval);
}

static ssize_t bit_interval_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	uint32_t time = 0;

	sscanf(buf, "%d", &time);
	if (time > 0)
		p_data->bit_interval = time;
	else
		printk(KERN_ERR "%s, invalid value %s\n", __func__, buf);

	return size;
}
static DEVICE_ATTR(bit_interval, 0644, bit_interval_show, bit_interval_store);

static ssize_t ctrl_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	bool flag = p_data->ctrl_mode_flag;

	if (flag == WIEGAND_SEND)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "send");
	else
		return scnprintf(buf, PAGE_SIZE, "%s\n", "receive");
}

static ssize_t ctrl_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);

	if (strncmp(buf, "send", strlen(buf) - 1) == 0) {
		reset_gpio(p_data, 1);
		p_data->ctrl_mode_flag = WIEGAND_SEND;
	} else if (strncmp(buf, "receive", strlen(buf) - 1) == 0) {
		reset_gpio(p_data, 0);
		p_data->ctrl_mode_flag = WIEGAND_RECEIVE;
	} else
		printk(KERN_ERR "%s, invalid value %s\n", __func__, buf);

	return size;
}
static DEVICE_ATTR(ctrl_mode, 0644, ctrl_mode_show, ctrl_mode_store);

static ssize_t send_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	uint64_t data = 0;
	
	p_data->irq0_flag = true;
	p_data->irq1_flag = true;

	sscanf(buf, "%lld", &data);

	mutex_lock(&p_data->io_lock);
	if (p_data->ctrl_mode_flag == WIEGAND_RECEIVE) {
		reset_gpio(p_data, 1);
		p_data->ctrl_mode_flag = WIEGAND_SEND;
	}

	send_data(p_data, p_data->bit_length, data);
	mutex_unlock(&p_data->io_lock);

	return size;
}
static DEVICE_ATTR(send, S_IWUSR, NULL, send_store);

static ssize_t receive_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	unsigned long long show_buf = 0;
	int i;
	ssize_t size = 0;
	
	wiegand_irq_disable(p_data);
	for (i = 0; i < p_data->bit_length; i++)
		show_buf = show_buf << 1 | p_data->rev_data[i];
	size = scnprintf(buf, PAGE_SIZE, "%llu\n", show_buf);
	wiegand_irq_enable(p_data);
	
	return size;
}
static DEVICE_ATTR(receive, S_IRUSR, receive_show, NULL);

static ssize_t new_flag_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	
	return scnprintf(buf, PAGE_SIZE, "%d\n", p_data->new_flag);
}

static ssize_t new_flag_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct wiegand_data *p_data = dev_get_drvdata(dev);
	int flag = true;
	
	sscanf(buf, "%d", &flag);
	if (flag == false) {
		p_data->new_flag = flag;
		memset(p_data->rev_data, 0, DATA_MAX);
		p_data->data_index = 0;
	} else
		printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
	
	return size;
}
static DEVICE_ATTR(new_flag, 0644, new_flag_show, new_flag_store);

static struct attribute *device_attr[] = {
	&dev_attr_bit_length.attr,
	&dev_attr_bit_width.attr,
	&dev_attr_bit_interval.attr,
	&dev_attr_ctrl_mode.attr,
	&dev_attr_send.attr,
	&dev_attr_receive.attr,
	&dev_attr_new_flag.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = device_attr,
};

static const struct attribute_group *attr_groups[] = {
	&attr_group,
	NULL,
};
 
static void wiegand_timer_callback(unsigned long data)
{
	struct wiegand_data *p_data = (struct wiegand_data *)data;
	
	if (p_data->data_index == p_data->bit_length) {
		p_data->new_flag = true;
		p_data->timer_is_running = false;
	} else
		mod_timer(&p_data->wiegand_timer, jiffies + msecs_to_jiffies(200));
}

static irqreturn_t wiegand_irq_handler_d0(int irq, void *dev_id)
{
	struct wiegand_data *p_data = (struct wiegand_data *)dev_id;

	if (p_data->irq0_flag) {
		p_data->irq0_flag = false;
		return IRQ_HANDLED;
	}

	if (p_data->new_flag) {
		p_data->new_flag = false;
		p_data->data_index = 0;
		memset(p_data->rev_data, 0, DATA_MAX);
	}

	p_data->rev_data[p_data->data_index] = 0;
	p_data->data_index++;

	if (!p_data->timer_is_running) {
		p_data->timer_is_running = true;	
		mod_timer(&p_data->wiegand_timer, jiffies + msecs_to_jiffies(200));
	}
	
	return IRQ_HANDLED;
}

static irqreturn_t wiegand_irq_handler_d1(int irq, void *dev_id)
{
	struct wiegand_data *p_data = (struct wiegand_data *)dev_id;

	if (p_data->irq1_flag) {
		p_data->irq1_flag = false;
		return IRQ_HANDLED;
	}

	if (p_data->new_flag) {
		p_data->new_flag = false;
		p_data->data_index = 0;
		memset(p_data->rev_data, 0, DATA_MAX);
	}

	p_data->rev_data[p_data->data_index] = 1;
	p_data->data_index++;

	if (!p_data->timer_is_running) {
		p_data->timer_is_running = true;
		mod_timer(&p_data->wiegand_timer, jiffies + msecs_to_jiffies(200));
	}

	return IRQ_HANDLED;
}

static int wiegand_probe(struct platform_device *pdev)
{
	struct wiegand_data *p_data;
	int err = -EINVAL;

	printk("[%s]: wiegand driver probe start\n", __func__);
	printk("driver version = %s\n", DRIVER_VERSION);

	p_data = kzalloc(sizeof(struct wiegand_data), GFP_KERNEL);
	if (!p_data) {
		printk(KERN_ERR "%s: failed to allocate p_data\n", __func__);
		return -ENOMEM;
	}
	
	mutex_init(&p_data->io_lock);
	mutex_init(&p_data->rev_data_lock);
	spin_lock_init(&p_data->irq_lock);

	err = parse_dt(&pdev->dev, p_data);
	if (err) {
		dev_err(&pdev->dev, "%s: parse_dt ret = %d\n", __func__, err);
		goto parse_dt_err;
	}

	platform_set_drvdata(pdev, p_data);

	p_data->dev_class = class_create(THIS_MODULE, "wiegand");
	if (IS_ERR(p_data->dev_class)) {
		printk(KERN_ERR "wiegand class_create failed\n");
		err = PTR_ERR(p_data->dev_class);
		p_data->dev_class = NULL;
		goto class_create_error;
	}
	
	p_data->dev = device_create_with_groups(p_data->dev_class, NULL,
		MKDEV(0, 0), p_data, attr_groups, "wiegand-send");
	if (IS_ERR(p_data->dev)) {
		printk(KERN_ERR "wiegand device create failed\n");
		err = PTR_ERR(p_data->dev);
		p_data->dev = NULL;
		goto device_create_error;
	}

	dev_set_drvdata(p_data->dev, p_data);

	/*
	p_data->pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(p_data->pctrl))
		dev_err(&pdev->dev, "pinctrl get failed\n");
	else {
		p_data->pctrl_active = pinctrl_lookup_state(p_data->pctrl, "default");
		if (IS_ERR(p_data->pctrl_active))
			dev_err(&pdev->dev, "pinctrl get state failed\n");
		else
			pinctrl_select_state(p_data->pctrl, p_data->pctrl_active);
	}*/

	setup_gpio_output(p_data);

	p_data->pdev = pdev;
	p_data->new_flag = false;
	memset(p_data->rev_data, 0, DATA_MAX);
	p_data->data_index = 0;
	
	setup_timer(&p_data->wiegand_timer, wiegand_timer_callback, (unsigned long)p_data);
	p_data->timer_is_running = false;
	
	p_data->irq0_flag = false;
	p_data->irq1_flag = false;

	printk("[%s]: wiegand driver probe over\n", __func__);
	
	return 0;

device_create_error:
	class_destroy(p_data->dev_class);
class_create_error:	
parse_dt_err:
	mutex_destroy(&p_data->rev_data_lock);
	mutex_destroy(&p_data->io_lock);
	kfree(p_data);

	printk("wiegand probe error\n");

	return err;
}

static int wiegand_remove(struct platform_device *pdev)
{
	struct wiegand_data *p_data = platform_get_drvdata(pdev);

	printk("[%s]: wiegand driver remove start\n", __func__);
	
	del_timer(&p_data->wiegand_timer);

	if (p_data->ctrl_mode_flag == WIEGAND_RECEIVE)
		free_gpio_irq(p_data);
	
	free_gpio(p_data);
	device_unregister(p_data->dev);
	class_destroy(p_data->dev_class);
	mutex_destroy(&p_data->io_lock);
	mutex_destroy(&p_data->rev_data_lock);
	kfree(p_data);
	
	printk("[%s]: wiegand driver remove over\n", __func__);
	
	return 0;
}

static struct of_device_id wiegand_match_table[] = {
	{ .compatible = "wiegand",},
	{ },
};

MODULE_DEVICE_TABLE(of, wiegand_match_table);

static struct platform_driver wiegand_driver = {
	.probe		= wiegand_probe,
	.remove		= wiegand_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = wiegand_match_table,
	}
};

static int __init wiegand_init(void)
{
	return platform_driver_register(&wiegand_driver);
}

static void __exit wiegand_exit(void)
{
	platform_driver_unregister(&wiegand_driver);
}

module_init(wiegand_init);
module_exit(wiegand_exit);

MODULE_DESCRIPTION("Wiegand driver");
MODULE_LICENSE("GPL");
