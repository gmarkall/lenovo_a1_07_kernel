/*
 * Support for Maxim (Max8903) Dual Input (USB/AC Adpater)
 * 1-Cell Li-Ion Charger connected via GPIOs.
 *
 * Copyright (c) 2010 Aimar Liu@Foxconn
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/max8903.h>
#include <linux/wakelock.h>
#include <linux/power_supply.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
//&*&*&*BC1_10726: fix issue that cpu cannot run 300Mhz issue
#include <plat/omap-pm.h>
//&*&*&*BC1_10726: fix issue that cpu cannot run 300Mhz issue

#define MAX8903_FULL_FUNCTION_SUPPORT

/* Power regulator thresholds. */
#define CURRENT_TRH_1500MA 		(1500000)
#define CURRENT_TRH_500MA 		(500000)

/* Power regulator modes. */
#define CURRENT_MAX_500MA 		(0x00)
#define CURRENT_MAX_1500MA 		(0x01)
#define CURRENT_MAX_SUSPEND		(0x03)

#define PSET_MASK	(1 << 0)

#define CHARGING_FULL_THRESHOLD 98
#define BATTERY_HOT_THRESHOLD 450
#define BATTERY_COLD_THRESHOLD -50
#define BATTERY_REMOVE_THRESHOLD -310

static enum power_supply_property max8903_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};
static enum power_supply_property max8903_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};
static enum power_supply_property max8903_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CAPACITY,
};

enum cable_type {
	MAX8903_BATTERY_SUPPLY = 0,
	MAX8903_USB_CABLE_SUPPLY,
	MAX8903_ID_CABLE_SUPPLY,
	MAX8903_AC_CABLE_SUPPLY,
};

enum charging_status {
	MAX8903_CHARGING_STATUS_DISCONNECT,
	MAX8903_CHARGING_STATUS_CONNECT,
	MAX8903_CHARGING_STATUS_FULL,
	MAX8903_CHARGING_STATUS_FAULT_TEMP,
	MAX8903_CHARGING_STATUS_FAULT_TIMEOUT,
	MAX8903_CHARGING_STATUS_FAULT_BATTERY_REMOVE,
	MAX8903_CHARGING_STATUS_CONDITION,
};

/* Define for compiler*/
struct max8903;

struct max8903_irq {
	void (*handler) (struct max8903 *, int, void *);
	void *data;
	int chip_irq;
	int state;
};

struct max8903_power {
	struct power_supply battery;
	struct power_supply usb;
	struct power_supply ac;
};

struct max8903 {
	struct device 		*dev;
	struct input_dev    *intpu_dev;
	struct max8903_mach_info *minfo;
	struct max8903_power power_uevent;
	struct regulator_dev *max8903_regulator_dev;
	struct regulator *max8903_regulator;
	struct delayed_work irq_delay_work_charge_state;
//#ifdef MAX8903_FULL_FUNCTION_SUPPORT
	struct delayed_work irq_work_fault_state;
//#endif
	struct delayed_work irq_delay_work_dcok_state;
	//&*&*&*AL1_20101116: Add wake lock for to let device can't enter suspend on charging full with a plug-in ac/usb cable
	struct wake_lock ac_usb_charging_full_wake_lock;
	//&*&*&*AL2_20101116: Add wake lock for to let device can't enter suspend on charging full with a plug-in ac/usb cable
	struct mutex irq_mutex; /* IRQ table mutex */
	spinlock_t spinlock; /* IRQ data lock */
	struct max8903_irq irq[MAX8903_IRQ_NUM];
	/* Variable for LED and charger switch */
	int supply;
	int charging_state;
	int reboot;
	struct delayed_work delay_work_reboot_state;
	int battery_soc;
	int full_recharging_en;
	int suspend;
//#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
	int temperature;
	int temp_fail;
#ifdef CONFIG_REGULATOR_MAX8903_CABLE_TYPE_DETECTION
	int has_usb_configuration;
#endif
//	struct delayed_work	led_schedule_green;
	struct work_struct led_schedule_green;
//#endif
};

#ifdef CONFIG_TWL4030_USB
extern int twl4030_cable_type_detection(void);
#endif

//#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
//extern int max17043_get_temperature(void);
extern int bq27541_get_temperature(void);
//#endif

#ifdef CONFIG_BATTERY_BQ27541
extern int bq27541_get_capacity(void);
extern void bq27541_set_charging_full_state(int state);
#else
extern int twl4030battery_capacity(void);
#endif

#ifdef CONFIG_LEDS_EP10
extern void ep10_set_led_gpio(int value);
#endif

#ifdef CONFIG_EP10_LED_BLINK
extern struct timer_list ledtrig_charging_timer;
extern int charging_activity;
#endif

#if 0
//&*&*&*AL1_20101119, Modify to use the WAKEUP REASON(especially charging full) for resuming behavior	
/* Global flags to record the wakeup reason */
extern unsigned int 	g_wakeup_stat;
extern unsigned int 	g_eint0pend;
//&*&*&*AL2_20101119, Modify to use the WAKEUP REASON for resuming behavior
#endif 

#ifdef CONFIG_USB_CLOCK_SUSPEND	
//&*&*&*BC1_101027: add cable type flag
int g_usb_ac = 0;
//&*&*&*BC2_101027: add cable type flag
#endif

struct timer_list led_change_green;
/* 
* This function be called by gauge driver for update battery capacity.
* Due to we disable charger when charging full that will not recharging again under terminal charging threshold.
*/
static struct max8903 *g_max8903 = NULL;
//&*&*&*BC1_10726: fix issue that cpu cannot run 300Mhz issue
void adjust_cpu_frequency(struct max8903 * max8903)
{
	struct max8903_mach_info * pdata = max8903->minfo;
	int cable_state = gpio_get_value(pdata->irq_info[MAX8903_PIN_DCOKn].gpio);

	if (!cable_state)
		omap_pm_set_min_mpu_freq(max8903->dev, 800000000);
	else
		omap_pm_set_min_mpu_freq(max8903->dev, -1);

	printk("%s\n", __func__);
}
//&*&*&*BC2_10726: fix issue that cpu cannot run 300Mhz issue

int max8903_charging_full(void)
{
	if(g_max8903 == NULL || g_max8903->charging_state == MAX8903_CHARGING_STATUS_FULL)
		return 1;
	g_max8903->full_recharging_en = 1;
	g_max8903->charging_state = MAX8903_CHARGING_STATUS_FULL;

	if(g_max8903->power_uevent.battery.dev)
		power_supply_changed(&g_max8903->power_uevent.battery);
	
	return 0;
}
EXPORT_SYMBOL(max8903_charging_full);

void max8903_enable_charging(void)
{
	if(g_max8903 == NULL)
		return;
	if(g_max8903->max8903_regulator == NULL || IS_ERR(g_max8903->max8903_regulator)) {
		g_max8903->max8903_regulator = regulator_get(g_max8903->dev, "max8903");
	}
	if(!regulator_is_enabled(g_max8903->max8903_regulator))
	{
        regulator_enable(g_max8903->max8903_regulator);
		printk("[max8903 charger]Re-charging : Need to keep 100 charging full.\n");
	}
	return;
}
EXPORT_SYMBOL(max8903_enable_charging);

#ifdef CONFIG_REGULATOR_MAX8903_CABLE_TYPE_DETECTION
void max8903_has_usb_configuration(void)
{
	if (g_max8903 == NULL)
		return;
	printk("[Composite.c]Enter %s, usb's configuration be set\n", 
			__func__);
	g_max8903->has_usb_configuration = 1;
	return;
}
EXPORT_SYMBOL(max8903_has_usb_configuration);
#endif

static int max8903_battery_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct max8903 *max8903 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			if (max8903->charging_state >= MAX8903_CHARGING_STATUS_FAULT_TIMEOUT) {
				val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
				return ret;
			}
			if(max8903->charging_state == MAX8903_CHARGING_STATUS_FULL)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else if(max8903->charging_state == MAX8903_CHARGING_STATUS_CONNECT)
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else if(max8903->charging_state == MAX8903_CHARGING_STATUS_DISCONNECT)
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			else if(max8903->charging_state == MAX8903_CHARGING_STATUS_FAULT_TEMP)
				val->intval = POWER_SUPPLY_HEALTH_COLD;
			break;
		/* Must return soc information, because android battery service will get capacity on all battery supply */
		case POWER_SUPPLY_PROP_CAPACITY:
				val->intval = bq27541_get_capacity(); //max8903->battery_soc;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static void led_schedule_handler(struct work_struct *work)
{
//		struct max8903 *max8903 = container_of(work, struct max8903, led_schedule_green.work);
	struct max8903 *max8903 = container_of(work, struct max8903, led_schedule_green);
		struct max8903_power *power_uevent = &max8903->power_uevent;
		
		if(bq27541_get_capacity() >= 95) 
		{
						max8903->charging_state = MAX8903_CHARGING_STATUS_FULL;
						power_supply_changed(&power_uevent->battery);
						del_timer_sync(&led_change_green);
		}
		else
			mod_timer(&led_change_green, jiffies + msecs_to_jiffies(2000));
}

static void led_change_green_handler(unsigned long _data)
{

	struct max8903 *max8903 = (struct max8903 *)_data;
	
 		schedule_work(&max8903->led_schedule_green);
}

static int max8903_ac_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct max8903 *max8903 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			if(max8903->supply == MAX8903_AC_CABLE_SUPPLY)
          		val->intval = 1;
         	else
         		val->intval = 0; 
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static int max8903_usb_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct max8903 *max8903 = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			if(max8903->supply == MAX8903_USB_CABLE_SUPPLY)
       		 	val->intval = 1;
       		else
       			val->intval = 0;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static int max8903_set_current_limit(struct regulator_dev *rdev,
					int min_uA, int max_uA)
{
	struct max8903_mach_info *pdata = rdev_get_drvdata(rdev);
	if (min_uA == CURRENT_TRH_1500MA) {
		pdata->gpio_dcm_state = !!(CURRENT_MAX_1500MA & PSET_MASK);
	} else if (min_uA == CURRENT_TRH_500MA) {
		pdata->gpio_dcm_state = !!(CURRENT_MAX_500MA & PSET_MASK);
//		pdata->gpio_iusb_state = !!(CURRENT_MAX_500MA & PSET_MASK);
	}
	else{
		pdata->gpio_dcm_state = !!(CURRENT_MAX_500MA & PSET_MASK);
	//	pdata->gpio_iusb_state = !(CURRENT_MAX_500MA & PSET_MASK);
	}
	gpio_set_value(pdata->gpio_dcm, pdata->gpio_dcm_state);
//	gpio_set_value(pdata->gpio_iusb, pdata->gpio_iusb_state);
	
	return 0;
}

static int max8903_get_current_limit(struct regulator_dev *rdev)
{
	struct max8903_mach_info *pdata = rdev_get_drvdata(rdev);
	u8 curr_lim;
	int ret;

	curr_lim = pdata->gpio_dcm_state;

	switch (curr_lim) {
	case CURRENT_MAX_1500MA:
		ret = 1500000;
		break;
	case CURRENT_MAX_500MA:
		ret = 500000;
		break;
	case CURRENT_MAX_SUSPEND:
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

static int max8903_enable(struct regulator_dev *rdev)
{
	struct max8903_mach_info *pdata = rdev_get_drvdata(rdev);

	dev_dbg(rdev_get_dev(rdev), "[max8903 charger] enabling charger\n");
	pdata->gpio_cen_state = 0;
	gpio_set_value(pdata->gpio_cen, 0);
	return 0;
}

static int max8903_disable(struct regulator_dev *rdev)
{
	struct max8903_mach_info *pdata = rdev_get_drvdata(rdev);

	dev_dbg(rdev_get_dev(rdev), "[max8903 charger] disabling charger\n");
	pdata->gpio_cen_state = 1;
	gpio_set_value(pdata->gpio_cen, 1);
	return 0;
}

static int max8903_is_enabled(struct regulator_dev *rdev)
{
	struct max8903_mach_info *pdata = rdev_get_drvdata(rdev);

	return !(pdata->gpio_cen_state);
}

#ifdef CONFIG_TWL4030_USB
static int max8903_get_supplies(struct max8903 *max8903)
{
	int supplies = 0;
	supplies = twl4030_cable_type_detection();
	max8903->supply = supplies;
	return supplies;
}
#endif

static void max8903_charger_control(struct max8903 *max8903)
{
#ifdef CONFIG_BATTERY_BQ27541
	max8903->battery_soc = bq27541_get_capacity();
#else
	max8903->battery_soc = twl4030battery_capacity();
#endif

	if(max8903->max8903_regulator == NULL || IS_ERR(max8903->max8903_regulator)) {
		max8903->max8903_regulator = regulator_get(max8903->dev, "max8903");
	}

	if(IS_ERR(max8903->max8903_regulator))
	{
		printk("[max8903 charger]Fault!! can't get max8903 regulator.\n");
		return;
	}
//#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
	/* Reflash battery temperator status */
	max8903->temperature = bq27541_get_temperature();
	printk("[max8903 charger] Enter %s, battery temperature is %d\n", __FUNCTION__, max8903->temperature);
	
	if(max8903->temperature <= BATTERY_REMOVE_THRESHOLD)
	{
		printk("[max8903 charger] Enter %s, temperature is %d (No Battery Exist)....\n", __FUNCTION__, max8903->temperature);
		max8903->charging_state = MAX8903_CHARGING_STATUS_FAULT_BATTERY_REMOVE;
		
		/*  Don't disable charging with no battery. VBAT will haven't enough power to driver EPD */
		//&*&*&*AL1_20101116: enable charger and disable irq for battery remove
		disable_irq_nosync(max8903->irq[MAX8903_PIN_CHARHEn].chip_irq);
				
		/* Send power key down for android to shut down device */
		if(max8903->intpu_dev)
		{
			printk("[max8903 charger] Send KEY_END to Android!!....\n");
			input_report_key(max8903->intpu_dev, KEY_END, 1);
		}
		return;
	}
	else if (max8903->temperature >= BATTERY_HOT_THRESHOLD || max8903->temperature <= BATTERY_COLD_THRESHOLD)
	{
		if(max8903->charging_state != MAX8903_CHARGING_STATUS_DISCONNECT)
		{
			max8903->charging_state = MAX8903_CHARGING_STATUS_FAULT_TEMP;
			printk("[max8903 charger] Enter %s, Battery temperature Hot/Cold....\n", __FUNCTION__);
			if(regulator_is_enabled(max8903->max8903_regulator))
			{
            	regulator_disable(max8903->max8903_regulator);
				printk("[max8903 charger] Enter %s, disable Charger ( Temperature )....\n", __FUNCTION__);
			}
			max8903->temp_fail = 1;
			/* Don't Keep battery full status after re-charging. Due to may used so long on temperator fail condition */
			if(max8903->full_recharging_en)
				max8903->full_recharging_en = 0;
			return;
		}
	}
//#endif
	
    switch(max8903->charging_state)
	{
		case MAX8903_CHARGING_STATUS_DISCONNECT:
			/* Don't disable charger when remove cable, because charge status pin will becaome invalid */
			max8903->full_recharging_en = 0;
//#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
			max8903->temp_fail = 0;
//#endif
			regulator_set_current_limit(max8903->max8903_regulator, CURRENT_TRH_500MA, CURRENT_TRH_500MA);
			//&*&*&*AL1_20101116: Add wake lock for to let device can't enter suspend on charging full with a plug-in ac/usb cable
			if(wake_lock_active(&max8903->ac_usb_charging_full_wake_lock))
					wake_unlock(&max8903->ac_usb_charging_full_wake_lock);
			//&*&*&*AL2_20101116: Add wake lock for to let device can't enter suspend on charging full with a plug-in ac/usb cable
			printk("[max8903 charger] Enter %s, Discharging....\n", __FUNCTION__);
			break;

		case MAX8903_CHARGING_STATUS_CONNECT:
			if(!regulator_is_enabled(max8903->max8903_regulator))
			{
            	regulator_enable(max8903->max8903_regulator);
			}
			if(max8903->supply == MAX8903_AC_CABLE_SUPPLY)
			{
				regulator_set_current_limit(max8903->max8903_regulator, CURRENT_TRH_1500MA, CURRENT_TRH_1500MA);
				printk("[max8903 charger] Enter %s, Charging....%s\n", __FUNCTION__, "AC adapter");
			}
			else
			{
				regulator_set_current_limit(max8903->max8903_regulator, CURRENT_TRH_500MA, CURRENT_TRH_500MA);
				printk("[max8903 charger] Enter %s, Charging....%s\n", __FUNCTION__, "USB Cable");
			}
			/* Keep battery full status after re-charging */
			if(max8903->full_recharging_en)
				max8903->charging_state = MAX8903_CHARGING_STATUS_FULL;
			break;
				
		case MAX8903_CHARGING_STATUS_CONDITION:
//#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
			/* Do condition detect - Temperator Fail before */
			if(max8903->temp_fail)
			{
				if(!regulator_is_enabled(max8903->max8903_regulator))
				{
            		regulator_enable(max8903->max8903_regulator);
					printk("[max8903 charger]Re-charging : Due to temperator is suitabled.\n");
				}
				max8903->temp_fail = 0;
			}
			/* Do condition detect - Charging Complete */
			else
//#endif
			{
				max8903->charging_state = MAX8903_CHARGING_STATUS_FULL;

				if(max8903->battery_soc >= CHARGING_FULL_THRESHOLD)
				{
					/* 
					* Disable charger when charging full, 
					* because charger is always invariably switch charging/dis-charging state when charger enable.
					* Note: many devices consume current from battery directly but charging current is very small in that time. 
					*/
					if(regulator_is_enabled(max8903->max8903_regulator))
					{
 //           			regulator_disable(max8903->max8903_regulator);
						printk("[max8903 charger] Enter %s, disable charger (FULL)....\n", __FUNCTION__);
					}
					//&*&*&*AL1_20101116: Add wake lock for to let device can't enter suspend on charging full with a plug-in ac/usb cable
					if(!wake_lock_active(&max8903->ac_usb_charging_full_wake_lock))
						wake_lock(&max8903->ac_usb_charging_full_wake_lock);
					//&*&*&*AL2_20101116: Add wake lock for to let device can't enter suspend on charging full with a plug-in ac/usb cable

					max8903->full_recharging_en = 1;
						
					printk("[max8903 charger] Enter %s, Charging FULL....\n", __FUNCTION__);
				}
				/* Do condition detect - Charging Complete but not reach battery full threshold or re-charging condition */
				else
				{
					if(!regulator_is_enabled(max8903->max8903_regulator))
					{
            			regulator_enable(max8903->max8903_regulator);
						printk("[max8903 charger]Re-enable-charging : Due to capacity lower than 98.\n");
					}
					if(max8903->battery_soc >= CHARGING_FULL_THRESHOLD - 8)
					{
						max8903->full_recharging_en = 1;
						printk("[max8903 charger] Enter %s, Still Judgment Charging FULL ....\n", __FUNCTION__);
					}
					else
					//&*&*&*HC1_20101221: Modify to prevent the wrong state transition during cable in/out event change
					{	
						printk("[max8903 charger] Enter %s, wrong state on charging complete ....\n", __FUNCTION__);
						max8903->charging_state = MAX8903_CHARGING_STATUS_DISCONNECT;
					}
					//&*&*&*HC2_20101221: Modify to prevent the wrong state transition during cable in/out event change						
				}
			}
			break;
			
		default:
			break;
	}
	
	#ifdef CONFIG_BATTERY_BQ27541
		bq27541_set_charging_full_state(max8903->full_recharging_en);
	#endif
}

static void max8903_charger_handler(struct max8903 *max8903, int irq, void *data)
{
	struct max8903_power *power_uevent = &max8903->power_uevent;
	struct max8903_mach_info *pdata_info = max8903->minfo;
	int charge_en_state = 0;
	
	printk("======%s,irq = %d\n",__func__,irq);
	
	switch (irq) 
	{
		case MAX8903_PIN_CHARHEn:
			/* 
			*  If device is on boot,  suggest to work_queue charging status again to 
			*  detect ac charging and led notification after usb driver initialization and led driver ready.
			*/
			if(max8903->reboot == 1)
			{
				schedule_delayed_work(&max8903->delay_work_reboot_state, 10*HZ);
				return;
			}
			/*
			*
			* TSET pin is NM, termination thresdhold is max * 10%
			* charging full with USB 50mA
			* charging full with AC  100mA
			* 
			* THM pin is 10k:zero Ohm, thermination funuction is disable on charger
			*
			*/
#ifdef CONFIG_REGULATOR_MAX8903_CABLE_TYPE_DETECTION
	//		if (max8903->supply != MAX8903_BATTERY_SUPPLY) {
			if (max8903->has_usb_configuration == 0 &&
			max8903->supply == MAX8903_USB_CABLE_SUPPLY)
				max8903->supply = MAX8903_AC_CABLE_SUPPLY;
			else if (max8903->has_usb_configuration == 1)
				max8903->supply = MAX8903_USB_CABLE_SUPPLY;
	//		}
#endif

			if(max8903->irq[MAX8903_PIN_CHARHEn].state == 0)
			{
				max8903->charging_state = MAX8903_CHARGING_STATUS_CONNECT;
				if(max8903->supply == MAX8903_BATTERY_SUPPLY)
				{
					printk("[max8903 charger] charge state is error!! warning...state is no cable.\n");
					return;
				}
				else
					max8903_charger_control(max8903);
			}
			else
			{
				if(max8903->supply != MAX8903_BATTERY_SUPPLY)
				{
					/* Charging full, fault or hot/cold */
					max8903->charging_state = MAX8903_CHARGING_STATUS_CONDITION;
					max8903_charger_control(max8903);
				}
				else
				{
				    /* Remove cable */
					max8903->charging_state = MAX8903_CHARGING_STATUS_DISCONNECT;
					max8903_charger_control(max8903);
				}
			}
			//&*&*&*AL1_20101123, To mark this notification, due to it's unnecessary 	
			/*if(power_uevent->usb.dev)
				power_supply_changed(&power_uevent->usb);
			if(power_uevent->ac.dev)
				power_supply_changed(&power_uevent->ac);*/
			//&*&*&*AL2_20101123, To mark this notification, due to it's unnecessary 	
			if((power_uevent->battery.dev) && (bq27541_get_capacity() < 95))
				power_supply_changed(&power_uevent->battery);
			
			break;
			
		case MAX8903_PIN_FLAUTn:
			/*
			* charging fault with pre  30 min
			* charging fault with fast  660 min
			*/
			/* re-toggle CEN pin */
			if(regulator_is_enabled(max8903->max8903_regulator))
			{
            	regulator_disable(max8903->max8903_regulator);
				printk("[max8903 charger] Enter %s, disable Charger ( charing fault )....\n", __FUNCTION__);
			}
			msleep(500);
			if(!regulator_is_enabled(max8903->max8903_regulator))
			{
            	regulator_enable(max8903->max8903_regulator);
				printk("[max8903 charger]Re-charging : Due to charging fault.\n");
			}
		 	break;
		case MAX8903_PIN_DCOKn:
			if(max8903->irq[MAX8903_PIN_DCOKn].state == 0)
			{
				#ifdef CONFIG_TWL4030_USB
				max8903_get_supplies(max8903);
				#endif
				/* It could be usb supply (over detect d+ d- timing).*/	
				if(max8903->supply == MAX8903_BATTERY_SUPPLY)
					max8903->supply = MAX8903_USB_CABLE_SUPPLY;
				/* 
				* Enable charger when cable-in, 
				* in order to charge status pin will becaome active.
				*/
				if(!regulator_is_enabled(max8903->max8903_regulator) && (max8903->charging_state < MAX8903_CHARGING_STATUS_FULL))
            		regulator_enable(max8903->max8903_regulator);
            		
    //		schedule_delayed_work(&max8903->led_change_green,jiffies + msecs_to_jiffies(2000));
    			mod_timer(&led_change_green, jiffies + msecs_to_jiffies(2000));
			}
			else
			{
#ifdef CONFIG_REGULATOR_MAX8903_CABLE_TYPE_DETECTION
				max8903->has_usb_configuration = 0;
#endif

				max8903->supply = MAX8903_BATTERY_SUPPLY;
				/* 
				* Check condition manual after charger disable and cable remove. (In order to LED trigger after charging full)
				* Because charging status interrupt can't be touch off after charger disable
				*/
				if(max8903->charging_state >= MAX8903_CHARGING_STATUS_FULL && max8903->reboot == 0)
					schedule_delayed_work(&max8903->delay_work_reboot_state, 1);
					
					charge_en_state = gpio_get_value(pdata_info->irq_info[MAX8903_PIN_CHARHEn].gpio);
			
				printk("MAX8903_PIN_CHARHEn = %d,max8903->supply = %d\n",charge_en_state,max8903->supply);
				
					del_timer_sync(&led_change_green);
		//			cancel_delayed_work(&max8903->led_change_green);
	//			if((power_uevent->battery.dev) && (charge_en_state == 1) && (max8903->supply != MAX8903_BATTERY_SUPPLY))
					{
//						max8903->charging_state = MAX8903_CHARGING_STATUS_FULL;
//						power_supply_changed(&power_uevent->battery);
					}
	//			else
					{
						max8903->charging_state = MAX8903_CHARGING_STATUS_DISCONNECT;
						power_supply_changed(&power_uevent->battery);
					}
			}
#ifdef CONFIG_USB_CLOCK_SUSPEND				
//&*&*&*BC1_101027: add cable type flag
			g_usb_ac = max8903->supply;
//&*&*&*BC2_101027: add cable type flag	
#endif
			break;
	}
}

static void max8903_irq_call_handler(struct max8903 *max8903, int irq)
{
	mutex_lock(&max8903->irq_mutex);
	
	if (max8903->irq[irq].handler)
		max8903->irq[irq].handler(max8903, irq, max8903->irq[irq].data);
	else {
		dev_err(max8903->dev, "[max8903 charger] irq %d nobody cared. now masked.\n",
			irq);
	}

	mutex_unlock(&max8903->irq_mutex);
}

static void max8903_delay_worker_polling_state(struct work_struct *work)
{
	struct max8903 *max8903 = container_of(work, struct max8903, delay_work_reboot_state.work);
	struct max8903_mach_info *pdata = max8903->minfo;

	max8903->irq[MAX8903_PIN_DCOKn].state = gpio_get_value(pdata->irq_info[MAX8903_PIN_DCOKn].gpio);
	max8903->irq[MAX8903_PIN_CHARHEn].state = gpio_get_value(pdata->irq_info[MAX8903_PIN_CHARHEn].gpio);
		
	if(max8903->reboot == 1)
	{
		if(max8903->irq[MAX8903_PIN_DCOKn].state == 0)
		{
			#ifdef CONFIG_TWL4030_USB
			max8903_get_supplies(max8903);
			#endif
			/* It could be usb supply (over detect d+ d- timing).*/	
			if(max8903->supply == MAX8903_BATTERY_SUPPLY)
				max8903->supply = MAX8903_USB_CABLE_SUPPLY;
		}
//&*&*&*BC1_10726: fix issue that cpu cannot run 300Mhz issue
	adjust_cpu_frequency(max8903);
//&*&*&*BC2_10726: fix issue that cpu cannot run 300Mhz issue
	}

	max8903->reboot = 0;

	if(max8903->suspend)
	{
		printk("[max8903 charger] Enter %s - check charging status after suspend/resume, DC_OK is %d, Charging_State is %d\n", __FUNCTION__,
			max8903->irq[MAX8903_PIN_DCOKn].state, max8903->irq[MAX8903_PIN_CHARHEn].state);

		if(max8903->irq[MAX8903_PIN_DCOKn].state == 1)
		{
			max8903->supply = MAX8903_BATTERY_SUPPLY;
			//&*&*&*AL1_20101119, Modify to use the WAKEUP REASON for resuming behavior	
			if(!wake_lock_active(&max8903->ac_usb_charging_full_wake_lock))
				wake_unlock(&max8903->ac_usb_charging_full_wake_lock);
			printk("[max8903 charger] Enter %s, cable remove cause charging status change, unlock device for suspend/resume.\n", __FUNCTION__);
			//&*&*&*AL2_20101119, Modify to use the WAKEUP REASON for resuming behavior	
		}
		else
		{
			#ifdef CONFIG_TWL4030_USB
			max8903_get_supplies(max8903);
			#endif
			/* It could be usb supply (over detect d+ d- timing).*/	
			if(max8903->supply == MAX8903_BATTERY_SUPPLY)
				max8903->supply = MAX8903_USB_CABLE_SUPPLY;
		}

		max8903->suspend = 0;
	}

#ifdef CONFIG_USB_CLOCK_SUSPEND				
//&*&*&*BC1_101027: add cable type flag
	g_usb_ac = max8903->supply;
//&*&*&*BC2_101027: add cable type flag	
#endif
	
	printk("[max8903 charger] Enter %s, check charging status again without any interrupt occurs.\n", __FUNCTION__);
	max8903_charger_handler(max8903, MAX8903_PIN_CHARHEn, NULL);
}

static void max8903_irq_delay_worker_dcok_state(struct work_struct *work)
{
	struct max8903 *max8903 = container_of(work, struct max8903, irq_delay_work_dcok_state.work);
	struct max8903_mach_info *pdata = max8903->minfo;
	
	max8903->irq[MAX8903_PIN_DCOKn].state = gpio_get_value(pdata->irq_info[MAX8903_PIN_DCOKn].gpio);
	printk("[max8903 charger] Enter %s, DC_OK is %d\n", __FUNCTION__,
		max8903->irq[MAX8903_PIN_DCOKn].state);
	max8903_irq_call_handler(max8903, MAX8903_PIN_DCOKn);
	enable_irq(max8903->irq[MAX8903_PIN_DCOKn].chip_irq);
//&*&*&*BC1_10726: fix issue that cpu cannot run 300Mhz issue
	if (max8903->reboot == 0)
		adjust_cpu_frequency(max8903);
//&*&*&*BC2_10726: fix issue that cpu cannot run 300Mhz issue
}

//#ifdef MAX8903_FULL_FUNCTION_SUPPORT
static void max8903_irq_worker_fault_state(struct work_struct *work)
{
	struct max8903 *max8903 = container_of(work, struct max8903, irq_work_fault_state.work);
	struct max8903_mach_info *pdata = max8903->minfo;

	max8903->irq[MAX8903_PIN_FLAUTn].state = gpio_get_value(pdata->irq_info[MAX8903_PIN_FLAUTn].gpio);
	printk("[max8903 charger] Enter %s, Fault_State is %d\n", __FUNCTION__,
		max8903->irq[MAX8903_PIN_FLAUTn].state);
	max8903_irq_call_handler(max8903, MAX8903_PIN_FLAUTn);
	enable_irq(max8903->irq[MAX8903_PIN_FLAUTn].chip_irq);
}
//#endif

static void max8903_irq_delay_worker_charge_state(struct work_struct *work)
{
	struct max8903 *max8903 = container_of(work, struct max8903, irq_delay_work_charge_state.work);
	struct max8903_mach_info *pdata = max8903->minfo;
	
	max8903->irq[MAX8903_PIN_DCOKn].state = gpio_get_value(pdata->irq_info[MAX8903_PIN_DCOKn].gpio);
	max8903->irq[MAX8903_PIN_CHARHEn].state = gpio_get_value(pdata->irq_info[MAX8903_PIN_CHARHEn].gpio);
	printk("[max8903 charger] Enter %s, DC_OK is %d, Charging_State is %d\n", __FUNCTION__,
		max8903->irq[MAX8903_PIN_DCOKn].state, max8903->irq[MAX8903_PIN_CHARHEn].state);
	max8903_irq_call_handler(max8903, MAX8903_PIN_CHARHEn);
	enable_irq(max8903->irq[MAX8903_PIN_CHARHEn].chip_irq);
}

static irqreturn_t max8903_irq_handler(int irq, void *data)
{
	struct max8903 *max8903 = (struct max8903 *)data;

	disable_irq_nosync(irq);
	
	printk("[max8903 charger] Enter %s, irq number is %d.\n", __FUNCTION__, irq);

	switch (irq)
	{
		//case 309:
		case OMAP_GPIO_IRQ(15):
			/* Wait a moment until charging status stable */
			schedule_delayed_work(&max8903->irq_delay_work_charge_state, 200);
			break;
#ifdef MAX8903_FULL_FUNCTION_SUPPORT
		case 209:
			schedule_delayed_work(&max8903->irq_work_fault_state, 500);
			break;
#endif
	//	case 211:
		case OMAP_GPIO_IRQ(51):
			/* must detect ac/usb promptly, due to usb will enter suspend when it doesn't have communication. */
			schedule_delayed_work(&max8903->irq_delay_work_dcok_state, 50);
			break;
		default:
			printk("[max8903 charger] Enter %s, no irq number is %d.\n", __FUNCTION__, irq);
			break;
	}
	
	return IRQ_HANDLED;
}

static int max8903_register_irq(struct max8903 *max8903, int irq,
			void (*handler) (struct max8903 *, int, void *),
			void *data)
{
	if (irq < 0 || irq > MAX8903_IRQ_NUM || !handler)
		return -EINVAL;

	if (max8903->irq[irq].handler)
		return -EBUSY;

	mutex_lock(&max8903->irq_mutex);
	max8903->irq[irq].handler = handler;
	max8903->irq[irq].data = data;
	mutex_unlock(&max8903->irq_mutex);

	return 0;
}

static struct regulator_ops max8903_ops = {
	.set_current_limit = max8903_set_current_limit,
	.get_current_limit = max8903_get_current_limit,
	.enable            = max8903_enable,
	.disable           = max8903_disable,
	.is_enabled        = max8903_is_enabled,
};

static struct regulator_desc max8903_desc = {
	.name  = "max8903",
	.ops   = &max8903_ops,
	.type  = REGULATOR_CURRENT,
};

static int __devinit max8903_probe(struct platform_device *pdev)
{
	struct regulator_init_data *i_pdata = pdev->dev.platform_data;
	struct max8903_mach_info *pdata = (struct max8903_mach_info *)(i_pdata->driver_data);
	struct regulator_dev *max8903_regulator_dev;
	struct max8903 *max8903;
	struct input_dev *input_dev;
	int ret, i;

	if (!pdata || !pdata->gpio_cen || !pdata->gpio_dcm || !pdata->irq_info || !i_pdata)
		return -EINVAL;

	max8903 = kzalloc(sizeof(struct max8903), GFP_KERNEL);
	if (max8903 == NULL) 
		return -ENOMEM;

	struct max8903_power *power_uevnet = &max8903->power_uevent;
	struct power_supply *usb = &power_uevnet->usb;
	struct power_supply *ac = &power_uevnet->ac;
	struct power_supply *battery = &power_uevnet->battery;

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		dev_dbg(max8903->dev, "[max8903 charger] allocate input device fail!!.\n");
		goto err_alloc;
	}
	max8903->intpu_dev = input_dev;
	/* Initial Max8903 Data Structure*/
	max8903->dev = &pdev->dev;
	max8903->minfo = pdata;
	mutex_init(&max8903->irq_mutex);
	spin_lock_init(&max8903->spinlock);
	wake_lock_init(&max8903->ac_usb_charging_full_wake_lock, WAKE_LOCK_SUSPEND, "AcUsbEvent");
	/* 
	*   Due to irqs will enter interrupt handler concurrent. In order to avoid loss functions in the same work_queue, 
	*   we must initial four work_queue.
	*/
	INIT_DELAYED_WORK(&max8903->irq_delay_work_charge_state, max8903_irq_delay_worker_charge_state);
#ifdef MAX8903_FULL_FUNCTION_SUPPORT
	INIT_DELAYED_WORK(&max8903->irq_work_fault_state, max8903_irq_worker_fault_state);
#endif
	INIT_DELAYED_WORK(&max8903->irq_delay_work_dcok_state, max8903_irq_delay_worker_dcok_state);
	INIT_DELAYED_WORK(&max8903->delay_work_reboot_state, max8903_delay_worker_polling_state);
	max8903->reboot = 1;

	/* Config charger enable and set_current pin */
	ret = gpio_request(pdata->gpio_cen, "ncharge_en");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request CEN GPIO: %d\n",
			pdata->gpio_cen);
		goto err_cen;
	}
	ret = gpio_request(pdata->gpio_dcm, "charge_mode_in1");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request DCM GPIO: %d\n",
			pdata->gpio_dcm);
		goto err_dcm;
	}
	/*
	ret = gpio_request(pdata->gpio_iusb, "charge_mode_in2");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request IUSB GPIO: %d\n",
			pdata->gpio_iusb);
		goto err_iusb;
	}
	*/
	ret = gpio_direction_output(pdata->gpio_dcm, pdata->gpio_dcm_state);
//	ret = gpio_direction_output(pdata->gpio_iusb, pdata->gpio_iusb_state);
	ret = gpio_direction_output(pdata->gpio_cen, pdata->gpio_cen_state);

	/* Registered IRQ, some irq pins don't have to use now. Config input pin */
	for( i=0; i<MAX8903_IRQ_NUM; i++ )
	{
#ifndef MAX8903_FULL_FUNCTION_SUPPORT
		if(i == MAX8903_PIN_FLAUTn)
		{
			if (gpio_request(pdata->irq_info[i].gpio, "max8903_gpio") < 0) 
			{				
				printk(KERN_ERR "can't get general GPIO for max8903 \n");
				return -ENXIO;			
			}			
			gpio_direction_input(pdata->irq_info[i].gpio);
		}
		else
#endif
		{
			if(gpio_request(pdata->irq_info[i].gpio, "max8903_irq") < 0)
			{			
				printk(KERN_ERR "Failed to request GPIO%d for max8903 IRQ\n", pdata->irq_info[i].gpio);
				return -ENXIO;		
			}		
			gpio_direction_input(pdata->irq_info[i].gpio);
			ret = request_irq(pdata->irq_info[i].irq, max8903_irq_handler, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "max8903", max8903);
			if (ret) {			
				dev_err(max8903->dev, "can't allocate irq %d\n", pdata->irq_info[i].irq);
				goto err_irq;		
			}
			max8903_register_irq(max8903, i, max8903_charger_handler, NULL);
			//Save real irq number for determine which irq 
			max8903->irq[i].chip_irq = pdata->irq_info[i].irq;
		}
	}

	/* Registered Regulator */
	i_pdata->consumer_supplies->dev = &pdev->dev;
	max8903_regulator_dev = regulator_register(&max8903_desc, &pdev->dev, i_pdata, pdata);
	if (IS_ERR(max8903_regulator_dev)) {
		dev_dbg(max8903->dev, "[max8903 charger] couldn't register regulator(max8903).\n");
		ret = PTR_ERR(max8903_regulator_dev);
		goto err_reg;
	}
	dev_dbg(&pdev->dev, "[max8903 charger] registered regulator(max8903) success.\n");
	
	max8903->max8903_regulator_dev = max8903_regulator_dev;
	platform_set_drvdata(pdev, max8903);
	
	/* Get regulator to enable charging first. It can avoid regulator's count error with disable charging when boot-up */
	if(max8903->max8903_regulator == NULL || IS_ERR(max8903->max8903_regulator)) {
		max8903->max8903_regulator = regulator_get(max8903->dev, "max8903");
	}
	if(max8903->max8903_regulator)
		regulator_enable(max8903->max8903_regulator);
	
	/* 
	* Re-charging after charging full while capacity lower than 98%.
	* Configuration as 100 is in order to avoid enable charger when charging full on device boot condition. 
	*/
	max8903->battery_soc = 100;
//#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
	max8903->temperature = 20;
//#endif
	g_max8903 = max8903;
	
	/* Registered power_supply subsystem to notify android */
	ac->name = "ac";
	ac->type = POWER_SUPPLY_TYPE_MAINS;
	ac->properties = max8903_ac_props;
	ac->num_properties = ARRAY_SIZE(max8903_ac_props);
	ac->get_property = max8903_ac_get_prop;
	ret = power_supply_register(&pdev->dev, ac);
	if (ret)
		goto err_ac_supply;

	usb->name = "usb";
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->properties = max8903_usb_props;
	usb->num_properties = ARRAY_SIZE(max8903_usb_props);
	usb->get_property = max8903_usb_get_prop;
	ret = power_supply_register(&pdev->dev, usb);
	if (ret)
		goto userr_usb_supply;

	battery->name		= "battery";
	battery->type		= POWER_SUPPLY_TYPE_BATTERY;
	battery->properties = max8903_battery_props;
	battery->num_properties = ARRAY_SIZE(max8903_battery_props);
	battery->get_property	= max8903_battery_get_prop;
	
	ret = power_supply_register(&pdev->dev, battery);
	if (ret) {
		goto userr_battery_supply;
	}
	
	/* Register the input device and enable EV_KEY (KEY_END) */
	set_bit(EV_KEY, input_dev->evbit);
	input_set_capability(input_dev, EV_KEY, KEY_END);

	input_dev->name = "max8903-charger";
	input_dev->phys = "max8903-charger/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0002;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0002;	
	
	ret = input_register_device(input_dev);
	if (ret)
	{
		dev_dbg(max8903->dev, "[max8903 charger] register input device fail!!.\n");
		goto reg_fail;
	}

//	INIT_DELAYED_WORK(&max8903->led_schedule_green, led_schedule_handler);
	INIT_WORK(&max8903->led_schedule_green, led_schedule_handler);
	
	init_timer(&led_change_green);
	led_change_green.function = led_change_green_handler;
	led_change_green.data = (unsigned long)max8903;

	if(gpio_get_value(pdata->irq_info[MAX8903_PIN_DCOKn].gpio) == 0)
		mod_timer(&led_change_green, jiffies + msecs_to_jiffies(8000));
	/* Detect adapter */
//&*&*&*BC1_10726: fix issue that cpu cannot run 300Mhz issue
//	schedule_delayed_work(&max8903->delay_work_reboot_state, 6 * HZ);
//&*&*&*BC2_10726: fix issue that cpu cannot run 300Mhz issue

	return 0;
	
reg_fail:
	power_supply_unregister(battery);
userr_battery_supply:
	power_supply_unregister(usb);
userr_usb_supply:
	power_supply_unregister(ac);
err_ac_supply:
	regulator_unregister(max8903_regulator_dev);
err_reg:
	for( i=0 ; i<MAX8903_IRQ_NUM ; i++ )
	{
#ifndef MAX8903_FULL_FUNCTION_SUPPORT
		if(i == MAX8903_PIN_FLAUTn)
			;
		else
		{
#endif
			free_irq(max8903->irq[i].chip_irq, max8903);
#ifndef MAX8903_FULL_FUNCTION_SUPPORT
		}
#endif
	}
err_irq:
err_cen:
err_dcm:
//err_iusb:
	input_free_device(input_dev);
err_alloc:
	kfree(max8903);
	return ret;
}

static int __devexit max8903_remove(struct platform_device *pdev)
{
	struct max8903 *max8903 = platform_get_drvdata(pdev);
	struct max8903_power *power = &max8903->power_uevent;
	struct regulator_dev *max8903_regulator_dev = max8903->max8903_regulator_dev;
	int i;
	
	regulator_unregister(max8903_regulator_dev);
	for( i=0 ; i<MAX8903_IRQ_NUM ; i++ )
	{
#ifndef MAX8903_FULL_FUNCTION_SUPPORT
		if(i == MAX8903_PIN_FLAUTn)
			;
		else
		{
#endif
		free_irq(max8903->irq[i].chip_irq, max8903);
#ifndef MAX8903_FULL_FUNCTION_SUPPORT
		}
#endif
	}

	power_supply_unregister(&power->ac);
	power_supply_unregister(&power->usb);
	power_supply_unregister(&power->battery);

	input_free_device(max8903->intpu_dev);
	
	kfree(max8903);

	return 0;
}

#ifdef CONFIG_PM

static int max8903_suspend(struct platform_device *dev, pm_message_t state)
{
	int ret;
	struct max8903 *max8903 = platform_get_drvdata(dev);
	struct max8903_mach_info *pdata = max8903->minfo;
	/* Cancel led trigger timer */
#ifdef CONFIG_EP10_LED_BLINK
	if(ledtrig_charging_timer.function && ledtrig_charging_timer.data)
	{
		del_timer(&ledtrig_charging_timer);
		charging_activity = 0;
	}
#endif

	ret = cancel_delayed_work(&max8903->irq_delay_work_charge_state);
	printk("[max8903 charger]Enter %s, cancel charging status irq work's. ret is %d.\n", __FUNCTION__, ret);
	cancel_delayed_work(&max8903->delay_work_reboot_state);
	max8903->suspend = 1;

	#ifdef CONFIG_LEDS_EP10
	//&*&*&*AL1_20101130, disable led on suspend and remove cable
	max8903->irq[MAX8903_PIN_DCOKn].state = gpio_get_value(pdata->irq_info[MAX8903_PIN_DCOKn].gpio);
	if(max8903->irq[MAX8903_PIN_DCOKn].state)
	{
		printk("[max8903 charger]Enter %s, disable led on suspend and remove cable...\n", __FUNCTION__);	
//		ep10_set_led_gpio(0);
	}
	//&*&*&*AL2_20101130, disable led on suspend and remove cable
	#endif
	//&*&*&*AL1_20101130, enable charge status irq when charger status work queue not execute yet.
	if(ret)
		enable_irq(max8903->irq[MAX8903_PIN_CHARHEn].chip_irq);
	//&*&*&*AL2_20101130, enable charge status irq when charger status work queue not execute yet.
	return 0;
}

static int max8903_resume(struct platform_device *dev)
{
	struct max8903 *max8903 = platform_get_drvdata(dev);
	printk("============%s",__func__);
#if 0
	//&*&*&*AL1_20101119, Modify to use the WAKEUP REASON for resuming behavior	
	if ((g_wakeup_stat & (0x1 << 0)) && (g_eint0pend & (0x1 << 0)))
	{
		if(!wake_lock_active(&max8903->ac_usb_charging_full_wake_lock))
			wake_lock(&max8903->ac_usb_charging_full_wake_lock);
		printk("[max8903 charger] Enter %s, charging status changed on resume, lock device.\n", __FUNCTION__);
	}
	//&*&*&*AL2_20101119, Modify to use the WAKEUP REASON for resuming behavior	
#endif
	/* Can't receive charger status interrupt when device resume, so need to re-check again. */
	schedule_delayed_work(&max8903->delay_work_reboot_state, 1);
	return 0;
}

#else

#define max17043_suspend NULL
#define max17043_resume NULL

#endif /* CONFIG_PM */

static struct platform_driver max8903_driver = {
	.driver = {
		.name = "max8903",
	},
	.probe		= max8903_probe,
	.remove = __devexit_p(max8903_remove),
	.suspend	= max8903_suspend,
	.resume		= max8903_resume,
};

static int __init max8903_init(void)
{
	return platform_driver_probe(&max8903_driver, max8903_probe);
}

static void __exit max8903_exit(void)
{
	platform_driver_unregister(&max8903_driver);
}

module_init(max8903_init);
module_exit(max8903_exit);

MODULE_AUTHOR("Aimar Liu");
MODULE_DESCRIPTION("Maxim 8903 Li-Ion Charger driver");
MODULE_LICENSE("GPL");
