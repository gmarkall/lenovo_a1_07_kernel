
/*
 * Copyright (C) 2011 Texas Instruments Inc.
 *
 * Modified from mach-omap2/board-zoom2.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/regulator/machine.h>
//??#include <linux/leds-omap-display.h>
#include <linux/leds.h>
#include <linux/leds-omap-display.h>
#include <linux/leds.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <plat/display.h>
#include <plat/mcspi.h>
#include <plat/omap-pm.h>
#include <plat/board.h>

#include "mux.h"
#include <linux/share_region.h>


#define LCD_PANEL_ENABLE_GPIO		36
#define LCD_CABC0_GPIO                  44
#define LCD_CABC1_GPIO                  45
#define LCD_BACKLIGHT_EN_EVT2           47
//add by James
#define DISPC_SZ_REGS		SZ_1K

#define RED_MASK        0x00FF0000
#define GREEN_MASK      0x0000FF00
#define BLUE_MASK       0x000000FF
#define RED_SHIFT       16
#define GREEN_SHIFT     8
#define BLUE_SHIFT      0
#define MAX_COLOR_DEPTH 255

#define DEFAULT_BACKLIGHT_BRIGHTNESS    75

//&*&*&*JJ1_20101217 for 10" Hannstar panel for CES demo
#if defined(CONFIG_PANEL_HANNSTAR) || defined(CONFIG_PANEL_LG070)
#define EDP_LCD_PANEL_BACKLIGHT_GPIO 	38
//#define EDP_LCD_PANEL_ADJ_PWM_GPIO 	58 //201100504, JimmySu mark for PWM
/*++++20110404, Jimmy Su add for EVT2 LCD*/
/*++++20110404, Jimmy Su add for EVT2 LCD*/
#define EDP_LCD_PWR_EN_GPIO                  47
#define EDP_LVDS_CLKSEL_GPIO                  90
#else
/*<--LH_SWRD_CL1_James 2011_04_19 CL1 LCD gpio*/
#define EDP_LCD_PANEL_BACKLIGHT_GPIO 	38
#define EDP_LCD_PANEL_RESET_GPIO 		48
#define EDP_LCD_PWR_EN_GPIO			    61
#define EDP_LCD_STBYB					49
#define EDP_LCD_RST						48
#define EDP_LCD_VGH_CTRL				159
#define EDP_LCD_ENABLE					110
#define EDP_LCD_ADJ						58
/*CL1 LCD gpio-->*/
/*-----20110404, Jimmy Su add for EVT2 LCD*/
#endif
/*-----20110404, Jimmy Su add for EVT2 LCD*/


extern unsigned get_last_off_on_transaction_id(struct device *dev);

static void dump_per(void){
        printk("\n\
        CM_FCLKEN_PER:   0x%.8X\n\
        CM_ICLKEN_PER:   0x%.8X\n\
        CM_IDLEST_PER:   0x%.8X\n\
        CM_AUTOIDLE_PER: 0x%.8X\n\
        CM_CLKSEL_PER:   0x%.8X\n\
        CM_SLEEPDEP_PER: 0x%.8X\n\
        CM_CLKSTCTRL_PER:0x%.8X\n\
        CM_CLKSTST_PER:  0x%.8X\n\n" , 
        *(u32*)0xfa005000,
        *(u32*)0xfa005010,
        *(u32*)0xfa005020,
        *(u32*)0xfa005030,
        *(u32*)0xfa005040,
        *(u32*)0xfa005044,
        *(u32*)0xfa005048,
        *(u32*)0xfa00504C);
}

/*---backlight--------------------------------------------------------------------*/
static void boxer_backlight_set_power(struct omap_pwm_led_platform_data *self, int on_off)
{
	gpio_direction_output(LCD_BACKLIGHT_EN_EVT2, !on_off);
	gpio_set_value(LCD_BACKLIGHT_EN_EVT2, !on_off);
}

static struct omap_pwm_led_platform_data boxer_backlight_data = {
	.name = "lcd-backlight",
	.intensity_timer = 8,
	.def_on = 0,   // PWM high == backlight OFF, PWM low == backlight ON
	.def_brightness = DEFAULT_BACKLIGHT_BRIGHTNESS,
	.set_power = boxer_backlight_set_power,
};

static struct platform_device boxer_backlight_led_device = {
	.name		= "omap_pwm_led",
	.id		= -1,
	.dev		= {
		.platform_data = &boxer_backlight_data,
	},
};

static void __init boxer_backlight_init(void)
{
        printk("Enabling backlight PWM for LCD\n");
        boxer_backlight_data.def_on = 0; // change the PWM polarity

        gpio_request(LCD_BACKLIGHT_EN_EVT2, "lcd backlight evt2");

        gpio_request(LCD_CABC0_GPIO, "lcd CABC0");
        gpio_direction_output(LCD_CABC0_GPIO,0);
        gpio_set_value(LCD_CABC0_GPIO,0);

        gpio_request(LCD_CABC1_GPIO, "lcd CABC1");
        gpio_direction_output(LCD_CABC1_GPIO,0);
        gpio_set_value(LCD_CABC1_GPIO,0);
        
        boxer_backlight_set_power(&boxer_backlight_data, 0);
}


/*--------------------------------------------------------------------------*/
#if 0
u8 color_component_correction[256]=
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
15,15,15,15,15,15,15,10,10,10,10,10,10,9,8,7,
6,5,4,4,2,2,2,2,2,2,1,1,1,1,0,0,
};

static int evt_clut_fill(void * ptr, u32 size)
{
	u16 count;
	u32 temp;
	u32 *byte = (u32 *)ptr;
	u16 color_corrected_value;
	u8 red, green, blue;
	for (count = 0; count < size / sizeof(u32); count++) {
		red   = count;
		green = count;
		blue  = count;
		temp = (((red << RED_SHIFT) & RED_MASK) | ((green << GREEN_SHIFT) & GREEN_MASK) | ((blue << BLUE_SHIFT) & BLUE_MASK)); 
		*byte++ = temp;
	}
	/* Roll back the pointer*/
	byte = (u32 *)ptr;

	for (count = 0; count < size / sizeof(u32); count++) {
		blue = *byte & BLUE_MASK;
		color_corrected_value = color_component_correction[count]+blue;
		color_corrected_value = (color_corrected_value >= MAX_COLOR_DEPTH) ? MAX_COLOR_DEPTH:color_corrected_value; 
		temp = ((*byte & (~BLUE_MASK)) | (color_corrected_value & BLUE_MASK));
		*byte++ = temp;
	}
	return 0;
}
#endif

/*--------------------------------------------------------------------------*/
static int edp1_panel_enable_lcd(struct omap_dss_device *dssdev)
{
#if 0
	gpio_direction_output(EDP_LCD_PANEL_BACKLIGHT_GPIO, 1);
//	gpio_direction_output(EDP_LCD_PANEL_ADJ_PWM_GPIO, 1); //20110224, JimmySu mark for PWM
/*++++20110404, Jimmy Su add for EVT2 LCD*/
	gpio_direction_output(EDP_LCD_PWR_EN_GPIO, 1);
//	gpio_direction_output(EDP_LVDS_CLKSEL_GPIO, 1);
/*-----20110404, Jimmy Su add for EVT2 LCD*/
printk("Jimmy(board-edp1-panel.c)edp1_panel_resume\n");	
#endif
	gpio_direction_output(EDP_LCD_STBYB, 0);
		
	gpio_direction_output(EDP_LCD_ENABLE, 1);//for vdd
	mdelay(50);
	gpio_direction_output(EDP_LCD_PANEL_RESET_GPIO, 0);
	mdelay(20);
	gpio_direction_output(EDP_LCD_PWR_EN_GPIO, 1);//for avdd
	mdelay(10);
	gpio_direction_output(EDP_LCD_VGH_CTRL, 1);
	mdelay(100);
	gpio_direction_output(EDP_LCD_PANEL_BACKLIGHT_GPIO, 1);
	
	printk("James(board-edp1-panel.c)edp1_panel_resume\n");
	
	void __iomem  *dispc_base =  ioremap(0x48050400, DISPC_SZ_REGS);
	printk("@@@@@@@@@@@DISPC_GFX_BA0 is 0x%08x ...\n", __raw_readl(dispc_base + 0x80));
	printk("@@@@@@@@@@@DISPC_GFX_POSITION X is %d ...\n", __raw_readl(dispc_base + 0x88) & 0x03ff);
	printk("@@@@@@@@@@@DISPC_GFX_POSITION Y is %d ...\n", (__raw_readl(dispc_base + 0x88) >> 16));
// add a constraint to run the L3@200Mhz while the LCD is ON for UI speed
	omap_pm_set_min_bus_tput(&(dssdev->dev), OCP_INITIATOR_AGENT , 200000 * 4);
	return 0;
}

static void edp1_panel_disable_lcd(struct omap_dss_device *dssdev)
{
//#ifndef CONFIG_PANEL_MIRASOL
#if 0
	gpio_direction_output(EDP_LCD_PANEL_BACKLIGHT_GPIO, 0);
//	gpio_direction_output(EDP_LCD_PANEL_ADJ_PWM_GPIO, 0); //20110224, JimmySu mark for PWM
/*++++20110404, Jimmy Su add for EVT2 LCD*/
	gpio_direction_output(EDP_LCD_PWR_EN_GPIO, 0);
//	gpio_direction_output(EDP_LVDS_CLKSEL_GPIO, 0);
/*-----20110404, Jimmy Su add for EVT2 LCD*/
printk("Jimmy(board-edp1-panel.c)edp1_panel_suspend\n");	
#endif

#ifdef CONFIG_PANEL_AT070TNA2
	printk("**************power off sequence\n");
	gpio_direction_output(EDP_LCD_PANEL_BACKLIGHT_GPIO, 0);
	mdelay(30);
	gpio_direction_output(EDP_LCD_STBYB, 1);
	mdelay(30);
	gpio_direction_output(EDP_LCD_VGH_CTRL, 0);
	mdelay(30);
 	gpio_direction_output(EDP_LCD_PWR_EN_GPIO, 0);
	gpio_direction_output(EDP_LCD_PANEL_RESET_GPIO, 1);
	mdelay(20);
	gpio_direction_output(EDP_LCD_ENABLE, 0);//for vdd
	printk("James(board-edp1-panel.c)edp1_panel_suspend\n");	
#endif
// remove the previous constraint on L3 bus speed in case LCD is OFF
	omap_pm_set_min_bus_tput(&(dssdev->dev), OCP_INITIATOR_AGENT ,0);
}

#ifdef CONFIG_PANEL_HANNSTAR
static struct omap_dss_device evt_lcd_device = {
	.name = "lcd",
	.driver_name = "hannstar_panel",
	.type = OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines = 18,
//&*&*&*BC1_110603: fix the issue that android software keyboard can not show 	
	.panel                  = {
                .width_in_mm = 204,
                .height_in_mm = 152,
        },	
//&*&*&*BC2_110603: fix the issue that android software keyboard can not show	.phy.dpi.data_lines = 18,
	.platform_enable = edp1_panel_enable_lcd,
	.platform_disable = edp1_panel_disable_lcd,
};

/*<--LH_SWRD_CL1_James 2011_04_19 CL1 lcd devices struct*/
#elif CONFIG_PANEL_AT070TNA2
static struct omap_dss_device evt_lcd_device = {
	.name = "lcd",
	.driver_name = "at070tna2_panel",
	.type = OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines = 24,
	//.resume = edp1_panel_resume,
	//.suspend = edp1_panel_suspend,
	.platform_enable = edp1_panel_enable_lcd,
	.platform_disable = edp1_panel_disable_lcd,
	.panel = {
                .width_in_mm = 204,
                .height_in_mm = 152,
        },	
};
/* CL1 lcd devices struct-->*/

#else
static struct omap_dss_device evt_lcd_device = {
	.name = "lcd",
	.driver_name = "boxer_panel",
	.type = OMAP_DISPLAY_TYPE_DPI,
	.channel = OMAP_DSS_CHANNEL_LCD,
	.phy.dpi.data_lines = 24,
};
#endif //CONFIG_PANEL_HANNSTAR

static struct omap_dss_device *evt_dss_devices[] = {
	&evt_lcd_device,
};

static struct omap_dss_board_info evt_dss_data = {
/* FIXME */
//??	.get_last_off_on_transaction_id = get_last_off_on_transaction_id,
	.num_devices = ARRAY_SIZE(evt_dss_devices),
	.devices = evt_dss_devices,
	.default_device = &evt_lcd_device,
};

static struct platform_device evt_dss_device = {
	.name          = "omapdss",
	.id            = -1,
	.dev            = {
		.platform_data = &evt_dss_data,
	},
};

/*--------------------------------------------------------------------------*/
static struct omap2_mcspi_device_config evt_lcd_mcspi_config = {
	.turbo_mode             = 0,
	.single_channel         = 1,  /* 0: slave, 1: master */

};

struct spi_board_info evt_spi_board_info[] __initdata = {
	[0] = {
		.modalias               = "boxer_disp_spi",
		.bus_num                = 4,
		.chip_select            = 0,
		.max_speed_hz           = 375000,
		.controller_data        = &evt_lcd_mcspi_config,
	},
};

/*--------------------------------------------------------------------------*/
static struct platform_device *evt_panel_devices[] __initdata = {
	//&evt_dss_device,
	&boxer_backlight_led_device,
};

void __init evt_lcd_panel_init(void)
{
	#ifdef CONFIG_PANEL_HANNSTAR
	int board_id;

	board_id = ep_get_hardware_id();	
	switch(board_id)
	{ 
		case BOARD_ID_EVT2: 
			EDP_LCD_PWR_EN_GPIO = 47;
			break;
		case BOARD_ID_EVT3: 
			EDP_LCD_PWR_EN_GPIO = 88;
			break;
		default:
			EDP_LCD_PWR_EN_GPIO = 47;
			break;
	}		
		omap_display_init(&evt_dss_data);
/*++++ 20110224, JimmySu modify for PWM */
	printk("[CES-demo]:gpio_request()++++++\n");
	gpio_request(EDP_LCD_PANEL_BACKLIGHT_GPIO, "lcd backlight");
//    gpio_request(EDP_LCD_PANEL_ADJ_PWM_GPIO, "lcd adj PWM");
/*++++20110404, Jimmy Su add for EVT2 LCD*/
	gpio_request(EDP_LCD_PWR_EN_GPIO, "LCD_PWR_EN");
//	gpio_request(EDP_LVDS_CLKSEL_GPIO, "lcd LVDS_CLKSEL_GPIO");	//&*&*&*BC_110525: disable gpio 90 to fix display flicker for EVT2
/*++++20110404, Jimmy Su add for EVT2 LCD*/
	gpio_direction_output(EDP_LCD_PANEL_BACKLIGHT_GPIO, 1);
//    gpio_direction_output(EDP_LCD_PANEL_ADJ_PWM_GPIO, 1);
/*++++20110404, Jimmy Su add for EVT2 LCD*/
	gpio_direction_output(EDP_LCD_PWR_EN_GPIO, 1);
//	gpio_direction_output(EDP_LVDS_CLKSEL_GPIO, 1);	//&*&*&*BC_110525: disable gpio 90 to fix display flicker for EVT2
/*++++20110404, Jimmy Su add for EVT2 LCD*/
	printk("[CES-demo]:gpio_request()------\n");
/*---- 20110224, JimmySu modify for PWM */
	#else
/*<--LH_SWRD_CL1_James 2011_04_19 CL1 power on sequence*/
 	int r0, r1, r2, r3;
	//gpio_request(EDP_LCD_PANEL_RESET_GPIO, "lcd reset");
/*  #define EDP_LCD_PANEL_BACKLIGHT_GPIO 	38
	#define EDP_LCD_PANEL_RESET_GPIO 		48
	#define EDP_LCD_PWR_EN_GPIO			    61
	#define EDP_LCD_STBYB					49
	#define EDP_LCD_PANEL_RESET_GPIO						48
	#define EDP_LCD_VGH_CTRL				159 */
	omap_display_init(&evt_dss_data);
	gpio_request(EDP_LCD_PANEL_BACKLIGHT_GPIO, "lcd backlight");
	gpio_request(EDP_LCD_PWR_EN_GPIO, "lcd_panel_pwren");
	gpio_request(EDP_LCD_VGH_CTRL, "lcd_panel_vgh_ctrl");
	gpio_request(EDP_LCD_STBYB, "lcd_panel_stbyb");
	gpio_request(EDP_LCD_PANEL_RESET_GPIO, "lcd_panel_reset");
	
	//delete the power off sequence for logo display
/*  	r0 = gpio_direction_output(EDP_LCD_PWR_EN_GPIO, 0);
	r1 = gpio_direction_output(EDP_LCD_VGH_CTRL, 0);
	r2=gpio_direction_output(EDP_LCD_PANEL_BACKLIGHT_GPIO, 0);
	gpio_direction_output(EDP_LCD_PANEL_RESET_GPIO, 1);
	gpio_direction_output(EDP_LCD_ENABLE, 0);//for vdd
	gpio_direction_output(EDP_LCD_STBYB, 1); */ 

	mdelay(1000); 
	
 	mdelay(200);
	r2=gpio_direction_output(EDP_LCD_PANEL_BACKLIGHT_GPIO, 0);
//power on  sequence	
 	gpio_direction_output(EDP_LCD_STBYB, 0);
		
	gpio_direction_output(EDP_LCD_ENABLE, 1);//for vdd
	mdelay(50);
	gpio_direction_output(EDP_LCD_PANEL_RESET_GPIO, 0);
	mdelay(20);
	r0 = gpio_direction_output(EDP_LCD_PWR_EN_GPIO, 1);//for avdd
	mdelay(10);
	r1 = gpio_direction_output(EDP_LCD_VGH_CTRL, 1);
	mdelay(100);
	gpio_direction_output(EDP_LCD_ADJ, 1);
//	r2=gpio_direction_output(EDP_LCD_PANEL_BACKLIGHT_GPIO, 1);//delete it for fix the logo display issue
	
	//r3=platform_device_register(&edp1_dss_device);
	printk("[CL1-EVT]:gpio_request()------r0=%d,r1=%d,r2=%d,r3=%d\n",r0,r1,r2,r3);  
/*CL1 power on sequence-->*/
	
	#endif //CONFIG_PANEL_HANNSTAR
}
