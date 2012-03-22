/*
 * AT070TNA2 LVDS panel support
 *
 * Copyright (C) 2011 Foxconn Technology Group
 * Author: James
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <plat/gpio.h>
#include <plat/mux.h>
#include <asm/mach-types.h>
#include <plat/control.h>

#include <plat/display.h>

#define LCD_XRES		1024
#define LCD_YRES		600

#define LCD_PIXCLOCK_MIN      	40800 /* Min. PIX Clock is 40.8MHz */
#define LCD_PIXCLOCK_TYP      	43200 /* Typ. PIX Clock is 51.2MHz */
#define LCD_PIXCLOCK_MAX      	67200 /* Max. PIX Clock is 67.2MHz */

#define LCD_PIXEL_CLOCK		LCD_PIXCLOCK_TYP

#define EDP_LCD_LVDS_SHTDN_GPIO 	37

//following the timing setting from TI's patch
static struct omap_video_timings at070tna2_panel_timings = {
	/* 1024 x 600 @ 60 Hz   */
	.x_res          = LCD_XRES,
	.y_res          = LCD_YRES,
	.pixel_clock    = LCD_PIXEL_CLOCK,
    .hfp            = 5,
    .hbp            = 90,
    .hsw            = 10,
    
    .vfp            = 3,
    .vbp            = 6,
    .vsw            = 3,
};

static int at070tna2_panel_probe(struct omap_dss_device *dssdev)
{
	printk("----------[CL1-EVT1]: start of %s-----------\n", __func__);
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IHS ;
	dssdev->panel.timings = at070tna2_panel_timings;
	
	dssdev->ctrl.pixel_size = 24;
	printk("----------[CL1-EVT1]: end of %s-----------\n", __func__);
	return 0;
}

static void at070tna2_panel_remove(struct omap_dss_device *dssdev)
{
}

static int at070tna2_panel_enable(struct omap_dss_device *dssdev)
{
	 int r = 0;
	 gpio_request(EDP_LCD_LVDS_SHTDN_GPIO, "lcd LVDS SHTN");

	 gpio_direction_output(EDP_LCD_LVDS_SHTDN_GPIO, 0);
	 msleep(10);
	 gpio_direction_output(EDP_LCD_LVDS_SHTDN_GPIO, 1);	
	 msleep(100);
	 gpio_direction_output(EDP_LCD_LVDS_SHTDN_GPIO, 0);
	 msleep(10);
	gpio_direction_output(EDP_LCD_LVDS_SHTDN_GPIO, 1);
	
	r = omapdss_dpi_display_enable(dssdev);
	if (r)
		goto err0;

	/* Delay recommended by panel DATASHEET */
	mdelay(4);
	if (dssdev->platform_enable) {
		r = dssdev->platform_enable(dssdev);
		if (r)
			goto err1;
	}
	return r;
err1:
	omapdss_dpi_display_disable(dssdev);
err0:
	return r;
}

static void at070tna2_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
	 gpio_direction_output(EDP_LCD_LVDS_SHTDN_GPIO, 0);
	 
	/* Delay recommended by panel DATASHEET */
	mdelay(4);

	omapdss_dpi_display_disable(dssdev);
}

static int at070tna2_panel_suspend(struct omap_dss_device *dssdev)
{
	at070tna2_panel_disable(dssdev);
	return 0;
}

static int at070tna2_panel_resume(struct omap_dss_device *dssdev)
{
	return at070tna2_panel_enable(dssdev);
}

static struct omap_dss_driver at070tna2_driver = {
	.probe		= at070tna2_panel_probe,
	.remove		= at070tna2_panel_remove,

	.enable		= at070tna2_panel_enable,
	.disable	= at070tna2_panel_disable,
	.suspend	= at070tna2_panel_suspend,
	.resume		= at070tna2_panel_resume,

	.driver         = {
		.name   = "at070tna2_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init at070tna2_panel_drv_init(void)
{
	return omap_dss_register_driver(&at070tna2_driver);
}

static void __exit at070tna2_panel_drv_exit(void)
{
	omap_dss_unregister_driver(&at070tna2_driver);
}

module_init(at070tna2_panel_drv_init);
module_exit(at070tna2_panel_drv_exit);
MODULE_LICENSE("GPL");
