/*
 * HannStar HSD100PXN1 LDVS panel support
 *
 * Copyright (C) 2010 HON HAI Technology Group
 * Author: JJ
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
//#include <linux/i2c/twl.h>
//#include <linux/spi/spi.h>

#include <plat/gpio.h>
#include <plat/mux.h>
#include <asm/mach-types.h>
#include <plat/control.h>

#include <plat/display.h>

#define LCD_XRES		1024
#define LCD_YRES		768

#define LCD_PIXCLOCK_MIN      	55000 /* Min. PIX Clock is 55MHz */
#define LCD_PIXCLOCK_TYP      	65000 /* Typ. PIX Clock is 65MHz */
#define LCD_PIXCLOCK_MAX      	75000 /* Max. PIX Clock is 75MHz */

/* Current Pixel clock */
#define LCD_PIXEL_CLOCK		LCD_PIXCLOCK_TYP

#define EDP_LCD_LVDS_SHTDN_GPIO 	37

/*HannStar HSD100PXN1 Manual
 * defines HFB, HSW, HBP, VFP, VSW, VBP as shown below
 */
/*
========== Detailed Timing Parameters ==========
Timming Name			= 1024x768@60Hz;
Hor Pixels				= 1024;				//pixels
Ver Pixels				= 768;				//Lines
Hor Frequency 		= 48.363;			//kHz = 20.7 usec/line
Ver Frequency 		= 60.004;			//Hz	= 16.7 msec/frame
Pixel Clock				= 65.000;			//MHz	=	15.4 nsec	กำ0.5%
Character Width 	= 8;					//Pixel	=	123.1nsec
Scan Type					= NONINTERLACED;	//H Phase = 5.1%
Hor Sync Polarity	= NEGATIVE;		//HBlank = 23.8% of HTotal
Ver Sync Polarity	= NEGATIVE;		//VBlank =  4.7% of VTotal
Hor Total Time 		= 20.677;			//(usec)	=	168 chars	=	1344 Pixels
Hor Addr Time 		= 15.754;			//(usec)	=	128 chars	=	1024 Pixels
Hor Blank Start		= 15.754;			//(usec)	=	128 chars	=	1024 Pixels
Hor Blank Time 		= 4.923;			//(usec)	=	 40 chars	=	 320 Pixels
Hor Sync Start		=	16.123			//(usec)	=	131 chars	=	1048 Pixels
H Right Border		=	0.000;			//(usec)	=		0 chars = 	 0 Pixels
H Front Porch			= 0.369;			//(usec)	=		3 chars =		24 Pixels
Hor Sync Time			=	2.092;			//(usec)	=	 17 chars =  136 Pixels
H Back Porch 			= 2.462;			//(usec)	=	 20	chars =  160 Pixels
H Left Border			= 0.000;			//(usec)	=	  0 chars = 	 0 Pixels
Ver Total Time		= 16.666;			//(msec)	=	806 lines			HT-(1.06xHA)=3.98
Ver Addr Time			=	15.880;			//(msec)	=	768 lines
Ver Blank Start		= 15.880;			//(msec)	=	768 lines
Ver Blank Time		= 0.786;			//(msec)	=	 38 lines
Ver Sync Start		= 15.942;			//(msec)	=	771 lines
V Bottom Border		= 0.000;			//(msec)	=		0 lines
V Front Porch			= 0.062;			//(msec)	= 	3 lines
Ver Sync Time			= 0.124;			//(msec)	=		6 lines
V Back Porch 			= 0.600;			//(msec)	=	 29 lines
V Top Border			=	0.000;			//(msec)	=		0 lines
*/
static struct omap_video_timings hannstar_panel_timings = {
	/* 1024 x 768 @ 60 Hz   */
	.x_res          = LCD_XRES,
	.y_res          = LCD_YRES,
	.pixel_clock    = LCD_PIXEL_CLOCK,
	.hfp            = 24,
	.hsw            = 136,
	.hbp            = 160,
	.vfp            = 3,
	.vsw            = 6,
	.vbp            = 29,
};

static int hannstar_panel_probe(struct omap_dss_device *dssdev)
{
	printk("[CES-demo]:hannstar_panel_probe()++++++\n");
	
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IHS ;
	//dssdev->panel.config = OMAP_DSS_LCD_TFT /*| OMAP_DSS_LCD_IPC | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IHS*/ ;
			
	dssdev->panel.timings = hannstar_panel_timings;

	dssdev->ctrl.pixel_size = 16;
	
	printk("[CES-demo]:hannstar_panel_probe()------\n");
	
	return 0;
}

static void hannstar_panel_remove(struct omap_dss_device *dssdev)
{
}

static int hannstar_panel_enable(struct omap_dss_device *dssdev)
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

static void hannstar_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
	 gpio_direction_output(EDP_LCD_LVDS_SHTDN_GPIO, 0);

	/* Delay recommended by panel DATASHEET */
	mdelay(4);

	omapdss_dpi_display_disable(dssdev);
}

static int hannstar_panel_suspend(struct omap_dss_device *dssdev)
{
	hannstar_panel_disable(dssdev);
	return 0;
}

static int hannstar_panel_resume(struct omap_dss_device *dssdev)
{
	return hannstar_panel_enable(dssdev);
}

static struct omap_dss_driver hannstar_driver = {
	.probe		= hannstar_panel_probe,
	.remove		= hannstar_panel_remove,

	.enable		= hannstar_panel_enable,
	.disable	= hannstar_panel_disable,
	.suspend	= hannstar_panel_suspend,
	.resume		= hannstar_panel_resume,

	.driver         = {
		.name   = "hannstar_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init hannstar_panel_drv_init(void)
{
	return omap_dss_register_driver(&hannstar_driver);
}

static void __exit hannstar_panel_drv_exit(void)
{
	omap_dss_unregister_driver(&hannstar_driver);
}

module_init(hannstar_panel_drv_init);
module_exit(hannstar_panel_drv_exit);
MODULE_LICENSE("GPL");
