/*
 *	Author: Nietzsche Zhong
 *	Date: 2011/04/11
 *	File: cm3217.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/slab.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#if 1
	#define dprintk(x, args...) do { printk(x, ##args); } while(0)
#else
	#define dprintk(x, args...) do { } while(0)
#endif

/* two addresses */
#define CM3217_SLAVE_ADDR_1 0x10
#define CM3217_SLAVE_ADDR_2 0x11

/* Command address : 0x20 */
#define CM3217_GAIN_EQUAL	(0 << 6)
#define CM3217_GAIN_HALF	(1 << 6)
#define CM3217_GAIN_QUARTERTH	(2 << 6)
#define CM3217_GAIN_EIGHTH	(3 << 6)

#define CM3217_RESERVE		(2 << 4)

#define CM3217_IT_TIMES_HALF	(0 << 2)
#define CM3217_IT_TIMES_EQUAL	(1 << 2)
#define CM3217_IT_TIMES_TWICE	(2 << 2)
#define CM3217_IT_TIMES_QUAD	(3 << 2)

#define CM3217_WDM_BYTE_MODE	(0 << 1)
#define CM3217_WDM_WORD_MODE	(1 << 1)

#define CM3217_ENABLE	(0 << 0)
#define CM3217_SUSPEND	(1 << 0)

#define EVENT_TYPE_LIGHT 0x3a

/* i2c_device */
static struct i2c_client * cm3217_client[2] = { NULL };
static struct i2c_board_info cm3217_info[1] = {
	{
		I2C_BOARD_INFO("cm3217", CM3217_SLAVE_ADDR_1),
	},
};

struct cm3217_data {
	struct work_struct work;
	struct input_dev * input;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif		
};

/* timer function */
static struct delayed_work cm_timer;
static struct cm3217_data *cm3217_data_globe = NULL;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cm3217_early_suspend(struct early_suspend *handler);
static void cm3217_late_resume(struct early_suspend *handler);
#endif /* CONFIG_HAS_EARLYSUSPEND */

static void cm_timer_handler(struct work_struct *work)
{
//	struct cm3217_data * data = container_of(work, struct cm3217_data, );
	schedule_work(&cm3217_data_globe->work);
	schedule_delayed_work(&cm_timer, msecs_to_jiffies(1000));
}

/* i2c_read */
inline int cm3217_read_high(struct i2c_client * client, char * buffer)
{
	struct i2c_msg msg[1];
	int rel;
	
	msg[0].addr = CM3217_SLAVE_ADDR_1;
	msg[0].flags = I2C_M_RD;
	msg[0].buf = buffer;
	msg[0].len = 1;

	rel = i2c_transfer(client->adapter, msg, 1);

	return 0;
}

inline int cm3217_read_low(struct i2c_client * client, char * buffer)
{
	struct i2c_msg msg[1];
	
	msg[0].addr = CM3217_SLAVE_ADDR_2;
	msg[0].flags = I2C_M_RD;
	msg[0].buf = buffer;
	msg[0].len = 1;

	i2c_transfer(client->adapter, msg, 1);

	return 0;
}

/* i2c_write */
inline int cm3217_write(struct i2c_client * client, char * buffer)
{
	struct i2c_msg msg[1];
	
	msg[0].addr = CM3217_SLAVE_ADDR_1;
	msg[0].flags = 0;
	msg[0].buf = buffer;
	msg[0].len = 1;

	i2c_transfer(client->adapter, msg, 1);

	return 0;
}

inline int cm3217_write2(struct i2c_client * client, char * buffer)
{
	struct i2c_msg msg[1];
	
	msg[0].addr = CM3217_SLAVE_ADDR_2;
	msg[0].flags = 0;
	msg[0].buf = buffer;
	msg[0].len = 1;

	i2c_transfer(client->adapter, msg, 1);

	return 0;
}

/* work func */
void cm_report_event(struct work_struct * work)
{
	struct cm3217_data * cmdata =
		container_of(work, struct cm3217_data, work);
	struct input_dev * input = cmdata->input;
	uint type = EV_ABS;
	uint code = EVENT_TYPE_LIGHT;
	static ushort value[5];
	static int i = 0;
	char data[10];

	data[0] = 0;
	data[1] = 0;

	cm3217_read_low(cm3217_client[0], data);
	cm3217_read_high(cm3217_client[0], data + 1);

	value[i] = (ushort)((data[1] << 8) + data[0]);
	//*resolution(step)
	value[i] = (value[i] * 6 / 10); // when FD_IT = 100ms

	//dprintk("::cm3217 data high: %d\t\t", data[1]);
	//dprintk("::cm3217 data low : %d\n", data[0]);

#if 0
	if (++i == 5) {
		value[4] += (value[0] + value[1] + value[2] + value[3]);
		value[4] /= 5;
		input_event(input, type, code, value[4]);
		input_sync(input);
		i = 0;
	}
#else		/* Henry Li: report event change from 5s to 2s as Lenovo want to speed up */
	if (++i == 2) {
		value[1] += value[0];
		value[1] /= 2;
		input_event(input, type, code, value[1]);
		input_sync(input);
		i = 0;
	}
#endif	

//	schedule_delayed_work(&cm_timer, msecs_to_jiffies(1000));
}

/* sysfs node */
static ssize_t cm3217_data_show(struct device * dev, struct device_attribute *attr, char * buf)
{
	char data[10];
	int lux;

	data[0] = 0;
	data[1] = 0;

	cm3217_read_low(cm3217_client[0], data);
	cm3217_read_high(cm3217_client[0], data + 1);

	dprintk("::cm3217 data high: %d\t\t", data[1]);
	dprintk("::cm3217 data low : %d\n", data[0]);

	lux = (data[1] << 8) + data[0];
	//*resolution(step)
	lux = (lux * 6 / 10); // when FD_IT = 100ms
	return sprintf(buf, "%d", lux);
}
static DEVICE_ATTR(data, 0666, cm3217_data_show, NULL);

static ssize_t cm3217_enable_store(struct device * dev, struct device_attribute *attr, char * buf, size_t count)
{
	ulong val;
	int error;
	char data[2];

	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;

	if (val) {
		data[0] = CM3217_RESERVE | CM3217_IT_TIMES_EQUAL 
			| CM3217_WDM_WORD_MODE | CM3217_ENABLE;
		cm3217_write(cm3217_client[0], data);
	}
	else {
		data[0] = CM3217_RESERVE | CM3217_IT_TIMES_EQUAL 
			| CM3217_WDM_WORD_MODE | CM3217_SUSPEND;
		cm3217_write(cm3217_client[0], data);
	}

	return count;
}
static DEVICE_ATTR(enable, 0666, NULL, cm3217_enable_store);

static struct attribute * cm3217_attributes[] = {
	&dev_attr_data.attr,
	&dev_attr_enable.attr,
	NULL
};

static const struct attribute_group cm3217_attr_group = {
	.attrs = cm3217_attributes,
};

/* i2c_driver */
static int __devinit cm3217_i2c_probe(struct i2c_client * client, 
		const struct i2c_device_id * id)
{
	int ret = 0;
	char data[10];

	struct cm3217_data * cmdata;
	struct input_dev * input;

	cmdata = kzalloc(sizeof(struct cm3217_data), GFP_KERNEL);
	if (!cmdata) {
		ret = -ENOMEM;
		goto exit;
	}
	i2c_set_clientdata(client, cmdata);

	/* input devide */
	input = input_allocate_device();
	if (!input) {
		ret = -ENOMEM;
		goto exit_kfree;
	}

	input->name = client->name;
	input->id.bustype = BUS_HOST;
	input->dev.parent = &client->dev;
	cmdata->input = input;
	ret = input_register_device(input);
	if (ret) {
		goto exit_kfree;
	}

	/* work init */
	INIT_WORK(&cmdata->work, cm_report_event);

	/* timer init */
	cm3217_data_globe = cmdata;
	
	INIT_DELAYED_WORK(&cm_timer, cm_timer_handler);
	schedule_delayed_work(&cm_timer, msecs_to_jiffies(1000));

	input_set_capability(input, EV_ABS, EVENT_TYPE_LIGHT);

	// word mode, IT_TIMES = 1T
	data[0] = CM3217_RESERVE | CM3217_IT_TIMES_EQUAL 
			| CM3217_WDM_WORD_MODE | CM3217_ENABLE;
	cm3217_write(client, data);
	
	// FD_IT, 100ms
	data[0] = 5 << 5;
	cm3217_write2(client, data);

#ifdef CONFIG_HAS_EARLYSUSPEND
	cmdata->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	cmdata->early_suspend.suspend = cm3217_early_suspend;
	cmdata->early_suspend.resume = cm3217_late_resume;
	register_early_suspend(&cmdata->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	
	ret = sysfs_create_group(&client->dev.kobj, &cm3217_attr_group);
	if (ret)
		goto exit_kfree;

	cm3217_read_low(cm3217_client[0], data);
	cm3217_read_high(cm3217_client[0], data + 1);

	dprintk("::cm3217 data high: %d\t\t", data[1]);
	dprintk("::cm3217 data low : %d\n", data[0]);

	return 0;
exit_kfree:
	kfree(cmdata);
exit:
	input_free_device(input);
	return ret;
}

static int __devexit cm3217_i2c_remove(struct i2c_client * client)
{
	sysfs_remove_group(&client->dev.kobj, &cm3217_attr_group);

	cancel_delayed_work_sync(&cm_timer);
	cancel_work_sync(&((struct cm3217_data *)i2c_get_clientdata(client))->work);
	kfree(i2c_get_clientdata(client));

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cm3217_early_suspend(struct early_suspend *handler)
{
	char data[2];

	cancel_delayed_work_sync(&cm_timer);
	data[0] = CM3217_RESERVE | CM3217_IT_TIMES_EQUAL 
		| CM3217_WDM_WORD_MODE | CM3217_SUSPEND;
	cm3217_write(cm3217_client[0], data);

	return 0;
}

static void cm3217_late_resume(struct early_suspend *handler)
{
	char data[2];

	schedule_delayed_work(&cm_timer, msecs_to_jiffies(1000));
	data[0] = CM3217_RESERVE | CM3217_IT_TIMES_EQUAL 
		| CM3217_WDM_WORD_MODE | CM3217_ENABLE;
	cm3217_write(cm3217_client[0], data);

	return 0;
}
#endif  /* CONFIG_HAS_EARLYSUSPEND */


/* power management */
#ifdef CONFIG_PM

static int cm3217_suspend(struct i2c_client * client, pm_message_t message)
{
	char data[2];

	data[0] = CM3217_RESERVE | CM3217_IT_TIMES_EQUAL 
		| CM3217_WDM_WORD_MODE | CM3217_SUSPEND;
	cm3217_write(cm3217_client[0], data);

	return 0;
}

static int cm3217_resume(struct i2c_client * client)
{
	char data[2];

	data[0] = CM3217_RESERVE | CM3217_IT_TIMES_EQUAL 
		| CM3217_WDM_WORD_MODE | CM3217_ENABLE;
	cm3217_write(cm3217_client[0], data);

	return 0;
}

#else

#define cm3217_suspend	NULL
#define cm3217_resume	NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id cm3217_id[] = {
	{ "cm3217", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cm3217_id);

static struct i2c_driver cm3217_driver = {
	.driver = {
		.name = "cm3217",
		.owner = THIS_MODULE,
	},
//	.suspend = cm3217_suspend,
//	.resume = cm3217_resume,
	.probe = cm3217_i2c_probe,
	.remove = __devexit_p(cm3217_i2c_remove),
	.id_table = cm3217_id,
};

/* entry point */
static int __init cm3217_i2c_init(void)
{
	struct i2c_client * new_client;
	struct i2c_adapter * this_adap;

	/* register i2c_device */
	if ((this_adap = i2c_get_adapter(2)) == NULL) 
	{
		dprintk("::no adapter\n");
		return -1;
	}
	dprintk("::adapter is %d\n", this_adap->id);
	
	if ((new_client = i2c_new_device(this_adap, cm3217_info)) == NULL)
	{
		dprintk("::no new client\n");
		return -1;
	}
	cm3217_client[0] = new_client;

	if ((new_client = i2c_new_dummy(this_adap, CM3217_SLAVE_ADDR_2)) == NULL)
	{
		dprintk("::no new client\n");
		return -1;
	}
	cm3217_client[1] = new_client;

	dprintk("::device already ok\n");

	/* register i2c_driver */
	return i2c_add_driver(&cm3217_driver);
}
module_init(cm3217_i2c_init);

static void __exit cm3217_i2c_exit(void)
{
	i2c_unregister_device(cm3217_client[0]);
	i2c_unregister_device(cm3217_client[1]);
	
	i2c_del_driver(&cm3217_driver);
}
module_exit(cm3217_i2c_exit);

MODULE_ALIAS("cm3217");
MODULE_AUTHOR("Nietzsche Zhong");
MODULE_DESCRIPTION("CM3217 Lightsenor Driver");
MODULE_LICENSE("GPL");
