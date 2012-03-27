/* arch/arm/mach-omap2/board-a1_07-btn-led.c
 *
 * Author: Markinus
 * Author: Parad0X
 * Author: milaq
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/gpio_event.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/keyreset.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>

#define A1_07_DEFAULT_KEYPAD_BRIGHTNESS 0
#define A1_07_GPIO_KP_LED 62
static DEFINE_MUTEX(a1_07_keypad_brightness_lock);

struct led_data {
	struct mutex led_data_mutex;
	struct work_struct brightness_work;
	spinlock_t brightness_lock;
	enum led_brightness brightness;
} keypad_led_data;

static struct gpio_event_platform_data a1_07_input_data = {
	.name		= "keyboard-backlight",	
};

static struct platform_device a1_07_input_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &a1_07_input_data,
	},
};

static void keypad_led_brightness_set_work(struct work_struct *work)
{

	unsigned long flags;

	enum led_brightness brightness;
	uint8_t value;

	spin_lock_irqsave(&keypad_led_data.brightness_lock, flags);
	brightness = keypad_led_data.brightness;
	spin_unlock_irqrestore(&keypad_led_data.brightness_lock, flags);

	value = brightness >= 1 ? 1 : 0;
	gpio_set_value(A1_07_GPIO_KP_LED, value);

}

static void keypad_led_brightness_set(struct led_classdev *led_cdev,
			       enum led_brightness brightness)
{
	unsigned long flags;
	mutex_lock(&a1_07_keypad_brightness_lock);

	pr_debug("Setting %s brightness current %d new %d\n",
			led_cdev->name, led_cdev->brightness, brightness);

	if (brightness > 255)
		brightness = 255;
	led_cdev->brightness = brightness;

	spin_lock_irqsave(&keypad_led_data.brightness_lock, flags);
	keypad_led_data.brightness = brightness;
	spin_unlock_irqrestore(&keypad_led_data.brightness_lock, flags);

	schedule_work(&keypad_led_data.brightness_work);
	mutex_unlock(&a1_07_keypad_brightness_lock);
}

static enum led_brightness keypad_led_brightness_get(struct led_classdev *led_cdev)
{
	return led_cdev->brightness;
}

static struct led_classdev a1_07_backlight_led = 
{
	.name = "button-backlight",
	.brightness = A1_07_DEFAULT_KEYPAD_BRIGHTNESS,
	.brightness_set = keypad_led_brightness_set,
	.brightness_get = keypad_led_brightness_get,
};

static int __init a1_07_init_keypad(void)
{
	int ret;

	ret = platform_device_register(&a1_07_input_device);
	if (ret != 0)
		goto exit;

	ret = gpio_request(A1_07_GPIO_KP_LED, "keypad_led");
	if (ret < 0) {
		pr_err("failed on request gpio keypad backlight on\n");
		goto exit;
	}

	ret = gpio_direction_output(A1_07_GPIO_KP_LED, 0);
	if (ret < 0) {
		pr_err("failed on gpio_direction_output keypad backlight on\n");
		goto err_gpio_kpl;
	}

	mutex_init(&keypad_led_data.led_data_mutex);
	INIT_WORK(&keypad_led_data.brightness_work, keypad_led_brightness_set_work);
	spin_lock_init(&keypad_led_data.brightness_lock);
	ret = led_classdev_register(&a1_07_input_device.dev, &a1_07_backlight_led);
	if (ret) {
		goto exit;
	}

	return 0;

err_gpio_kpl:
	gpio_free(A1_07_GPIO_KP_LED);
exit:
	return ret;

}

device_initcall(a1_07_init_keypad);

