/* drivers/leds/leds-button-ep10.c
 *
 * Copyright (c) 2011 Foxconn
 *	
 * Aimar Liu <aimar.ts.liu@foxconn.com>
 *
 * EP10 - LEDs GPIO driver
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

struct ep10_gpio_button_led {
	struct led_classdev		 cdev;
	struct ep10_led_platdata	*pdata;
};

static struct ep10_gpio_button_led *gpio_button_led;

static inline struct ep10_gpio_button_led *pdev_to_ep10_gpio_button_led(struct platform_device *dev)
{
	return platform_get_drvdata(dev);
}

static inline struct ep10_gpio_button_led *to_ep10_gpio_button_led(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct ep10_gpio_button_led, cdev);
}

void ep10_set_button_led_gpio(int value)
{
	struct ep10_led_platdata *pd;
	if(gpio_button_led == NULL)
		return;
	pd = gpio_button_led->pdata;
	gpio_set_value(pd->gpio, (value ? 1 : 0));	
}
EXPORT_SYMBOL(ep10_set_button_led_gpio);

static void ep10_button_led_set(struct led_classdev *led_cdev,
			    enum led_brightness value)
{
	struct ep10_gpio_button_led *led = to_ep10_gpio_button_led(led_cdev);
	struct ep10_led_platdata *pd = led->pdata;
	gpio_set_value(pd->gpio, (value ? 1 : 0));	
}

static int ep10_button_led_probe(struct platform_device *pdev)
{
	struct ep10_led_platdata *pdata = pdev->dev.platform_data;
	struct ep10_gpio_button_led *led;
	int ret;

	printk("[ep10 button leds]Enter %s...id is %d\n", __FUNCTION__, pdev->id);

	if (!pdata || !pdata->gpio || !pdata->def_trigger || !pdata->name)
		return -EINVAL;
	
	led = kzalloc(sizeof(struct ep10_gpio_button_led), GFP_KERNEL);
	if (led == NULL) {
		dev_err(&pdev->dev, "[ep10 button led]No memory for ep10 button led device\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, led);

	led->cdev.brightness_set = ep10_button_led_set;
	led->cdev.default_trigger = pdata->def_trigger;
	led->cdev.name = pdata->name;
	led->cdev.flags |= LED_CORE_SUSPENDRESUME;
	/* Set LED brightness to 0 when resume */
	led->cdev.brightness = 0;

	led->pdata = pdata;

	gpio_button_led = led;

	/* no point in having a pull-up if we are always driving */
	ret = gpio_request(pdata->gpio, "charging_led");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request led GPIO: %d\n",
			pdata->gpio);
		goto exit_err1;
	}
	ret = gpio_direction_output(pdata->gpio, 0);
	
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

static int ep10_button_led_remove(struct platform_device *dev)
{
	struct ep10_gpio_button_led *led = pdev_to_ep10_gpio_button_led(dev);

	led_classdev_unregister(&led->cdev);
	kfree(led);

	return 0;
}

static struct platform_driver ep10_button_led_driver = {
	.probe		= ep10_button_led_probe,
	.remove		= ep10_button_led_remove,
	.driver		= {
		.name		= "ep10_button_led",
		.owner		= THIS_MODULE,
	},
};

static int __init ep10_button_led_init(void)
{
	return platform_driver_register(&ep10_button_led_driver);
}

static void __exit ep10_button_led_exit(void)
{
	platform_driver_unregister(&ep10_button_led_driver);
}

module_init(ep10_button_led_init);
module_exit(ep10_button_led_exit);

MODULE_AUTHOR("Aimar Liu <aimar.ts.liu@foxconn.com>");
MODULE_DESCRIPTION("EP10 Button LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ep10-button-leds");
