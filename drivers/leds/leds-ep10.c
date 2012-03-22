/* drivers/leds/leds-ep10.c
 *
 * Copyright (c) 2010 Foxconn
 *	
 * Aimar Liu <aimar.ts.liu@foxconn.com>
 *
 * EK1 - LEDs GPIO driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds-ep10.h>
#include <linux/gpio.h>
#include <linux/slab.h>

/* our context */

struct ep10_gpio_led {
	struct led_classdev		 cdev;
	struct ep10_led_platdata	*pdata;
};

static struct ep10_gpio_led *gpio_led;

extern int led_on_flags;/*<--LH_SWRD_CL1_Beacon@2011-6-7,add a varible-->*/
extern u8 battery_capacity;
static inline struct ep10_gpio_led *pdev_to_ep10_gpio_led(struct platform_device *dev)
{
	return platform_get_drvdata(dev);
}

static inline struct ep10_gpio_led *to_ep10_gpio_led(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct ep10_gpio_led, cdev);
}

void ep10_set_led_gpio(int value)
{
	struct ep10_led_platdata *pd;
	if(gpio_led == NULL)
		return;
	pd = gpio_led->pdata;
	gpio_set_value(pd->gpio_charge,(value ? 1: 0));
	gpio_set_value(pd->gpio_charge_full,(value ? 0:1)); 
}
EXPORT_SYMBOL(ep10_set_led_gpio);

static int ep10_led_set(struct led_classdev *led_cdev,
			    enum led_brightness value)
{
	struct ep10_gpio_led *led = to_ep10_gpio_led(led_cdev);
	struct ep10_led_platdata *pd = led->pdata;
	int suspend_flag = led_cdev->flags & LED_SUSPENDED;
	
	//printk("-----------------%s,value = %d,led_on_flags = %d\n",__func__,value,led_on_flags);
	/*<--LH_SWRD_CL1_Beacon@2011-7-25,add a state of led-->*/
	if(battery_capacity  <= 10){
		gpio_set_value(pd->gpio_charge,(value? 1: 0));
		//printk("-----------------%s,battery_soc_ep10 = %d\n",__func__,battery_capacity);
		
		return 0;
	}
	 if(led_on_flags != 0 ){
		led_on_flags  = 0;
		gpio_set_value(pd->gpio_charge, 0);
		gpio_set_value(pd->gpio_charge_full, 0);
		
		return 0;
	}
	else if(suspend_flag){
		gpio_set_value(pd->gpio_charge, 0);
		gpio_set_value(pd->gpio_charge_full, 0);
	}
 	else{
		gpio_set_value(pd->gpio_charge,(value? 1: 0));
		gpio_set_value(pd->gpio_charge_full,(value? 0:1)); 
	}
	
	return 0;
}

static int ep10_led_probe(struct platform_device *pdev)
{
	struct ep10_led_platdata *pdata = pdev->dev.platform_data;
	struct ep10_gpio_led *led;
	int ret;

	printk("[ep10 leds]Enter %s...id is %d\n", __FUNCTION__, pdev->id);

	if (!pdata || !pdata->gpio_charge || !pdata->gpio_charge_full || !pdata->def_trigger || !pdata->name)
		return -EINVAL;
	
	led = kzalloc(sizeof(struct ep10_gpio_led), GFP_KERNEL);
	if (led == NULL) {
		dev_err(&pdev->dev, "[ep10 led]No memory for ep10 led device\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, led);

	led->cdev.brightness_set = ep10_led_set;
	led->cdev.default_trigger = pdata->def_trigger;
	led->cdev.name = pdata->name;
	led->cdev.flags |= LED_CORE_SUSPENDRESUME;
	/* Set LED brightness to 0 when resume */
	led->cdev.brightness = 0;

	led->pdata = pdata;

	gpio_led = led;

	/* no point in having a pull-up if we are always driving */
	ret = gpio_request(pdata->gpio_charge, "charging_led");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request led GPIO: %d\n",
			pdata->gpio_charge);
		goto exit_err1;
	}
	ret = gpio_direction_output(pdata->gpio_charge, 0);
	ret = gpio_request(pdata->gpio_charge_full, "charging_full_led");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request led GPIO: %d\n",
			pdata->gpio_charge_full);
		goto exit_err1;
	}

	ret = gpio_direction_output(pdata->gpio_charge_full, 0);
	
	/* register our new led device */
	ret = led_classdev_register(&pdev->dev, &led->cdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "[ep10 led]led_classdev_register failed\n");
		goto exit_err1;
	}

	return 0;

 exit_err1:
	kfree(led);
	return ret;
}

static int ep10_led_remove(struct platform_device *dev)
{
	struct ep10_gpio_led *led = pdev_to_ep10_gpio_led(dev);

	led_classdev_unregister(&led->cdev);
	kfree(led);

	return 0;
}

static struct platform_driver ep10_led_driver = {
	.probe		= ep10_led_probe,
	.remove		= ep10_led_remove,
	.driver		= {
		.name		= "ep10_led",
		.owner		= THIS_MODULE,
	},
};

static int __init ep10_led_init(void)
{
	return platform_driver_register(&ep10_led_driver);
}

static void __exit ep10_led_exit(void)
{
	platform_driver_unregister(&ep10_led_driver);
}

module_init(ep10_led_init);
module_exit(ep10_led_exit);

MODULE_AUTHOR("Aimar Liu <aimar.ts.liu@foxconn.com>");
MODULE_DESCRIPTION("EK1 LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ep10-leds");
