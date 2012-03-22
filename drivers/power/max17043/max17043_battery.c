/*
 *  max17043_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *  
 *  Modify for max17043 gauge by Aimar@Foxconn (2010.10.27)
 *  Added low battery alert by Aimar@Foxconn (2011.04.26)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/swab.h>
#include <linux/i2c.h>
#include <linux/idr.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
#include <linux/i2c/twl4030-madc.h>
#endif
#include <linux/max17043_battery.h>

#define MAX17040_VCELL_MSB	0x02
#define MAX17040_VCELL_LSB	0x03
#define MAX17040_SOC_MSB	0x04
#define MAX17040_SOC_LSB	0x05
#define MAX17040_MODE_MSB	0x06
#define MAX17040_MODE_LSB	0x07
#define MAX17040_VER_MSB	0x08
#define MAX17040_VER_LSB	0x09
#define MAX17040_RCOMP_MSB	0x0C
#define MAX17040_RCOMP_LSB	0x0D
#define MAX17040_CMD_MSB	0xFE
#define MAX17040_CMD_LSB	0xFF

#define MAX17040_DELAY		60*HZ
#define MAX17040_ALERT_BIT	(1<<5)

#define MAX17403_FAKE_SOC_DEBUG 1
#define MAX17043_INFO_PRESENTATION 1
#define MAX17043_LOW_BATTERY_NOTIFY 1

#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
/*
* Charging's temperature limit is base on H/W design.
* The design is 0~45 degrees centigrade
*/
#define TEMPERATURE_HOT_THRESHOLD  45
#define TEMPERATURE_COLD_THRESHOLD -5
#define TEMPERATURE_REMOVE_THRESHOLD -31
#define TEMPERATURE_ON_MADC_CHANNEL 0
#endif

#ifdef MAX17043_LOW_BATTERY_NOTIFY
#define LOW_BATTERY_THRESHOLD 10
#define CRITICAL_LOW_BATTERY_THRESHOLD 5
#define SHUT_DOWN_BATTERY_THRESHOLD 3
#define MAX_LOW_BATTERY_THRESHOLD 32
#endif

#define RESET_GAUGE_SOC_THRESHOLD 100

struct max17043_chip {
	struct i2c_client		*client;
	struct delayed_work		max17043_delay_work;
	struct power_supply		battery;
	struct max17043_platform_data	*pdata;
	/* State Of Connect */
	int online;
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
	/* battery temperature*/
	int temp;
#endif
	/* charging full */
	int charging_full;
	/* booting flag for check weather device is on booting */
	int booting;
	/* mamual reset gauge */
	int already_reset_gauge;
	/* reset count for gauge breakdown */
	int reset_gauge;
#ifdef MAX17403_FAKE_SOC_DEBUG
	/* enable fake soc as "non 1"*/
	int enable_fake_soc;
#endif
#ifdef MAX17043_LOW_BATTERY_NOTIFY
	struct delayed_work	max17043_low_battery_delay_work;
	int low_battery;
#endif
};

extern void max17043_write_custom_model(struct i2c_client *client);

#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
/* Get temperature from twl4030-madc converter */
extern int twl4030_madc_conversion(struct twl4030_madc_request *conv);
extern const struct max17043_temp_access max17043_temp_map[];
#endif

#ifdef CONFIG_REGULATOR_MAX8903
extern int max8903_charging_full(void);
extern void max8903_enable_charging(void);
#endif

/* 
* This function be called by gauge driver for update battery capacity.
* Due to we disable charger when charging full that will not recharging again under terminal charging threshold.
*/
static struct max17043_chip *g_max17043_chip = NULL;

static int max17043_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17043_chip *chip = container_of(psy,
				struct max17043_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->vcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if(chip->charging_full)
			val->intval = 100;
		else
			val->intval = chip->soc;
		break;
#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
	case POWER_SUPPLY_PROP_HEALTH:
		if(chip->temp > TEMPERATURE_HOT_THRESHOLD)
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		else if(chip->temp < TEMPERATURE_COLD_THRESHOLD && chip->temp > TEMPERATURE_REMOVE_THRESHOLD)
			val->intval = POWER_SUPPLY_HEALTH_COLD;
		else if(chip->temp <= TEMPERATURE_REMOVE_THRESHOLD)
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = chip->temp;
		break;
#else
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
#endif
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int max17043_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max17043_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}
/*
* The MAX17043/MAX17044 have six 16-bit registers:
* SOC, VCELL, MODE, VERSION, CONFIG, and COMMAND. Register reads and writes are only valid 
* if all 16 bits are transferred.
*/
static int max17043_write_bytes_reg(u8 reg, int wt_value, struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[3];
	int err;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 3;
	msg->buf = data;

	data[0] = reg;
	data[1] = wt_value >> 8;
	data[2] = wt_value & 0xFF;
	err = i2c_transfer(client->adapter, msg, 1);
	
	return err;
}

#ifdef MAX17043_LOW_BATTERY_NOTIFY
static ssize_t max17043_alert_low_battery_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 lsb = 0x00;
	u8 msb = 0x00;
	int value = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct max17043_chip *chip = i2c_get_clientdata(client);
	
	unsigned long val = simple_strtoul(buf, NULL, 10);
	if (val < 0 || val > 32)
		return -EINVAL;	
		
	msb = max17043_read_reg(client, MAX17040_RCOMP_MSB);
	lsb = max17043_read_reg(client, MAX17040_RCOMP_LSB);

	if(val == 0)
	{
		lsb = lsb & ~MAX17040_ALERT_BIT;
		lsb = lsb & ~(0x1F);
		lsb = lsb | (MAX_LOW_BATTERY_THRESHOLD - chip->low_battery);
		value = (msb<<8) | lsb;
	}
	else
		value = lsb;

	max17043_write_bytes_reg(MAX17040_RCOMP_MSB, value, client);
	//chip->low_battery = val;

	return count;
}
static DEVICE_ATTR(alert_low_battery, S_IRUGO|S_IWUSR|S_IWGRP /*S_IWUSR | S_IRWXUGO*/, NULL, max17043_alert_low_battery_store);
#endif

#ifdef MAX17403_FAKE_SOC_DEBUG
static ssize_t max17043_fake_soc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct max17043_chip *chip = i2c_get_clientdata(client);
	
	unsigned long val = simple_strtoul(buf, NULL, 10);
	if (val < 0 || val > 100)
		return -EINVAL;	
	if(val != 1)
		chip->enable_fake_soc = val;
	else
		chip->enable_fake_soc = 1;

	return count;
}
static DEVICE_ATTR(fake_soc, S_IRUGO|S_IWUSR|S_IWGRP /*S_IWUSR | S_IRWXUGO*/, NULL, max17043_fake_soc_store);
#endif

static ssize_t max17043_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);		
	unsigned long val = simple_strtoul(buf, NULL, 10);
	
	if (val < 0 || val > 1)
		return -EINVAL;	
	/* 
	* COMMAND register writes is not only valid if all 16 bits are transferred from experimentation. 
	* So need to write data separated.
	*/
	if(val == 1) /* Power-on Reset */
	{
		max17043_write_reg(client, MAX17040_CMD_MSB, 0x54);
		max17043_write_reg(client, MAX17040_CMD_LSB, 0x00);
	}
	else /* Quick-Start */
	{
		//max17043_write_reg(client, MAX17040_MODE_MSB, 0x40);
		//max17043_write_reg(client, MAX17040_MODE_LSB, 0x00);
		max17043_write_bytes_reg(MAX17040_MODE_MSB, 0x4000, client);
	}
	return count;
}
static DEVICE_ATTR(reset, S_IRUGO|S_IWUSR|S_IWGRP /*S_IWUSR | S_IRWXUGO*/, NULL, max17043_reset_store);

static ssize_t max17043_model_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);		
	unsigned long val = simple_strtoul(buf, NULL, 10);
	
	if (val < 0 || val > 1)
		return -EINVAL;	
	if(val == 1)
		max17043_write_custom_model(client);
	
	return count;
}
static DEVICE_ATTR(model_data, S_IRUGO|S_IWUSR|S_IWGRP /*S_IWUSR | S_IRWXUGO*/, NULL ,max17043_model_data_store);

static struct attribute *max17043_attributes[] = {
	&dev_attr_model_data.attr,
	&dev_attr_reset.attr,
#ifdef MAX17403_FAKE_SOC_DEBUG
	&dev_attr_fake_soc.attr,
#endif
#ifdef MAX17043_LOW_BATTERY_NOTIFY
	&dev_attr_alert_low_battery.attr,
#endif
	NULL
};
static const struct attribute_group max17043_attr_group = {
	.attrs = max17043_attributes,
};

void max17043_set_charging_full_state(int state)
{
	g_max17043_chip->charging_full = state;
}
EXPORT_SYMBOL(max17043_set_charging_full_state);

static void max17043_reset(struct i2c_client *client)
{
	max17043_write_reg(client, MAX17040_CMD_MSB, 0x54);
	max17043_write_reg(client, MAX17040_CMD_LSB, 0x00);
	printk("[Max17043 gauge]Enter %s, gauge reset finish.\n", __FUNCTION__);
}

static void max17043_quick_start(struct i2c_client *client)
{
	max17043_write_bytes_reg(MAX17040_MODE_MSB, 0x4000, client);
	printk("[Max17043 gauge]Enter %s, gauge quick start!!\n", __FUNCTION__);
}

static void max17043_get_vcell(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	u8 msb = 0x00;
	u8 lsb = 0x00;

	msb = max17043_read_reg(client, MAX17040_VCELL_MSB);
	lsb = max17043_read_reg(client, MAX17040_VCELL_LSB);

	chip->vcell = ((msb << 4) + (lsb >> 4)) * 125;
}

int max17043_get_soc(void)
{
	u8 msb;
	u8 lsb;

	if(g_max17043_chip == NULL)
		return -1;
	msb = max17043_read_reg(g_max17043_chip->client, MAX17040_SOC_MSB);
	lsb = max17043_read_reg(g_max17043_chip->client, MAX17040_SOC_LSB);

	msb = msb >> 1;
	printk("[Max17043 gauge]Enter %s, msb-0x%02x, lsb-0x%02x.\n", __FUNCTION__, msb, lsb);

	if((g_max17043_chip->soc > 2) && (msb == 0))
	{
		g_max17043_chip->reset_gauge = 0;
		return g_max17043_chip->soc;
	}

	g_max17043_chip->soc = msb;
	
	if(g_max17043_chip->soc > 100)
		g_max17043_chip->soc = 100;
#ifdef MAX17403_FAKE_SOC_DEBUG
	if(g_max17043_chip->enable_fake_soc != 1)
		g_max17043_chip->soc = g_max17043_chip->enable_fake_soc;
#endif
	return g_max17043_chip->soc;
}
EXPORT_SYMBOL(max17043_get_soc);

static void max17043_get_version(struct i2c_client *client)
{
	u8 msb;
	u8 lsb;

	msb = max17043_read_reg(client, MAX17040_VER_MSB);
	lsb = max17043_read_reg(client, MAX17040_VER_LSB);

	dev_info(&client->dev, "MAX17043 Fuel-Gauge Ver %d%d\n", msb, lsb);
}

static void max17043_get_online(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	if (chip->pdata->battery_is_online)
		chip->online = chip->pdata->battery_is_online();
	else
		chip->online = 1;
}

#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
int max17043_get_temperature(void)
{
	int temp = 0;
	int i = 0;
	int ret = 0;
	struct twl4030_madc_request req;	

	if(g_max17043_chip == NULL)
		return -1;

	req.channels = (1 << TEMPERATURE_ON_MADC_CHANNEL);	
	req.do_avg = 1;	
	req.method = TWL4030_MADC_SW1;	
	req.active = 0;	
	req.func_cb = NULL;
	ret = twl4030_madc_conversion(&req);	
	if(ret != 0)
		goto err;
		
	temp = (u16)req.rbuf[TEMPERATURE_ON_MADC_CHANNEL];       
	/* VL is 3.3v, adc convert is 10bit (1024)*/
	temp = (temp*33000)/1024;
	
	while(max17043_temp_map[i].V_bat_ntc >= temp)
	{
		i++;
	}
	temp = max17043_temp_map[i].T_temp_ntc;	
	
	g_max17043_chip->temp = temp;
	/* Adjust temperature value to fit actual temparture of battery (Hard Code) */
	if(g_max17043_chip->temp < 25)
		g_max17043_chip->temp = g_max17043_chip->temp + 11;
	else
		g_max17043_chip->temp = g_max17043_chip->temp + 2;
err:
	return g_max17043_chip->temp;

}
EXPORT_SYMBOL(max17043_get_temperature);
#endif

#ifdef MAX17043_LOW_BATTERY_NOTIFY
static void max17043_set_low_battery(struct i2c_client *client, u8 value)
{
	/* Enable low battery alert and threshold is 10% or  4% */
	int config_value = max17043_read_reg(client, MAX17040_RCOMP_MSB);
	config_value = (config_value << 8) | (MAX_LOW_BATTERY_THRESHOLD-value);
	max17043_write_bytes_reg(MAX17040_RCOMP_MSB, config_value, client);
}

static int max17043_get_low_battery(struct i2c_client *client)
{
	return max17043_read_reg(client, MAX17040_RCOMP_LSB);
}

static void max17043_set_low_battery_alert(struct max17043_chip *chip)
{
	if(chip)
	{
		/* Clear alert by software & Config low battery capacity is 15% */	
		if(g_max17043_chip->soc > LOW_BATTERY_THRESHOLD)
		{
			if(chip->low_battery != LOW_BATTERY_THRESHOLD)
			{
				max17043_set_low_battery(chip->client, LOW_BATTERY_THRESHOLD);
				chip->low_battery = LOW_BATTERY_THRESHOLD;
			}
		}
		else if((g_max17043_chip->soc > CRITICAL_LOW_BATTERY_THRESHOLD) && (g_max17043_chip->soc <= LOW_BATTERY_THRESHOLD))
		{
			if(chip->low_battery != CRITICAL_LOW_BATTERY_THRESHOLD)
			{
				max17043_set_low_battery(chip->client, CRITICAL_LOW_BATTERY_THRESHOLD);
				chip->low_battery = CRITICAL_LOW_BATTERY_THRESHOLD;
			}
		}
		else if((g_max17043_chip->soc > SHUT_DOWN_BATTERY_THRESHOLD) && (g_max17043_chip->soc <= CRITICAL_LOW_BATTERY_THRESHOLD))
		{
			if(chip->low_battery != SHUT_DOWN_BATTERY_THRESHOLD)
			{
				max17043_set_low_battery(chip->client, SHUT_DOWN_BATTERY_THRESHOLD);
				chip->low_battery = SHUT_DOWN_BATTERY_THRESHOLD;
			}
		}
		else
		{
			max17043_set_low_battery(chip->client, 1);
			chip->low_battery = 1;
		}
	}
}

static void max17043_low_battery_work(struct work_struct *work)
{
	struct max17043_chip *chip;
	chip = container_of(work, struct max17043_chip, max17043_low_battery_delay_work.work);

	max17043_get_soc();
	printk("[max17043 gauge]Enter %s, low battery interrupt, set next threshold.\n", __FUNCTION__);
	max17043_set_low_battery_alert(chip);
}

static irqreturn_t max17043_low_power_irq(int irq, void *handle)
{
	struct max17043_chip *chip = (struct max17043_chip *)handle;
	schedule_delayed_work(&chip->max17043_low_battery_delay_work, 100);
	
	return IRQ_HANDLED;
}
#endif

static void max17043_work(struct work_struct *work)
{
	struct max17043_chip *chip;
	chip = container_of(work, struct max17043_chip, max17043_delay_work.work);

	max17043_get_vcell(chip->client);
	max17043_get_soc();
	max17043_get_online(chip->client);
#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
	max17043_get_temperature();
#endif
	/*
	* Report battery information on device booting after 20 sec, to wait charging status stable after device booting.
	* Report battery information on AC/USB cable mode pre 10sec
	* Report battery information on Battery mode pre 60sec
	*/
	if(chip->booting)
		schedule_delayed_work(&chip->max17043_delay_work, MAX17040_DELAY/3);
	else if(!chip->online)
		schedule_delayed_work(&chip->max17043_delay_work, MAX17040_DELAY/6);
	else
		schedule_delayed_work(&chip->max17043_delay_work, MAX17040_DELAY);
	
#ifdef MAX17043_INFO_PRESENTATION
	printk("********** Max17043 Gauge Report *************\n");
    printk("***        Voltage is %d              ***\n", chip->vcell);
	printk("***        Capacity is %d                 ***\n", chip->soc);
#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
	printk("***        Temperature is %d              ***\n", chip->temp);
#endif
	printk("***        Battery on-line is %d           ***\n", chip->online);
	printk("**********         END           *************\n");
#endif
	//&*&*&*AL1_20101207: Prevent gauge break down
	if((chip->soc == 0) && (chip->reset_gauge == 0))
	{
		printk("[max17043 gauge] Enter %s, gauge break down!! reset and redownload custom model.\n", __FUNCTION__);
		max17043_reset(chip->client);
		msleep(500);
		chip->reset_gauge = 1;
		max17043_write_custom_model(chip->client);
		msleep(500);
		return;
	}
	//&*&*&*AL2_20101207: Prevent gauge break down

#ifdef MAX17043_LOW_BATTERY_NOTIFY
	if(chip->online == 0)
		max17043_set_low_battery_alert(chip);
	printk("[max17043 gauge]max17043_get_low_battery, ATHD register is 0x%02x.\n", max17043_get_low_battery(chip->client));
#endif

#ifdef CONFIG_REGULATOR_MAX8903
	if(chip->soc == 100)
	{
		if(!max8903_charging_full())
			return;
	}
	else if((chip->soc <= 99) && chip->charging_full)
	{
		max8903_enable_charging();
		printk("[max17043 gauge] Enter %s, re-write custom model and qucik_start.\n", __FUNCTION__);
		max17043_write_custom_model(chip->client);
		msleep(500);
		max17043_quick_start(chip->client);
		msleep(1000);
	}
#endif
	/* 
	* Report battery infomation to android.
	* Note that : don't report status on booting, due to led driver and charging status not ready
	*/
	if(chip->battery.dev && chip->booting == 0)
		power_supply_changed(&chip->battery);
	else
	{
		chip->booting = 0;
#ifdef MAX17043_LOW_BATTERY_NOTIFY
		/* Config low battery capacity is 10% */	
		max17043_set_low_battery_alert(chip);
//&*&*&*HC1_20110428, enable low batt alert wakeup source
		// set Low Battery Alert as wakeup source
		set_irq_wake(OMAP_GPIO_IRQ(44), 1);
//&*&*&*HC2_20110428, enable low batt alert wakeup source		
#endif
	}
}

static enum power_supply_property max17043_battery_props[] = {
	/* Change status report on charging driver. It can response charging state promptly */
	//POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
	POWER_SUPPLY_PROP_TEMP,
#endif
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
};

static int __devinit max17043_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17043_chip *chip;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;
	
#ifdef MAX17043_LOW_BATTERY_NOTIFY
	if (!client->dev.platform_data) 
	{
		dev_err(&client->dev, "%s no platform data\n", __func__);
		return -ENODEV;
	}
#endif

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;
	chip->booting = 1;
	chip->reset_gauge = 1;
	INIT_DELAYED_WORK(&chip->max17043_delay_work, max17043_work);
	
	i2c_set_clientdata(client, chip);

	chip->battery.name		= "battery-max17043";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= max17043_get_property;
	chip->battery.properties	= max17043_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(max17043_battery_props);

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		kfree(chip);
		return ret;
	}

	max17043_get_version(client);
	
	ret = sysfs_create_group(&client->dev.kobj, &max17043_attr_group);
		if (ret)
			dev_err(&client->dev, "failed: create device attribute \n");

	/* Get soc on boot, confirm battery capacity for android booting... */ 
	g_max17043_chip = chip;
	g_max17043_chip->enable_fake_soc = 1;
	g_max17043_chip->soc = 0;
	
	max17043_get_soc();
#ifdef MAX17043_LOW_BATTERY_NOTIFY
	INIT_DELAYED_WORK(&chip->max17043_low_battery_delay_work, max17043_low_battery_work);
	/* Set IRQ for low_battery warnning */
	if(gpio_request(chip->pdata->irq_info[0].gpio, "max17043_alert") < 0)			
		dev_err(&client->dev, "Failed to request GPIO%d for max17043 alert pin\n", chip->pdata->irq_info[0].gpio);
	else
	{
		gpio_direction_input(chip->pdata->irq_info[0].gpio);
		ret = request_irq(chip->pdata->irq_info[0].irq, max17043_low_power_irq, chip->pdata->irq_info[0].irq_flags, "max17043_irq", chip);
		if (ret)			
			dev_err(&client->dev, "can't allocate irq %d\n", chip->pdata->irq_info[0].gpio);	
	}
#else
	if (gpio_request(chip->pdata->irq_info[0].gpio, "max17043_alert") < 0) 				
		printk(KERN_ERR "can't get general GPIO for max17043 \n");
	else
		gpio_direction_input(chip->pdata->irq_info[0].gpio);
#endif
	/* This schedule is updated battery capacity for charger. */
	schedule_delayed_work(&chip->max17043_delay_work, 2*HZ);

	/* Need 1s to ready after reset max17043 gauge*/
	//max17043_reset(client);
	//max17043_write_custom_model(client);
	
	return 0;
}

static int __devexit max17043_remove(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->max17043_delay_work);
	power_supply_unregister(&chip->battery);
#ifdef MAX17043_LOW_BATTERY_NOTIFY
	free_irq(chip->pdata->irq_info[0].irq, chip);
#endif
	sysfs_remove_group(&client->dev.kobj, &max17043_attr_group);
	kfree(chip);
	
	return 0;
}

#ifdef CONFIG_PM
static int max17043_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	cancel_delayed_work(&chip->max17043_delay_work);
	
	return 0;
}

static int max17043_resume(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	schedule_delayed_work(&chip->max17043_delay_work, 100/*MAX17040_DELAY*/);

	return 0;
}
#else
#define max17043_suspend NULL
#define max17043_resume NULL
#endif /* CONFIG_PM */

static const struct i2c_device_id max17043_id[] = {
	{ "max17043", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17043_id);

static struct i2c_driver max17043_i2c_driver = {
	.driver	= {
		.name	= "max17043",
	},
	.probe		= max17043_probe,
	.remove		= __devexit_p(max17043_remove),
	.suspend	= max17043_suspend,
	.resume		= max17043_resume,
	.id_table	= max17043_id,
};

static int __init max17043_init(void)
{
	return i2c_add_driver(&max17043_i2c_driver);
}
module_init(max17043_init);

static void __exit max17043_exit(void)
{
	i2c_del_driver(&max17043_i2c_driver);
}
module_exit(max17043_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("MAX17040 Fuel Gauge");
MODULE_LICENSE("GPL");
