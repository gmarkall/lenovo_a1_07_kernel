/*
 * OMAP display LED Driver
 *
 * Copyright (C) 2010 Texas Instruments
 *
 * Author: Dan Murphy <DMurphy@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/leds-omap-display.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/i2c/twl.h>

struct display_led_data {
	struct led_classdev pri_display_class_dev;
	struct led_classdev sec_display_class_dev;
	struct omap_disp_led_platform_data *led_pdata;
	struct mutex pri_disp_lock;
	struct mutex sec_disp_lock;
};

static void omap_primary_disp_store(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	struct display_led_data *led_data = container_of(led_cdev,
			struct display_led_data, pri_display_class_dev);
	mutex_lock(&led_data->pri_disp_lock);

	if (led_data->led_pdata->primary_display_set)
		led_data->led_pdata->primary_display_set(value);

	mutex_unlock(&led_data->pri_disp_lock);
}

static void omap_secondary_disp_store(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	struct display_led_data *led_data = container_of(led_cdev,
			struct display_led_data, sec_display_class_dev);

	mutex_lock(&led_data->sec_disp_lock);

	if ((led_data->led_pdata->flags & LEDS_CTRL_AS_TWO_DISPLAYS) &&
		led_data->led_pdata->secondary_display_set)
			led_data->led_pdata->secondary_display_set(value);

	mutex_unlock(&led_data->sec_disp_lock);
}

static int omap_display_led_probe(struct platform_device *pdev)
{
	int ret;
	struct display_led_data *info;

	pr_info("%s:Enter\n", __func__);

	printk("enter [PWM] omap_display_led_probe\n");

	if (pdev->dev.platform_data == NULL) {
		pr_err("%s: platform data required\n", __func__);
		return -ENODEV;
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	info->led_pdata = pdev->dev.platform_data;
	platform_set_drvdata(pdev, info);

	info->pri_display_class_dev.name = "lcd-backlight";
	info->pri_display_class_dev.brightness_set = omap_primary_disp_store;
	info->pri_display_class_dev.max_brightness = LED_FULL;
	mutex_init(&info->pri_disp_lock);

	ret = led_classdev_register(&pdev->dev,
				    &info->pri_display_class_dev);
	if (ret < 0) {
		pr_err("%s: Register led class failed\n", __func__);
		kfree(info);
		return ret;
	}

	if (info->led_pdata->flags & LEDS_CTRL_AS_TWO_DISPLAYS) {
		pr_info("%s: Configuring the secondary LED\n", __func__);
		info->sec_display_class_dev.name = "lcd-backlight2";
		info->sec_display_class_dev.brightness_set =
			omap_secondary_disp_store;
		info->sec_display_class_dev.max_brightness = LED_OFF;
		mutex_init(&info->sec_disp_lock);

		ret = led_classdev_register(&pdev->dev,
					    &info->sec_display_class_dev);
		if (ret < 0) {
			pr_err("%s: Register led class failed\n", __func__);
			kfree(info);
			return ret;
		}

		if (info->led_pdata->secondary_display_set)
			info->led_pdata->secondary_display_set(0);
	}

	pr_info("%s:Exit\n", __func__);

	return ret;
}

static int omap_display_led_remove(struct platform_device *pdev)
{
	struct display_led_data *info = platform_get_drvdata(pdev);

	led_classdev_unregister(&info->pri_display_class_dev);
	if (info->led_pdata->flags & LEDS_CTRL_AS_TWO_DISPLAYS)
		led_classdev_unregister(&info->sec_display_class_dev);

	return 0;
}

static struct platform_driver omap_display_led_driver = {
	.probe = omap_display_led_probe,
	.remove = omap_display_led_remove,
	.driver = {
		   .name = "display_led",
		   .owner = THIS_MODULE,
		   },
};

static int __init omap_display_led_init(void)
{
	return platform_driver_register(&omap_display_led_driver);
}

static void __exit omap_display_led_exit(void)
{
	platform_driver_unregister(&omap_display_led_driver);
}

module_init(omap_display_led_init);
module_exit(omap_display_led_exit);

MODULE_DESCRIPTION("OMAP Display Lighting driver");
MODULE_AUTHOR("Dan Murphy <dmurphy@ti.com");
MODULE_LICENSE("GPL");
