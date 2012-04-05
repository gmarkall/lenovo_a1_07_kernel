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
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <linux/cdev.h>
#include <asm/system.h>


#include <linux/bq27541_battery.h>

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
#define CRITICAL_LOW_BATTERY_THRESHOLD 4
#define MAX_LOW_BATTERY_THRESHOLD 32
#endif

#define RESET_GAUGE_SOC_THRESHOLD 100

#define BATTERY_NAME "bq27541"
/*********************************/
u8 battery_capacity;
struct bq27541_chip {
	struct i2c_client		*client;
	struct delayed_work		bq27541_delay_work;
	struct power_supply		battery;
	struct bq27541_platform_data	*pdata;
	struct device *dev;
	/* State Of Connect */
	int online;
	/* battery voltage */
	int voltage;
	/* battery capacity */
	int soc;

	/* battery temperature*/
	int temp;
	
	/*battery current*/
	int curr;

	/* charging full */
	int charging_full;
	/* booting flag for check weather device is on booting */
	int booting;
	/* mamual reset gauge */
	int already_reset_gauge;
	/* enable fake soc as "non 1"*/
	int enable_fake_soc;

	int fw_version;
	int hw_version;
};

/* 
* This function be called by gauge driver for update battery capacity.
* Due to we disable charger when charging full that will not recharging again under terminal charging threshold.
*/
static struct bq27541_chip *g_bq27541_chip = NULL;

static int battery_major = 240 ;
dev_t devno ;
struct battery_dev_t
{
	struct cdev cdev; 
};
struct battery_dev_t battery_dev;

static void bq27541_get_fw_version(struct i2c_client *client);
static void bq27541_get_hw_version(struct i2c_client *client);



static int bq27541_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct bq27541_chip *chip = container_of(psy,
				struct bq27541_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->voltage;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
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
#else
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = chip->temp;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
#endif
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = chip->curr;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_FW_VERSION:
		val->intval = chip->fw_version;//bq27541_get_fw_version(chip->client);
		break;
	case POWER_SUPPLY_PROP_HW_VERSION:
		val->intval = chip->hw_version;//bq27541_get_hw_version(chip->client);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int bq27541_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int bq27541_read_reg(struct i2c_client *client, int reg)
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
static int bq27541_read_bytes_reg(u8 reg, int* wt_value, struct i2c_client *client)
{
	u8 mm1[] = { reg };
	struct i2c_msg msgs[2] = {
		{.addr = client->addr,.flags = 0,.buf = mm1,.len = 1},
		{.addr = client->addr,.flags = I2C_M_RD,.buf = wt_value,.len = 2}
	};
	int err;
	
	if (!client->adapter)
		return -ENODEV;

	if (i2c_transfer(client, msgs, 2) != 2)
		return -EIO;

	return 0;
}

/*
* The MAX17043/MAX17044 have six 16-bit registers:
* SOC, VCELL, MODE, VERSION, CONFIG, and COMMAND. Register reads and writes are only valid 
* if all 16 bits are transferred.
*/
static int bq27541_write_bytes_reg(u8 reg, int wt_value, struct i2c_client *client)
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

static ssize_t bq27541_fake_soc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq27541_chip *chip = i2c_get_clientdata(client);
	
	unsigned long val = simple_strtoul(buf, NULL, 10);
	if (val < 0 || val > 100)
		return -EINVAL;	

	return count;
}
static DEVICE_ATTR(fake_soc, S_IWUSR | S_IRWXUGO, NULL, bq27541_fake_soc_store);

static ssize_t bq27541_reset_store(struct device *dev,
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

	return count;
}
static DEVICE_ATTR(reset, S_IWUSR | S_IRWXUGO, NULL, bq27541_reset_store);

static ssize_t bq27541_firewire_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);		
	unsigned long val = simple_strtoul(buf, NULL, 10);
	
//	if (val < 0 || val > 1)
//		return -EINVAL;	

	return count;
}
static DEVICE_ATTR(model_data, S_IWUSR | S_IRWXUGO, NULL ,bq27541_firewire_data_store);

static struct attribute *bq27541_attributes[] = {
	&dev_attr_model_data.attr,
	&dev_attr_reset.attr,
	&dev_attr_fake_soc.attr,
	NULL
};
static const struct attribute_group bq27541_attr_group = {
	.attrs = bq27541_attributes,
};

void bq27541_set_charging_full_state(int state)
{
	g_bq27541_chip->charging_full = state;
}
EXPORT_SYMBOL(bq27541_set_charging_full_state);

static void bq27541_reset(struct i2c_client *client)
{
//	bq27541_write_reg(client, MAX17040_CMD_MSB, 0x54);
//	bq27541_write_reg(client, MAX17040_CMD_LSB, 0x00);
	printk("[bq27541 gauge]Enter %s, gauge reset finish.\n", __FUNCTION__);
}

static void bq27541_get_voltage(struct i2c_client *client)
{
	struct bq27541_chip *chip = i2c_get_clientdata(client);
	u8 msb = 0x00;
	u8 lsb = 0x00;

	msb = bq27541_read_reg(client, BQ27541_REG_VOLT_LOW);
	lsb = bq27541_read_reg(client, BQ27541_REG_VOLT_HIGH);

	chip->voltage = (lsb * 0x100 + msb) * 1000;
//	printk("================bq27541 get voltage value,%s,msb= 0x%x,lsb= 0x%x\n",__func__,msb,lsb);
}

static void bq27541_get_current(struct i2c_client *client)
{
	struct bq27541_chip *chip = i2c_get_clientdata(client);
	u8 msb = 0x00;
	u8 lsb = 0x00;
	u16 tmp = 0x00;

	msb = bq27541_read_reg(client, BQ27541_REG_AI_LOW);
	lsb = bq27541_read_reg(client, BQ27541_REG_AI_HIGH);

	tmp = lsb * 0x100 +msb;
	chip->curr = (s16) tmp;
//	printk("================bq27541 get current value,%s,msb= 0x%x,lsb= 0x%x\n",__func__,msb,lsb);
}

int bq27541_get_capacity(void)
{
	u8 msb;
	u8 lsb;
	u8 tmp;

	if(g_bq27541_chip == NULL)
		return -1;
	
	msb = bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_RSOC_LOW);
	lsb = bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_RSOC_HIGH);

#if 0
printk("================%s,msb= 0x%x,lsb= 0x%x\n",__func__,msb,lsb);

printk("================%s,BQ27541_REG_FCC_LOW= 0x%x\n",__func__,
			bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_FCC_LOW));
printk("================%s,BQ27541_REG_FCC_HIGH= 0x%x\n",__func__,
			bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_FCC_HIGH));
printk("================%s,BQ27541_REG_FAC_LOW= 0x%x\n",__func__,
			bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_FAC_LOW));
printk("================%s,BQ27541_REG_FAC_HIGH= 0x%x\n",__func__,
			bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_FAC_HIGH));
printk("================%s,BQ27541_REG_RM_LOW= 0x%x\n",__func__,
			bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_RM_LOW));
printk("================%s,BQ27541_REG_RM_HIGH= 0x%x\n",__func__,
			bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_RM_HIGH));
#endif
			
#if defined(CONFIG_CL1_3G_BOARD)
			tmp = lsb * 0x100 + msb;
			if(tmp <= 15)
				tmp = tmp/3;
			else
				tmp = (950*(tmp-15)/85 + 5)/10 + 5;
			g_bq27541_chip->soc = tmp;
#else
	g_bq27541_chip->soc = lsb * 0x100 + msb;
#endif			
	
	return g_bq27541_chip->soc;
}
EXPORT_SYMBOL(bq27541_get_capacity);

static void bq27541_get_version(struct i2c_client *client)
{
	u8 msb;
	u8 lsb;

	msb = bq27541_read_reg(client, BQ27541_REG_SI_LOW);
	lsb = bq27541_read_reg(client, BQ27541_REG_SI_HIGH);
	
//	printk("================%s,msb= 0x%x,lsb= 0x%x\n",__func__,msb,lsb);

	dev_info(&client->dev, "bq27541 Fuel-Gauge Ver %d%d\n", msb, lsb);
}

static void bq27541_get_online(struct i2c_client *client)
{
	struct bq27541_chip *chip = i2c_get_clientdata(client);

	if (chip->pdata->battery_is_online)
		chip->online = chip->pdata->battery_is_online();
	else
		chip->online = 1;
}

int bq27541_get_temperature(void)
{
	u8 msb;
	u8 lsb;
	int temp;

	msb = bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_TEMP_LOW);
	lsb = bq27541_read_reg(g_bq27541_chip->client, BQ27541_REG_TEMP_HIGH);
	
//		printk("================%s,msb= 0x%x,lsb= 0x%x\n",__func__,msb,lsb);
	temp = (lsb *0x100 + msb)/10 - 273;
	g_bq27541_chip->temp = temp * 10;
	
	return g_bq27541_chip->temp;

}
EXPORT_SYMBOL(bq27541_get_temperature);

static old_capacity = 0;
static void bq27541_work(struct work_struct *work)
{
	struct bq27541_chip *chip;
	int capacity = 0;
	chip = container_of(work, struct bq27541_chip, bq27541_delay_work.work);

	bq27541_get_voltage(chip->client);
	bq27541_get_current(chip->client);
	bq27541_get_online(chip->client);


	bq27541_get_temperature();
	
	capacity = bq27541_get_capacity();
	
	if(old_capacity != capacity)
	{
		kobject_uevent(&(chip->dev->kobj), KOBJ_CHANGE);
		power_supply_changed(&chip->battery);
		old_capacity = capacity;
		battery_capacity = capacity;
	}

	/*
	* Report battery information on device booting after 20 sec, to wait charging status stable after device booting.
	* Report battery information on AC/USB cable mode pre 10sec
	* Report battery information on Battery mode pre 60sec
	*/
	if(chip->booting)
		schedule_delayed_work(&chip->bq27541_delay_work, MAX17040_DELAY/3);
	else if(!chip->online)
		schedule_delayed_work(&chip->bq27541_delay_work, MAX17040_DELAY/6);
	else
		schedule_delayed_work(&chip->bq27541_delay_work, MAX17040_DELAY);
	
#if 0
	printk("********** bq27541 Gauge Report *************\n");
    printk("***        Voltage is %d              ***\n", chip->voltage);
	printk("***        Capacity is %d                 ***\n", chip->soc);

	printk("***        Temperature is %d              ***\n", chip->temp);

	printk("***        Battery on-line is %d           ***\n", chip->online);
	printk("**********         END           *************\n");
#endif
#if 0
	//&*&*&*AL1_20101207: Prevent gauge break down
	if(chip->soc == 0)
	{
		printk("[bq27541 gauge] Enter %s, gauge break down!! reset and redownload custom model.\n", __FUNCTION__);
		bq27541_reset(chip->client);
		return;
	}
	//&*&*&*AL2_20101207: Prevent gauge break down
	
	/* 
	* Due to we disable charger when charging full that will not recharging again under terminal charging threshold.
	* If return valus is 1, represent charging full. It must modify soc to 100%
	*/
	if(!chip->already_reset_gauge && chip->charging_full)
	{
		if(chip->soc < RESET_GAUGE_SOC_THRESHOLD)
		{
			printk("[bq27541 gauge] Enter %s, reset gauge and write custom model \n", __FUNCTION__);
			bq27541_reset(chip->client);
			//bq27541_write_custom_model(chip->client);
		}
		chip->already_reset_gauge = 1;
	}

	if(chip->battery.dev && chip->booting == 0)
		power_supply_changed(&chip->battery);
	else
	{
		/* Config low battery capacity is 10% */	
//		bq27541_set_low_battery_alert(chip);
		chip->booting = 0;
	}
#endif
}

static enum power_supply_property bq27541_battery_props[] = {
	/* Change status report on charging driver. It can response charging state promptly */
	//POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,

	POWER_SUPPLY_PROP_TEMP,

	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_FW_VERSION,
	POWER_SUPPLY_PROP_HW_VERSION,
};

static void bq27541_get_fw_version(struct i2c_client *client)
{
	struct bq27541_chip *chip = i2c_get_clientdata(client);
	int value = 0x200;
	u8 msb;
	u8 lsb;
	unsigned int tmp;
	
	bq27541_write_bytes_reg(0x0, value, client);
	mdelay(300);
	msb = bq27541_read_reg(client, 0x0);
	lsb = bq27541_read_reg(client, 0x1);

	tmp = lsb * 0x100 +msb;
	#if 0
	printk("\n");
	printk("\n");
	printk("The FW_VERSION is %8x\n", tmp);
	printk("\n");
	printk("\n");
	#endif
	chip->fw_version = (unsigned int) tmp;

}
static void bq27541_get_hw_version(struct i2c_client *client)
{
	struct bq27541_chip *chip = i2c_get_clientdata(client);
	int value = 0x300;
	u8 msb;
	u8 lsb;
	unsigned int tmp;
	
	bq27541_write_bytes_reg(0x0, value, client);
	mdelay(300);
	msb = bq27541_read_reg(client, 0x0);
	lsb = bq27541_read_reg(client, 0x1);

	tmp = lsb * 0x100 +msb;
	#if 0
	printk("\n");
	printk("\n");
	printk("The HW_VERSION is %8x\n", tmp);
	printk("\n");
	printk("\n");
	#endif
	chip->hw_version = (unsigned int) tmp;

}


static int __devinit bq27541_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct bq27541_chip *chip;
	int ret;
	int res;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;


	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->dev = &client->dev;
	chip->pdata = client->dev.platform_data;
	chip->booting = 1;
	INIT_DELAYED_WORK(&chip->bq27541_delay_work, bq27541_work);
	
	i2c_set_clientdata(client, chip);
	
	bq27541_get_fw_version(chip->client);
	bq27541_get_hw_version(chip->client);

	chip->battery.name		= "battery-bq27541";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= bq27541_get_property;
	chip->battery.properties	= bq27541_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(bq27541_battery_props);



	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		kfree(chip);
		return ret;
	}
	
	bq27541_get_version(client);
	
//	ret = sysfs_create_group(&client->dev.kobj, &bq27541_attr_group);
//		if (ret)
//			dev_err(&client->dev, "failed: create device attribute \n");

	/* Get soc on boot, confirm battery capacity for android booting... */ 
	g_bq27541_chip = chip;
	g_bq27541_chip->enable_fake_soc = 1;
	
	bq27541_get_capacity();

	schedule_delayed_work(&chip->bq27541_delay_work, 2*HZ);

	return 0;

}

static int __devexit bq27541_remove(struct i2c_client *client)
{
	struct bq27541_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->bq27541_delay_work);
	power_supply_unregister(&chip->battery);
//	sysfs_remove_group(&client->dev.kobj, &bq27541_attr_group);
	kfree(chip);
	
	return 0;
}

#ifdef CONFIG_PM
static int bq27541_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct bq27541_chip *chip = i2c_get_clientdata(client);
	cancel_delayed_work(&chip->bq27541_delay_work);
	
	return 0;
}
static int bq27541_resume(struct i2c_client *client)
{
	struct bq27541_chip *chip = i2c_get_clientdata(client);

	if(!chip->online)
		schedule_delayed_work(&chip->bq27541_delay_work, MAX17040_DELAY/6);
	else
		schedule_delayed_work(&chip->bq27541_delay_work, MAX17040_DELAY);
	return 0;
}

#else
	#define bq27541_suspend NULL
	#define bq27541_resume NULL
#endif /* CONFIG_PM */

static const struct i2c_device_id bq27541_id[] = {
	{ "bq27541", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bq27541_id);

static struct i2c_driver bq27541_i2c_driver = {
	.driver	= {
		.name	= "bq27541",
	},
	.probe		= bq27541_probe,
	.remove		= __devexit_p(bq27541_remove),
	.suspend	= bq27541_suspend,
	.resume		= bq27541_resume,
	.id_table	= bq27541_id,
};

static int __init bq27541_init(void)
{
	int ret;
	
	ret = i2c_add_driver(&bq27541_i2c_driver);
	if (ret)
		printk(KERN_ERR "Unable to register BQ27541 driver\n");

	return ret;
//	return i2c_add_driver(&bq27541_i2c_driver);
}
module_init(bq27541_init);

static void __exit bq27541_exit(void)
{
	i2c_del_driver(&bq27541_i2c_driver);
}
module_exit(bq27541_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("MAX17040 Fuel Gauge");
MODULE_LICENSE("GPL");
