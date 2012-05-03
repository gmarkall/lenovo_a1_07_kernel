/* linux/arch/arm/mach-omap2/board-zoom2-wifi.c
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>

#include <asm/gpio.h>
#include <asm/io.h>
	
#include <linux/wifi_tiwlan.h>

#define GPIO_WL_RST_EN 158

static int evt_wifi_cd;		/* WIFI virtual 'card detect' status */
static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

int omap_wifi_status_register(void (*callback)(int card_present, void *dev_id), void *dev_id)
{
	printk(">>> omap_wifi_status_register\n");
	
	if (wifi_status_cb)
		return -EAGAIN;
	
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	
	printk("<<< omap_wifi_status_register\n");
	return 0;
}

int omap_wifi_status(int irq)
{
	printk("irq: %d, evt_wifi_cd: %d\n", irq, evt_wifi_cd);
	return evt_wifi_cd;
}

int evt_wifi_set_carddetect(int val)
{
	printk(">>> evt_wifi_set_carddetect, val: %d\n", val);
	
	evt_wifi_cd = val;
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		printk("%s: Nobody to notify\n", __func__);
	
	printk("<<< evt_wifi_set_carddetect\n");
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(evt_wifi_set_carddetect);
#endif

static int evt_wifi_power_state;

int evt_wifi_power(int on)
{
	 pr_info("%s: %d\n", __func__, on);
	printk(">>> evt_wifi_power, on: %d\n", on);
        gpio_direction_output(GPIO_WL_RST_EN, on);
        msleep(300);

	evt_wifi_power_state = on;
	
	printk("<<< evt_wifi_power\n");
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(evt_wifi_power);
#endif

static int evt_wifi_reset_state;
int evt_wifi_reset(int on)
{
	pr_info("%s: %d\n", __func__, on);
	printk(">>> evt_wifi_reset, on: %d\n", on);
	evt_wifi_reset_state = on;
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(evt_wifi_reset);
#endif

struct wifi_platform_data evt_wifi_control = {
	.set_power	= evt_wifi_power,
	.set_reset	= evt_wifi_reset,
	.set_carddetect	= evt_wifi_set_carddetect,
};

#ifdef CONFIG_WIFI_CONTROL_FUNC

static struct platform_device evt_wifi_device = {
	.name		= "device_wifi",
	.id		= 1,
	.dev		= {
		.platform_data = &evt_wifi_control,
	},
};
#endif

static int __init evt_wifi_init(void)
{
	int ret;

	pr_info("%s: start\n", __func__);
	printk(">>> evt_wifi_init\n");

        ret = gpio_request(GPIO_WL_RST_EN, "wl_rst_en");
        if (ret < 0) {
            printk("reserving GPIO for wl_rst_en failed\n");
        }
        // Off initially
        gpio_direction_output(GPIO_WL_RST_EN, 0);
        msleep(300);

#ifdef CONFIG_WIFI_CONTROL_FUNC
	ret = platform_device_register(&evt_wifi_device);
#endif
	
	printk("<<< evt_wifi_init\n");
	return ret;
}

device_initcall(evt_wifi_init);
