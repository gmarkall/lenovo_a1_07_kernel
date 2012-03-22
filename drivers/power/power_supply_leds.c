/*
 *  LEDs triggers for power supply class
 *
 *  Copyright © 2007  Anton Vorontsov <cbou@mail.ru>
 *  Copyright © 2004  Szabolcs Gyurko
 *  Copyright © 2003  Ian Molton <spyro@f2s.com>
 *
 *  Modified: 2004, Oct     Szabolcs Gyurko
 *
 *  You may use this code as per GPL version 2
 */

#include <linux/kernel.h>
#include <linux/power_supply.h>
#include <linux/slab.h>

#include "power_supply.h"

/* Battery specific LEDs triggers. */
int led_on_flags =	0;/*<--LH_SWRD_CL1_Beacon@2011-6-7,add a varible-->*/
extern u8 battery_capacity;

#ifdef CONFIG_EP10_LED_BLINK
#include <linux/timer.h>

struct timer_list ledtrig_charging_timer;
int charging_activity = 0;
static int charging_lastactivity = 0;

static void ledtrig_charging_activity(void)
{
	if(charging_activity == 0)
	{
		charging_activity++;
		if (!timer_pending(&ledtrig_charging_timer))
		{
			mod_timer(&ledtrig_charging_timer, jiffies + msecs_to_jiffies(1000));
		}
	}
}

static void ledtrig_charging_timerfunc(unsigned long data)
{
	struct power_supply* psy = (struct power_supply*)data;
	if (charging_lastactivity != charging_activity) {
		charging_activity++;
		charging_lastactivity = charging_activity;
		led_trigger_event(psy->charging_trig, LED_FULL);
		mod_timer(&ledtrig_charging_timer, jiffies + msecs_to_jiffies(1000));
	} else {
		led_trigger_event(psy->charging_trig, LED_OFF);
		charging_activity--;
		mod_timer(&ledtrig_charging_timer, jiffies + msecs_to_jiffies(500));
	}
	/*<--LH_SWRD_CL1_Beacon@2011-7-25,del a state of led-->*/
	if(battery_capacity >10){
		//printk("-----------------%s,capacity=%d\n",__func__,battery_capacity);
		del_timer(&ledtrig_charging_timer);
       		 charging_activity = 0;
		 led_trigger_event(psy->charging_trig, LED_FULL);
	}
}
#endif

static void power_supply_update_bat_leds(struct power_supply *psy)
{
	union power_supply_propval status;

	if (psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &status))
		return;

	dev_dbg(psy->dev, "%s %d\n", __func__, status.intval);

	switch (status.intval) {
	case POWER_SUPPLY_STATUS_FULL:
#if 0
#ifdef CONFIG_EP10_LED_BLINK
		del_timer(&ledtrig_charging_timer);
        charging_activity = 0;
#endif
#endif
		printk("==Entry charge full state:battery_capacity = %d\n",battery_capacity);
		led_trigger_event(psy->charging_full_trig, LED_OFF);
		led_trigger_event(psy->charging_trig, LED_OFF);
		led_trigger_event(psy->full_trig, LED_OFF);
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		/*<--LH_SWRD_CL1_Beacon@2011-7-25,add a state of led-->*/
#ifdef CONFIG_EP10_LED_BLINK
		if(battery_capacity <= 10){
		//printk("===============%s,battery_capacity = %d\n",__func__,battery_capacity);
		led_trigger_event(psy->charging_full_trig, LED_OFF);
		ledtrig_charging_activity();
		}
#else
		led_trigger_event(psy->charging_full_trig, LED_OFF);
		led_trigger_event(psy->charging_trig, LED_FULL);
#endif
		led_trigger_event(psy->full_trig, LED_OFF);
		led_trigger_event(psy->charging_trig, LED_FULL);
		break;
	case POWER_SUPPLY_HEALTH_COLD:
	case POWER_SUPPLY_HEALTH_UNKNOWN:
		led_trigger_event(psy->charging_full_trig, LED_OFF);
		led_trigger_event(psy->charging_trig, LED_OFF);
		led_trigger_event(psy->full_trig, LED_OFF);
		break;
		/*<--LH_SWRD_CL1_Beacon@2011-6-7,add a trigger  condition*/
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		led_on_flags ++;
#ifdef CONFIG_EP10_LED_BLINK
		if(battery_capacity <= 10){
		del_timer(&ledtrig_charging_timer);
       		 charging_activity = 0;
		}
#endif
		printk("Entry discharging:led_on_flags=%d!\n",led_on_flags);
		led_trigger_event(psy->charging_trig, LED_OFF);
		break;
	default:
#ifdef CONFIG_EP10_LED_BLINK
		del_timer(&ledtrig_charging_timer);
		charging_activity = 0;
#endif
		led_trigger_event(psy->charging_full_trig, LED_OFF);
		led_trigger_event(psy->charging_trig, LED_OFF);
		led_trigger_event(psy->full_trig, LED_OFF);
		break;
	}
}

static int power_supply_create_bat_triggers(struct power_supply *psy)
{
	int rc = 0;

	psy->charging_full_trig_name = kasprintf(GFP_KERNEL,
					"%s-charging-or-full", psy->name);
	if (!psy->charging_full_trig_name)
		goto charging_full_failed;

	psy->charging_trig_name = kasprintf(GFP_KERNEL,
					"%s-charging", psy->name);
	if (!psy->charging_trig_name)
		goto charging_failed;

	psy->full_trig_name = kasprintf(GFP_KERNEL, "%s-full", psy->name);
	if (!psy->full_trig_name)
		goto full_failed;

#ifdef CONFIG_EP10_LED_BLINK
	if(!strcmp(psy->name, "battery"))
	{
		ledtrig_charging_timer.function = ledtrig_charging_timerfunc;
		ledtrig_charging_timer.data = (unsigned long)psy;
		init_timer(&ledtrig_charging_timer);
	}
#endif

	led_trigger_register_simple(psy->charging_full_trig_name,
				    &psy->charging_full_trig);
	led_trigger_register_simple(psy->charging_trig_name,
				    &psy->charging_trig);
	led_trigger_register_simple(psy->full_trig_name,
				    &psy->full_trig);

	goto success;

full_failed:
	kfree(psy->charging_trig_name);
charging_failed:
	kfree(psy->charging_full_trig_name);
charging_full_failed:
	rc = -ENOMEM;
success:
	return rc;
}

static void power_supply_remove_bat_triggers(struct power_supply *psy)
{
	led_trigger_unregister_simple(psy->charging_full_trig);
	led_trigger_unregister_simple(psy->charging_trig);
	led_trigger_unregister_simple(psy->full_trig);
#ifdef CONFIG_EP10_LED_BLINK
	del_timer(&ledtrig_charging_timer);
#endif
	kfree(psy->full_trig_name);
	kfree(psy->charging_trig_name);
	kfree(psy->charging_full_trig_name);
}

/* Generated power specific LEDs triggers. */

static void power_supply_update_gen_leds(struct power_supply *psy)
{
	union power_supply_propval online;

	if (psy->get_property(psy, POWER_SUPPLY_PROP_ONLINE, &online))
		return;

	dev_dbg(psy->dev, "%s %d\n", __func__, online.intval);

	if (online.intval)
		led_trigger_event(psy->online_trig, LED_FULL);
	else
		led_trigger_event(psy->online_trig, LED_OFF);
}

static int power_supply_create_gen_triggers(struct power_supply *psy)
{
	int rc = 0;

	psy->online_trig_name = kasprintf(GFP_KERNEL, "%s-online", psy->name);
	if (!psy->online_trig_name)
		goto online_failed;

	led_trigger_register_simple(psy->online_trig_name, &psy->online_trig);

	goto success;

online_failed:
	rc = -ENOMEM;
success:
	return rc;
}

static void power_supply_remove_gen_triggers(struct power_supply *psy)
{
	led_trigger_unregister_simple(psy->online_trig);
	kfree(psy->online_trig_name);
}

/* Choice what triggers to create&update. */

void power_supply_update_leds(struct power_supply *psy)
{
	if (psy->type == POWER_SUPPLY_TYPE_BATTERY)
		power_supply_update_bat_leds(psy);
	else
		power_supply_update_gen_leds(psy);
}

int power_supply_create_triggers(struct power_supply *psy)
{
	if (psy->type == POWER_SUPPLY_TYPE_BATTERY)
		return power_supply_create_bat_triggers(psy);
	return power_supply_create_gen_triggers(psy);
}

void power_supply_remove_triggers(struct power_supply *psy)
{
	if (psy->type == POWER_SUPPLY_TYPE_BATTERY)
		power_supply_remove_bat_triggers(psy);
	else
		power_supply_remove_gen_triggers(psy);
}
