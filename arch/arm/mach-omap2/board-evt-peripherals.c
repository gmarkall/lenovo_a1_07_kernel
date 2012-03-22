
/*
 * Copyright (C) 2009 Texas Instruments Inc.
 *
 * Modified from mach-omap2/board-zoom2.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/switch.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/mmc/host.h>
#include <linux/i2c/twl.h>
//#include <linux/input/cyttsp.h>
//#include <linux/kxtf9.h>
//#include <linux/max17042.h>
#include <linux/interrupt.h>
#include <linux/regulator/fixed.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/input/matrix_keypad.h>
#include <linux/usb/android_composite.h>

#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/usb.h>
#include <plat/mux.h>
#include <plat/common.h>
#include <plat/dmtimer.h>
#include <plat/control.h>

#ifdef CONFIG_SERIAL_OMAP
#include <plat/serial.h>
#include <plat/omap-serial.h>
#endif

//&*&*&*SJ1_20110607
#if defined (CONFIG_ANDROID_FACTORY_DEFAULT_REBOOT)
#include "android_factory_default.h"
#endif
//&*&*&*SJ2_20110607
//&*&*&*SJ1_20110419, Add /proc file to display software and hardware version.
#if defined (CONFIG_PROCFS_DISPLAY_SW_HW_VERSION)
#include <linux/share_region.h>
#endif
//&*&*&*SJ2_20110419, Add /proc file to display software and hardware version.

/***++++20110115, jimmySu add backlight driver for PWM***/
#ifdef CONFIG_LEDS_OMAP_DISPLAY
#include <plat/common.h>
#include <plat/cpu.h>
#include <plat/clockdomain.h>
#include <plat/powerdomain.h>
#include <plat/clock.h>
#include <plat/omap_hwmod.h>
#include "cm.h"
#include "cm-regbits-34xx.h"
#include "prcm-common.h"
#include <linux/switch.h>
#include <linux/leds-omap-display.h>
#include <linux/leds.h>

unsigned long gptimer8;
//&*&*&*HC1_20110826, modify pwm init sequence
struct clk *gptimer8_ick = NULL;
struct clk *gptimer8_fck = NULL;
//&*&*&*HC2_20110826, modify pwm init sequence
#define EDP_LCD_PANEL_ADJ_PWM_GPIO 	58
#endif // CONFIG_LEDS_OMAP_DISPLAY
/***----20110115, jimmySu add backlight driver for PWM***/

//&*&*&*JJ1 touch driver
#if defined(CONFIG_TOUCHSCREEN_CYPRESS_TTSP) || defined(CONFIG_TOUCHSCREEN_CYPRESS_7inch)
#include <linux/cyttsp.h>
#endif /* CONFIG_TOUCHSCREEN_CYPRESS_TTSP */
#if defined(CONFIG_TOUCHSCREEN_ITE_IT7260)
#include <linux/it7260.h>
#endif /* CONFIG_TOUCHSCREEN_ITE_IT7260 */
//&*&*&*JJ2 touch driver
//&*&*&*AL1 g-sensor driver 20101217
#if defined(CONFIG_ADXL345_SENSOR_MODULE) || defined(CONFIG_ADXL345_GSENSOR_MODULE)
#define GPIO_ADXL345_IRQ     100
#endif
//&*&*&*AL2 g-sensor driver 20101217

#include "mux.h"
#include "hsmmc.h"
#include <mach/board-encore.h>

#if defined(CONFIG_BATTERY_MAX17043)
#include <linux/max17043_battery.h>
#endif
#if defined(CONFIG_BATTERY_BQ27541)
	#include <linux/bq27541_battery.h>
#endif
#if defined(CONFIG_REGULATOR_MAX8903)
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/max8903.h>
#endif
#if defined(CONFIG_LEDS_EP10)
#include <linux/leds-ep10.h>
#endif
#if defined(CONFIG_TWL4030_MADC_BATTERY)
#include <linux/twl4030_madc_battery.h>
#endif
#if defined(CONFIG_LEDS_BUTTON_EP10)
#include <linux/leds-ep10.h>
#endif
/*<-- LH_SWRD_CL1_Richard@2011.5.11 morgan touch for cl1 */
#if defined(CONFIG_TOUCHSCREEN_MG_I2C)
#define MORGAN_IRQ_GPIO  14
#endif
/*LH_SWRD_CL1_Richard@2011.5.11 morgan touch for cl1  -->*/
//&*&*&*BC1_110615: fix the issue that system can not enter off mode
#include "twl4030.h"
//&*&*&*BC2_110615: fix the issue that system can not enter off mode
//&*&*&*JJ1 touch driver
#if defined(CONFIG_TOUCHSCREEN_CYPRESS_TTSP) || defined(CONFIG_TOUCHSCREEN_CYPRESS_7inch) || defined(CONFIG_TOUCHSCREEN_EETI_EXC7200)
#define ZOOM2_TOUCHSCREEN_IRQ_GPIO	14 /*14(EVT2)*//*99(EVT1)*/
#endif /* CONFIG_TOUCHSCREEN_CYPRESS_TTSP */
//&*&*&*JJ2 touch driver

#define KXTF9_DEVICE_ID                 "kxtf9"
#define KXTF9_I2C_SLAVE_ADDRESS         0x0F
#define KXTF9_GPIO_FOR_PWR              34
#define KXTF9_GPIO_FOR_IRQ              113

#define CYTTSP_I2C_SLAVEADDRESS         34
#define OMAP_CYTTSP_GPIO                99
#define OMAP_CYTTSP_RESET_GPIO          46
#define LCD_EN_GPIO                     36

#define MAX17042_GPIO_FOR_IRQ		100
#define BOARD_ENCORE_PROD_3G        	0x8

#define AIC3100_NAME                    "tlv320dac3100"
#define AIC3100_I2CSLAVEADDRESS         0x18
#define AUDIO_CODEC_IRQ_GPIO            59
#define AUDIO_CODEC_POWER_ENABLE_GPIO   103
#define AUDIO_CODEC_RESET_GPIO          37

#define CONFIG_SERIAL_OMAP_IDLE_TIMEOUT 5

extern void evt_lcd_panel_init(void);


static inline int is_encore_3g(void)
{
        return (system_rev & BOARD_ENCORE_PROD_3G);
}

//&*&*&*JJ1
#include <media/v4l2-int-device.h>
#if (defined(CONFIG_VIDEO_MT9D115) || defined(CONFIG_VIDEO_MT9D115_MODULE))
#include <media/mt9d115.h>
extern struct mt9d115_platform_data zoom2_mt9d115_platform_data;
#endif
//&*&*&*JJ1

#if (defined(CONFIG_VIDEO_MT9V115) || defined(CONFIG_VIDEO_MT9V115_MODULE)) && \
    defined(CONFIG_VIDEO_OMAP3)
#include <media/mt9v115.h>
extern struct mt9v115_platform_data zoom2_mt9v115_platform_data;
extern void zoom2_cam_mt9v115_init(void);
#else
#define zoom2_cam_mt9v115_init()	NULL
#endif

#if (defined(CONFIG_VIDEO_S5K5CA) || defined(CONFIG_VIDEO_S5K5CA_MODULE)) && \
    defined(CONFIG_VIDEO_OMAP3)
#include <media/s5k5ca.h>
extern struct s5k5ca_platform_data zoom2_s5k5ca_platform_data;
extern void zoom2_cam_s5k5ca_init(void);
#else
#define zoom2_cam_s5k5ca_init()	NULL
#endif

int  cyttsp_dev_init(int resource)
{
        if (resource)
        {
                if (gpio_request(OMAP_CYTTSP_RESET_GPIO, "tma340_reset") < 0) {
                        printk(KERN_ERR "can't get tma340 xreset GPIO\n");
                        return -1;
                }

                if (gpio_request(OMAP_CYTTSP_GPIO, "cyttsp_touch") < 0) {
                        printk(KERN_ERR "can't get cyttsp interrupt GPIO\n");
                        return -1;
                }

                gpio_direction_input(OMAP_CYTTSP_GPIO);
        }
        else
        {
                gpio_free(OMAP_CYTTSP_GPIO);
                gpio_free(OMAP_CYTTSP_RESET_GPIO);
        }
        
        return 0;
}

// static struct cyttsp_platform_data cyttsp_platform_data = {
//         .maxx = 480,
//         .maxy = 800,
//         .flags = FLIP_DATA_FLAG | REVERSE_Y_FLAG,
//         .gen = CY_GEN3,
//         .use_st = CY_USE_ST,
//         .use_mt = CY_USE_MT,
//         .use_hndshk = CY_SEND_HNDSHK,
//         .use_trk_id = CY_USE_TRACKING_ID,
//         .use_sleep = CY_USE_SLEEP,
//         .use_gestures = CY_USE_GESTURES,
//         /* activate up to 4 groups
//          * and set active distance
//          */
//         .gest_set = CY_GEST_GRP1 | CY_GEST_GRP2 |
//                     CY_GEST_GRP3 | CY_GEST_GRP4 |
// 		    CY_ACT_DIST,
//         /* change act_intrvl to customize the Active power state 
//          * scanning/processing refresh interval for Operating mode
//          */
//         .act_intrvl = CY_ACT_INTRVL_DFLT,
//         /* change tch_tmout to customize the touch timeout for the
//          * Active power state for Operating mode
//          */
//         .tch_tmout = CY_TCH_TMOUT_DFLT,
//         /* change lp_intrvl to customize the Low Power power state 
//          * scanning/processing refresh interval for Operating mode
//          */
//         .lp_intrvl = CY_LP_INTRVL_DFLT,
// };


/************************ KXTF9 device **************************/

static void kxtf9_dev_init(void)
{
	if (gpio_request(KXTF9_GPIO_FOR_IRQ, "kxtf9_irq") < 0) {
		printk(KERN_ERR "Can't get GPIO for kxtf9 IRQ\n");
		return;
	}

	printk("kxtf9_dev_init: Init kxtf9 irq using gpio %d.\n", KXTF9_GPIO_FOR_IRQ);
	gpio_direction_input(KXTF9_GPIO_FOR_IRQ);
}


// struct kxtf9_platform_data kxtf9_platform_data_here = {
// 	.min_interval   = 1,
// 	.poll_interval  = 1000,
// 
// 	.g_range        = KXTF9_G_8G,
// 	.shift_adj      = SHIFT_ADJ_2G,
// 
// 	// Map the axes from the sensor to the device.
// 
// 	//. SETTINGS FOR THE EVT1A:
// 	.axis_map_x     = 1,
// 	.axis_map_y     = 0,
// 	.axis_map_z     = 2,
// 	.negate_x       = 1,
// 	.negate_y       = 0,
// 	.negate_z       = 0,
// 
// 	.data_odr_init          = ODR12_5F,
// 	.ctrl_reg1_init         = KXTF9_G_8G | RES_12BIT | TDTE | WUFE | TPE,
// 	.int_ctrl_init          = KXTF9_IEN  | KXTF9_IEA | KXTF9_IEL,
// 	.int_ctrl_init          = KXTF9_IEN,
// 	.tilt_timer_init        = 0x03,
// 	.engine_odr_init        = OTP12_5 | OWUF50 | OTDT400,
// 	.wuf_timer_init         = 0x16,
// 	.wuf_thresh_init        = 0x28,
// 	.tdt_timer_init         = 0x78,
// 	.tdt_h_thresh_init      = 0xFF,
// 	.tdt_l_thresh_init      = 0x14,
// 	.tdt_tap_timer_init     = 0x53,
// 	.tdt_total_timer_init   = 0x24,
// 	.tdt_latency_timer_init = 0x10,
// 	.tdt_window_timer_init  = 0xA0,
// 
// 	.gpio = KXTF9_GPIO_FOR_IRQ,
// };


/************************ LCD power supply **************************/

static struct regulator_consumer_supply encore_lcd_tp_supply[] = {
	{ .supply = "vtp" },
	{ .supply = "vlcd" },
};

static struct regulator_init_data encore_lcd_tp_vinit = {
        .constraints = {
                .min_uV = 3300000,
                .max_uV = 3300000,
                .valid_modes_mask = REGULATOR_MODE_NORMAL,
                .valid_ops_mask = REGULATOR_CHANGE_STATUS,
        },
        .num_consumer_supplies = 2,
        .consumer_supplies = encore_lcd_tp_supply,
};

static struct fixed_voltage_config encore_lcd_touch_reg_data = {
        .supply_name = "vdd_lcdtp",
        .microvolts = 3300000,
//        .gpio = LCD_EN_GPIO,
        .enable_high = 1,
        .enabled_at_boot = 0,
        .init_data = &encore_lcd_tp_vinit,
};

static struct platform_device encore_lcd_touch_regulator_device = {
        .name   = "reg-fixed-voltage",
        .id     = -1,
        .dev    = {
                .platform_data = &encore_lcd_touch_reg_data,
        },
};


/************************ Keypad **************************/

static int board_keymap[] = {
	KEY(0, 0, KEY_HOME),
	KEY(1, 0, KEY_VOLUMEUP),
	KEY(2, 0, KEY_VOLUMEDOWN),
};

static struct gpio_keys_button evt_gpio_buttons[] = {
	{
		.code		= KEY_POWER,
		.gpio		= 14,
		.desc		= "POWER",
		.active_low	= 0,
		.wakeup		= 1,
	},
	{
		.code		= KEY_HOME,
		.gpio		= 48,
		.desc		= "HOME",
		.active_low	= 1,
		.wakeup		= 1,
	},
};

static struct matrix_keymap_data board_map_data = {
	.keymap			= board_keymap,
	.keymap_size		= ARRAY_SIZE(board_keymap),
};

static struct gpio_keys_platform_data evt_gpio_key_info = {
	.buttons	= evt_gpio_buttons,
	.nbuttons	= ARRAY_SIZE(evt_gpio_buttons),
};

static struct platform_device evt_keys_gpio = {
	.name	= "gpio-keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &evt_gpio_key_info,
	},
};

static struct twl4030_keypad_data evt_kp_twl4030_data = {
	.keymap_data	= &board_map_data,
	.rows		= 8,
	.cols		= 8,
	.rep		= 1,
};


//&*&*&*HC1_20110225_Add, modify for cl1
static int cl1_twl4030_keymap[] = {
#if 0
	KEY(0, 0, KEY_VOLUMEUP),		/* VOLUMEUP */
	KEY(1, 0, KEY_VOLUMEDOWN),		/* VOLUMEDOWN */
	KEY(1, 1, KEY_HOME),		/* HOME */
	KEY(1, 2, KEY_BACK),		/* BACK */
	KEY(2, 2, KEY_MENU),		/* MENU */
	KEY(2, 1, KEY_SEARCH),	/* SEARCH */
#else
/*<--LH_SWRD_CL1_Richard@20110421,Add for V_up V_down*/
	KEY(1, 1, KEY_VOLUMEDOWN),
	KEY(1,	2, KEY_VOLUMEUP),
	KEY(2, 1, KEY_HOME),		/* HOME */
	KEY(2, 2, KEY_BACK),		/* BACK */
	KEY(1, 3, KEY_MENU),		/* MENU */
/*LH_SWRD_CL1_Richard@20110421,Add for V_up V_down-->*/
#endif		
};
//&*&*&*HC2_20110613_Mod, correct the key mapping according to cl1 Artwork (v1.2)

static struct matrix_keymap_data cl1_map_data = {
	.keymap			= cl1_twl4030_keymap,
	.keymap_size		= ARRAY_SIZE(cl1_twl4030_keymap),
};

static struct twl4030_keypad_data cl1_kp_twl4030_data = {
	.keymap_data	= &cl1_map_data,
	.rows		= 3,
	.cols		= 3,
	.rep		= 1,
};
//&*&*&*HC2_20110225_Add, modify for EP10

//------------------ Voltage MMC1 -----------------//

static struct regulator_consumer_supply evt_vmmc1_supply = {
    .supply = "vmmc",
};

/* VMMC1 for OMAP VDD_MMC1 (i/o) and MMC1 card */
static struct regulator_init_data evt_vmmc1 = {
	.constraints = {
		//.min_uV 		= 2500000,
		//.max_uV 		= 2850000,
		.min_uV 		= 2850000,
		.max_uV 		= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					  | REGULATOR_MODE_STANDBY,
		.valid_ops_mask 	= REGULATOR_CHANGE_VOLTAGE
					  | REGULATOR_CHANGE_MODE
					  | REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &evt_vmmc1_supply,
};

//------------------ Voltage VSIM -----------------//

static struct regulator_consumer_supply evt_vsim_supply = {
    .supply = "vmmc_aux",
};

/* VSIM for OMAP VDD_MMC1A (i/o for DAT4..DAT7) */
static struct regulator_init_data evt_vsim = {
	.constraints = {
		.min_uV 		= 1800000,
		.max_uV 		= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					  | REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					  | REGULATOR_CHANGE_MODE
					  | REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &evt_vsim_supply,
};

/*--------------------------------------------------------------------------*/

#ifdef CONFIG_CHARGER_MAX8903
static struct platform_device max8903_charger_device = {
	.name   = "max8903_charger",
	.id     = -1,
};
#endif

#ifdef CONFIG_VIBRATOR
static struct platform_device omap_cl1_vibrator_en = {
	.name = "vibrator",
	.id = -1,
};
#endif

#ifdef CONFIG_ENCORE_MODEM_MGR
struct platform_device encore_modem_mgr_device = {
	.name = "encore_modem_mgr",
	.id   = -1,
};
#endif

/*--------------------------------------------------------------------------*/

/***++++20110115, jimmySu add backlight driver for PWM***/
#ifdef CONFIG_LEDS_OMAP_DISPLAY
/* omap3 led display */
static void zoom_pwm_config(u8 brightness)
{
	u32 omap_pwm;

	if (gptimer8_ick)
		clk_enable(gptimer8_ick);
	if (gptimer8_fck)
		clk_enable(gptimer8_fck);
/*20kHz*/
	omap_pwm = 0xFFFFFAEC + (brightness*0x5);
	//omap_pwm = 0xFFFF9A70 + (brightness * 0x2B);//1khz
	//omap_pwm = 0xFFFFFEFC + (brightness * 0x1);//100khz
	
	//omap_pwm = 0xFFFFFAD0 + (brightness*0x5);

	__raw_writel(omap_pwm, gptimer8+ 0x038); 

	if (gptimer8_ick)
		clk_disable(gptimer8_ick);
	if (gptimer8_fck)
		clk_disable(gptimer8_fck);	
}

static void zoom_pwm_enable(int enable)
{
	static int pwn_onoff = 0;

	if (pwn_onoff == enable)
		return;

	if(enable == 1){
		//__raw_writel(0x00001847, gptimer8+ 0x024);
		if (gptimer8_ick)
			clk_enable(gptimer8_ick);
		if (gptimer8_fck)
			clk_enable(gptimer8_fck);

		__raw_writel(0x000018C7, gptimer8+ 0x024);//for LCD backlight control
	}else{
		//__raw_writel(0x00001846, gptimer8+ 0x024);
		__raw_writel(0x000018C6, gptimer8+ 0x024);//for LCD backlight control
//&*&*&*BC1_110525:disable dss and pwm clocks to fix off mode issue
//		cm_rmw_mod_reg_bits(OMAP3430_EN_GPT8_MASK, 0x0 << OMAP3430_EN_GPT8_SHIFT, PER_CM_MOD, CM_FCLKEN);
//		cm_rmw_mod_reg_bits(OMAP3430_EN_GPT8_MASK, 0x0 << OMAP3430_EN_GPT8_SHIFT, PER_CM_MOD, CM_ICLKEN);
//		cm_rmw_mod_reg_bits(OMAP3430_CLKSEL_GPT8_MASK, 0x0 << OMAP3430_CLKSEL_GPT8_SHIFT, PER_CM_MOD, CM_CLKSEL);
//&*&*&*BC2_110525:disable dss and pwm clocks to fix off mode issue
		if (gptimer8_ick)
			clk_disable(gptimer8_ick);
		if (gptimer8_fck)
			clk_disable(gptimer8_fck);	
	}

	pwn_onoff = enable;
}
static void zoom_pwm_init(void)
{
	int i, counterR;
	u32 v;
	printk("[PWM] Enter %s\n", __FUNCTION__);

	gptimer8_ick = clk_get(NULL, "gpt8_ick");
	if (WARN(IS_ERR(gptimer8_ick), "clock: failed to get %s.\n", "gpt8_ick"))
	{
		gptimer8_ick = NULL;
		return;
	}	

	clk_enable(gptimer8_ick);
	
	gptimer8_fck = clk_get_sys("omap_timer.8", "fck");
	if (WARN(IS_ERR(gptimer8_fck), "clock: failed to get %s.\n", "omap_timer.8"))
	{
		gptimer8_fck = NULL;
		return;
	}	

	clk_enable(gptimer8_fck);
//	cm_rmw_mod_reg_bits(OMAP3430_CLKSEL_GPT8_MASK, 0x1 << OMAP3430_CLKSEL_GPT8_SHIFT, PER_CM_MOD, CM_CLKSEL);
//&*&*&*BC1_110525:disable dss and pwm clocks to fix off mode issue
//	cm_rmw_mod_reg_bits(OMAP3430_EN_GPT8_MASK, 0x1 << OMAP3430_EN_GPT8_SHIFT, PER_CM_MOD, CM_FCLKEN);
//	cm_rmw_mod_reg_bits(OMAP3430_EN_GPT8_MASK, 0x1 << OMAP3430_EN_GPT8_SHIFT, PER_CM_MOD, CM_ICLKEN);
//&*&*&*BC2_110525:disable dss and pwm clocks to fix off mode issue
	__raw_writel(0x4, gptimer8+ 0x040);

	__raw_writel(0xFFFFFFFE, gptimer8+ 0x038);
	__raw_writel(0xFFFFFAEC, gptimer8+ 0x02C);
	//__raw_writel(0xFFFF9A70, gptimer8+ 0x02C);//1khz
	//__raw_writel(0xFFFFFEFC, gptimer8+ 0x02C);//1khz
	__raw_writel(0x00000001, gptimer8+ 0x030);
	__raw_writel(0xFFFF0000, gptimer8+ 0x028);

	//__raw_writel(0x00001847, gptimer8+ 0x024);
	__raw_writel(0x000018C7, gptimer8+ 0x024);

	clk_disable(gptimer8_ick);
	clk_disable(gptimer8_fck);
	printk("[PWM] , PWM Set Completed\n");

}

void omap_set_primary_brightness(u8 brightness)
{


	u8 pmbr1;
	static int zoom_pwm1_config = 0;
	static int zoom_pwm1_output_enabled = 0;

//	printk("*######***enter [%s] (%d), brightness = %d\n", __FUNCTION__, __LINE__, brightness);
	if (zoom_pwm1_config == 0) {

		zoom_pwm_init();
		zoom_pwm1_config = 1;
	}

	if (!brightness) {
		zoom_pwm_enable(0);
		zoom_pwm1_output_enabled = 0;
		zoom_pwm1_config = 0;
		return;
	}

	zoom_pwm_config(brightness);
	if (zoom_pwm1_output_enabled == 0) {
		zoom_pwm_enable(1);
		zoom_pwm1_output_enabled = 1;
	}

}

static struct omap_disp_led_platform_data omap_disp_led_data = {
	.flags = LEDS_CTRL_AS_ONE_DISPLAY,
	.primary_display_set = omap_set_primary_brightness,
	.secondary_display_set = NULL,
};

static struct platform_device omap_disp_led = {
	.name	=	"display_led",
	.id	=	-1,
	.dev	= {
		.platform_data = &omap_disp_led_data,
	},
};
/* end led Display */

#endif //CONFIG_LEDS_OMAP_DISPLAY
/***-----20110115, jimmySu add backlight driver for PWM***/

/*--------------------------------------------------------------------------*/

static struct platform_device *evt_board_devices[] __initdata = {	
	&encore_lcd_touch_regulator_device,
#ifdef CONFIG_LEDS_OMAP_DISPLAY
	&omap_disp_led,
#endif
//	&evt_keys_gpio,
#ifdef CONFIG_CHARGER_MAX8903
	&max8903_charger_device,
#endif
#ifdef CONFIG_VIBRATOR
	&omap_cl1_vibrator_en,
#endif
};

static struct omap2_hsmmc_info mmc[] __initdata = {
//&*&*&*SJ1_20110422, Modify mmcblk number.
#if defined (CONFIG_CHANGE_INAND_MMC_SCAN_INDEX)  //For EP Series

	{
		.name		= "internal",
		.mmc		= 2,
		.caps		= MMC_CAP_8_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.no_off 	= true,
		.nonremovable	= true,
		.power_saving	= true,
		.ocr_mask		= 0xff,
	},
	{
		.name		= "external",
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.cd_active_high = false,
		.gpio_wp	= -EINVAL,
		.power_saving	= false,
	},
#else

	{
		.name		= "external",
		.mmc 		= 1,
		.caps 		= MMC_CAP_4_BIT_DATA,
		.cd_active_high = false,
		.gpio_wp 	= -EINVAL,
		.power_saving 	= true,
	},
	{
		.name		= "internal",
		.mmc		= 2,
		.caps		= MMC_CAP_8_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.no_off		= true,
		.nonremovable	= true,
		.power_saving	= true,
		.ocr_mask       = 0xff,
	},
#endif
//&*&*&*SJ2_20110422, Modify mmcblk number.
	{
		.mmc            = 3,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_wp        = -EINVAL,
		.gpio_cd        = -EINVAL,
	},
	{}      /* Terminator */
};

static struct regulator_consumer_supply evt_vpll2_supply = {
	.supply = "vdds_dsi",
};

static struct regulator_consumer_supply evt_vdda_dac_supply = {
	.supply = "vdda_dac",
};

static struct regulator_init_data evt_vpll2 = {
	.constraints = {
		.min_uV             = 1800000,
		.max_uV             = 1800000,
		.valid_modes_mask   = REGULATOR_MODE_NORMAL
				      | REGULATOR_MODE_STANDBY,
		.valid_ops_mask     = REGULATOR_CHANGE_MODE
				      | REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &evt_vpll2_supply,
};

static struct regulator_init_data evt_vdac = {
	.constraints = {
		.min_uV             = 1800000,
		.max_uV             = 1800000,
		.valid_modes_mask   = REGULATOR_MODE_NORMAL
				    | REGULATOR_MODE_STANDBY,
		.valid_ops_mask     = REGULATOR_CHANGE_MODE
				    | REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &evt_vdda_dac_supply,
};

static int evt_twl_gpio_setup(struct device *dev, unsigned gpio, unsigned ngpio)
{
	/* gpio + 0 is "mmc0_cd" (input/IRQ) */
//&*&*&*SJ1_20110422, Modify mmcblk number.
#if defined (CONFIG_CHANGE_INAND_MMC_SCAN_INDEX)
//&*&*&*SJ1_20110607, Add SIM card detection.
#if defined (CONFIG_SIM_CARD_DETECTION) && defined (CONFIG_CHANGE_INAND_MMC_SCAN_INDEX)
	mmc[0].gpio_sim_cd = gpio + 1;
	mmc[1].gpio_sim_cd = -EINVAL;
	mmc[2].gpio_sim_cd = -EINVAL;
#endif
//&*&*&*SJ2_20110607, Add SIM card detection.
	mmc[1].gpio_cd = gpio + 0;
#else
	mmc[0].gpio_cd = gpio + 0;
#endif
//&*&*&*SJ1_20110422, Modify mmcblk number.
        
#ifdef CONFIG_MMC_EMBEDDED_SDIO
       /* The controller that is connected to the 128x device
        * should have the card detect gpio disabled. This is
        * achieved by initializing it with a negative value
        */
       mmc[CONFIG_TIWLAN_MMC_CONTROLLER - 1].gpio_cd = -EINVAL;
#endif

	/* link regulators to MMC adapters ... we "know" the
	 * regulators will be set up only *after* we return.
	*/
	omap2_hsmmc_init(mmc);
        
//&*&*&*SJ1_20110422, Modify mmcblk number.
#if defined (CONFIG_CHANGE_INAND_MMC_SCAN_INDEX)        
	evt_vmmc1_supply.dev = mmc[1].dev;
	evt_vsim_supply.dev = mmc[1].dev;
#else
	evt_vmmc1_supply.dev = mmc[0].dev;
	evt_vsim_supply.dev = mmc[0].dev;
#endif
//&*&*&*SJ2_20110422, Modify mmcblk number.
	
	return 0;
}


static int evt_batt_table[] = {
/* 0 C*/
30800, 29500, 28300, 27100,
26000, 24900, 23900, 22900, 22000, 21100, 20300, 19400, 18700, 17900,
17200, 16500, 15900, 15300, 14700, 14100, 13600, 13100, 12600, 12100,
11600, 11200, 10800, 10400, 10000, 9630,  9280,  8950,  8620,  8310,
8020,  7730,  7460,  7200,  6950,  6710,  6470,  6250,  6040,  5830,
5640,  5450,  5260,  5090,  4920,  4760,  4600,  4450,  4310,  4170,
4040,  3910,  3790,  3670,  3550
};

static struct twl4030_bci_platform_data evt_bci_data = {
	.battery_tmp_tbl	= evt_batt_table,
	.tblsize		= ARRAY_SIZE(evt_batt_table),
};

/************************** USB devices *******************************/

static struct twl4030_usb_data evt_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static struct twl4030_gpio_platform_data evt_gpio_data = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
//&*&*&*HC1_110729, enable debounce for gpio0(SD card detect) and gpio1(SIM card detect)
	.debounce = 0x3,
//&*&*&*HC2_110729, enable debounce for gpio0(SD card detect) and gpio1(SIM card detect)
	.setup		= evt_twl_gpio_setup,
};

static struct twl4030_madc_platform_data evt_madc_data = {
	.irq_line	= 1,
};

static struct twl4030_codec_audio_data evt_audio_data = {
	.audio_mclk = 26000000,
};

static struct twl4030_codec_data evt_codec_data = {
	.audio_mclk = 26000000,
	.audio = &evt_audio_data,
};
//&*&*&*BC1_110615: fix the issue that system can not enter off mode
static struct __initdata twl4030_power_data evt_t2scripts_data;
//&*&*&*BC2_110615: fix the issue that system can not enter off mode
static struct twl4030_platform_data evt_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,

	/* platform_data for children goes here */
	.bci		= &evt_bci_data,
	.madc		= &evt_madc_data,
	.usb		= &evt_usb_data,
	.gpio		= &evt_gpio_data,
//&*&*&*HC1_20110225_Add, modify for EP10
	//.keypad		= &evt_kp_twl4030_data,
	.keypad		= &cl1_kp_twl4030_data,	
//&*&*&*HC2_20110225_Add, modify for EP10
	.codec		= &evt_codec_data,
	/* regulators */
	.vmmc1		= &evt_vmmc1,
	.vsim		= &evt_vsim,
	.vpll2		= &evt_vpll2,
	.vdac		= &evt_vdac,
//&*&*&*BC1_110615: fix the issue that system can not enter off mode	
	.power		= &evt_t2scripts_data,
//&*&*&*BC2_110615: fix the issue that system can not enter off mode	
};

#ifdef CONFIG_BATTERY_MAX17042
struct max17042_platform_data max17042_platform_data_here = {

        //fill in device specific data here
        //load stored parameters from Rom Tokens? 
        //.val_FullCAP =
        //.val_Cycles =
        //.val_FullCAPNom =
        //.val_SOCempty =
        //.val_Iavg_empty =
        //.val_RCOMP0 =
        //.val_TempCo=
        //.val_k_empty0 =
        //.val_dQacc =
        //.val_dPacc =

        .gpio = MAX17042_GPIO_FOR_IRQ,
};
#endif
//&*&*&*JJ1 Cypress touch driver	
#if defined(CONFIG_TOUCHSCREEN_CYPRESS_TTSP) || defined(CONFIG_TOUCHSCREEN_CYPRESS_7inch)
static struct cyttsp_platform_data cyttsp_data = {
	
#if defined(CONFIG_TOUCHSCREEN_CYPRESS_7inch)
#ifdef CONFIG_PANEL_PORTRAIT  //Protrait
	.maxx = 600,
	.maxy = 1024,
	.flags = 0x0,
#else   //Landscape
	.maxx = 1024,
	.maxy = 600,
	.flags = 0x5,
#endif
#endif

#if defined(CONFIG_TOUCHSCREEN_CYPRESS_TTSP)
#ifdef CONFIG_PANEL_PORTRAIT  //Protrait
	.maxx = 768,
	.maxy = 1024,
	.flags = 0x5, //Jimmy Su, 20100831, align touch and tft orientation
#else   //Landscape
	.maxx = 1024,
	.maxy = 600,
	.flags = 0x6, //Jimmy Su, 20100831, align touch and tft orientation
#endif
#endif
//	.gen = CYTTSP_SPI10,	/* either */
//	.gen = CYTTSP_GEN2,	/* or */
	.gen = CYTTSP_GEN3,	/* or */
	.use_st = CYTTSP_USE_ST,
	.use_mt = CYTTSP_USE_MT,
	.use_trk_id = CYTTSP_USE_TRACKING_ID,
	.use_sleep = CYTTSP_USE_SLEEP,
	.use_gestures = CYTTSP_USE_GESTURES,
	/* activate up to 4 groups
	 * and set active distance
	 */
	.gest_set = CYTTSP_GEST_GRP1 | CYTTSP_GEST_GRP2 | 
				CYTTSP_GEST_GRP3 | CYTTSP_GEST_GRP4 | 
				CYTTSP_ACT_DIST,
	/* change act_intrvl to customize the Active power state 
	 * scanning/processing refresh interval for Operating mode
	 */
	.act_intrvl = CYTTSP_ACT_INTRVL_DFLT,
	/* change tch_tmout to customize the touch timeout for the
	 * Active power state for Operating mode
	 */
	.tch_tmout = CYTTSP_TCH_TMOUT_DFLT,
	/* change lp_intrvl to customize the Low Power power state 
	 * scanning/processing refresh interval for Operating mode
	 */
	.lp_intrvl = CYTTSP_LP_INTRVL_DFLT,
};

static void cyttsp_tch_gpio_init(void)
{
	if (gpio_request(ZOOM2_TOUCHSCREEN_IRQ_GPIO, "touch") < 0) {
		printk(KERN_ERR "can't get cypress touch pen down GPIO\n");
		return;
	}
	gpio_direction_input(ZOOM2_TOUCHSCREEN_IRQ_GPIO);
	//omap_set_gpio_debounce(ZOOM2_TOUCHSCREEN_IRQ_GPIO, 1);
	//omap_set_gpio_debounce_time(ZOOM2_TOUCHSCREEN_IRQ_GPIO, 0xa);
}
#endif /* CONFIG_TOUCHSCREEN_CYPRESS_TTSP */
//&*&*&*JJ2 Cypress touch driver	

//&*&*&*JJ1 ITE IT7260 touch driver
#if defined(CONFIG_TOUCHSCREEN_ITE_IT7260)
static struct ite_i2c_it7260_platform_data it7260_platform_data[] = {
	{
		.version	= 0x0,
	}
};

static void it7260_tch_gpio_init(void)
{
	if (gpio_request(ZOOM2_TOUCHSCREEN_IRQ_GPIO, "touch") < 0) {
		printk(KERN_ERR "can't get cypress touch pen(%d) down GPIO\n");
		return;
	}
	gpio_direction_input(ZOOM2_TOUCHSCREEN_IRQ_GPIO);
	omap_set_gpio_debounce(ZOOM2_TOUCHSCREEN_IRQ_GPIO, 1);
	omap_set_gpio_debounce_time(ZOOM2_TOUCHSCREEN_IRQ_GPIO, 0xa);
}
#endif /* CONFIG_TOUCHSCREEN_ITE_IT7260 */
//&*&*&*JJ2 ITE IT7260 touch driver

#if defined(CONFIG_REGULATOR_MAX8903)
#define OMAP_MAX8903_DCOK_GPIO 51
#define OMAP_MAX8903_CEN_GPIO  42
#define OMAP_MAX8903_CHG_GPIO 15

static int max8903_is_battery_online(void)
{
	int state;
	state = gpio_get_value(OMAP_MAX8903_DCOK_GPIO);
	
	return (state ? 1 : 0);
}

static int max8903_is_charger_online(void)
{
	int state;
	state = gpio_get_value(OMAP_MAX8903_DCOK_GPIO);
	
	return (state ? 0 : 1);
}

static int max8903_is_charging(void)
{
	int state;
	state = gpio_get_value(OMAP_MAX8903_CHG_GPIO);
	
	return (state ? 0 : 1);
}

static int max8903_charger_enable(void)
{
	gpio_set_value(OMAP_MAX8903_CEN_GPIO, 0);
	
	return 0;
}
#endif

#if defined(CONFIG_BATTERY_MAX17043)
#define OMAP_MAX17043_ALERT_IRQ_GPIO 44
static struct max17043_irq_info max17043_irq = {
	.gpio = OMAP_MAX17043_ALERT_IRQ_GPIO,
	.irq = OMAP_GPIO_IRQ(OMAP_MAX17043_ALERT_IRQ_GPIO),
	.irq_flags = IRQF_TRIGGER_FALLING,
};

static struct max17043_platform_data max17043_pdata = { 
	.irq_info = &max17043_irq,
	#if defined(CONFIG_REGULATOR_MAX8903)
	.battery_is_online = max8903_is_battery_online,
	.charger_is_online = max8903_is_charger_online,
	.battery_is_charging = max8903_is_charging,
	.charger_enable = max8903_charger_enable,
	#endif
};
#endif

#if defined(CONFIG_BATTERY_BQ27541)
static struct bq27541_platform_data bq27541_pdata = {
	#if defined(CONFIG_REGULATOR_MAX8903)
	.battery_is_online = max8903_is_battery_online,
	.charger_is_online = max8903_is_charger_online,
	.battery_is_charging = max8903_is_charging,
	.charger_enable = max8903_charger_enable,
	#endif
};
#endif

static struct i2c_board_info __initdata evt_i2c_boardinfo[] = {
// #ifdef CONFIG_BATTERY_MAX17042
//     {
//         I2C_BOARD_INFO(MAX17042_DEVICE_ID, MAX17042_I2C_SLAVE_ADDRESS),
//         .platform_data = &max17042_platform_data_here,
//         .irq = OMAP_GPIO_IRQ(MAX17042_GPIO_FOR_IRQ),
//     },
// #endif  /*CONFIG_BATTERY_MAX17042*/
	{
		I2C_BOARD_INFO("tps65921", 0x48),
		.flags		= I2C_CLIENT_WAKE,
		.irq		= INT_34XX_SYS_NIRQ,
		.platform_data	= &evt_twldata,
	},
// 	{
// 		I2C_BOARD_INFO(KXTF9_DEVICE_ID, KXTF9_I2C_SLAVE_ADDRESS),
// 		.platform_data = &kxtf9_platform_data_here,
// 		.irq = OMAP_GPIO_IRQ(KXTF9_GPIO_FOR_IRQ),
// 	},
};



#if defined(CONFIG_SND_SOC_TLV320DAC3100)
static void audio_dac_3100_dev_init(void)
{
    printk("board-3621_evt1a.c: audio_dac_3100_dev_init ...\n");
    if (gpio_request(AUDIO_CODEC_RESET_GPIO, "AUDIO_CODEC_RESET_GPIO") < 0) {
       printk(KERN_ERR "can't get AUDIO_CODEC_RESET_GPIO \n");
       return;
    }

    printk("board-3621_evt1a.c: audio_dac_3100_dev_init > set AUDIO_CODEC_RESET_GPIO to output Low!\n");
    gpio_direction_output(AUDIO_CODEC_RESET_GPIO, 0);
	gpio_set_value(AUDIO_CODEC_RESET_GPIO, 0);

    printk("board-3621_evt1a.c: audio_dac_3100_dev_init ...\n");
    if (gpio_request(AUDIO_CODEC_POWER_ENABLE_GPIO, "AUDIO DAC3100 POWER ENABLE") < 0) {
       printk(KERN_ERR "can't get AUDIO_CODEC_POWER_ENABLE_GPIO \n");
       return;
    }

    printk("board-3621_evt1a.c: audio_dac_3100_dev_init > set AUDIO_CODEC_POWER_ENABLE_GPIO to output and value high!\n");
    gpio_direction_output(AUDIO_CODEC_POWER_ENABLE_GPIO, 0);
	gpio_set_value(AUDIO_CODEC_POWER_ENABLE_GPIO, 1);

	/* 1 msec delay needed after PLL power-up */
    mdelay (1);

    printk("\tset AUDIO_CODEC_RESET_GPIO to output and value high!\n");
	gpio_set_value(AUDIO_CODEC_RESET_GPIO, 1);
}
#endif

#ifdef CONFIG_BATTERY_MAX17042
static void max17042_dev_init(void)
{
        printk("max17042_dev_init ...\n");

    if (gpio_request(MAX17042_GPIO_FOR_IRQ, "max17042_irq") < 0) {
        printk(KERN_ERR "Can't get GPIO for max17042 IRQ\n");
        return;
    }

    printk("\tInit max17042 irq pin %d !\n", MAX17042_GPIO_FOR_IRQ);
    gpio_direction_input(MAX17042_GPIO_FOR_IRQ);
    printk("\tmax17042 GPIO pin read %d\n", gpio_get_value(MAX17042_GPIO_FOR_IRQ));
}
#endif

static struct i2c_board_info __initdata evt_i2c_bus2_info[] = {
//aimar + 20101214
#if defined(CONFIG_ADXL345_SENSOR_MODULE) || defined(CONFIG_ADXL345_GSENSOR_MODULE)
	{
		I2C_BOARD_INFO("adxl345", 0x53),
        .irq = OMAP_GPIO_IRQ(GPIO_ADXL345_IRQ),
	},
#endif
//aimar - 20101214
/*	{
		I2C_BOARD_INFO(CY_I2C_NAME, CYTTSP_I2C_SLAVEADDRESS),
		.platform_data = &cyttsp_platform_data,
		.irq = OMAP_GPIO_IRQ(OMAP_CYTTSP_GPIO),
	},
	{
		I2C_BOARD_INFO(AIC3100_NAME,  AIC3100_I2CSLAVEADDRESS),
                .irq = OMAP_GPIO_IRQ(AUDIO_CODEC_IRQ_GPIO),
	},*/
//&*&*&*JJ1 Cypress touch driver	
#if defined(CONFIG_TOUCHSCREEN_CYPRESS_TTSP)
	{
		I2C_BOARD_INFO(CYTTSP_I2C_NAME, 0x24),
		.platform_data = &cyttsp_data,
		.irq = OMAP_GPIO_IRQ(ZOOM2_TOUCHSCREEN_IRQ_GPIO),
	},
#endif /* CONFIG_TOUCHSCREEN_CYPRESS_TTSP */
#if defined(CONFIG_TOUCHSCREEN_CYPRESS_7inch)
	{
		//I2C_BOARD_INFO(CYTTSP_I2C_NAME, 0x24),//mirasol
		I2C_BOARD_INFO(CYTTSP_I2C_NAME, 0x22),
		.platform_data = &cyttsp_data,
		.irq = OMAP_GPIO_IRQ(ZOOM2_TOUCHSCREEN_IRQ_GPIO),
	},
#endif
#if defined(CONFIG_TOUCHSCREEN_ITE_IT7260)
//&*&*&*JJ2 Cypress touch driver	
//&*&*&*JJ1 ITE IT7260 touch driver
	{
		I2C_BOARD_INFO(IT7260_I2C_NAME,  0x46),
		.platform_data = &it7260_platform_data,
//		.irq = OMAP_GPIO_IRQ(ZOOM2_TOUCHSCREEN_IRQ_GPIO),
	},
#endif /* CONFIG_TOUCHSCREEN_ITE_IT7260 */
//&*&*&*JJ2 ITE IT7260 touch driver
//&*&*&*JJ1_20101215 EETI EXC7200 touch driver
#if defined(CONFIG_TOUCHSCREEN_EETI_EXC7200)
	{
		I2C_BOARD_INFO("egalax_i2c",  0x04),
//		.platform_data = &egalax_i2c_boardinfo,
		.irq = OMAP_GPIO_IRQ(ZOOM2_TOUCHSCREEN_IRQ_GPIO),
	},
#endif /* CONFIG_TOUCHSCREEN_EETI_EXC7200 */
//&*&*&*JJ2_20101215 EETI EXC7200 touch driver

//&*&*&*JJ1
#if (defined(CONFIG_VIDEO_MT9D115) || defined(CONFIG_VIDEO_MT9D115_MODULE))
	{
		I2C_BOARD_INFO("mt9d115", MT9D115_I2C_ADDR),
		.platform_data = &zoom2_mt9d115_platform_data,
	},
#endif
//&*&*&*JJ1
#if (defined(CONFIG_VIDEO_S5K5CA) || defined(CONFIG_VIDEO_S5K5CA_MODULE)) && \
    defined(CONFIG_VIDEO_OMAP3)
	{
		I2C_BOARD_INFO("s5k5ca", S5K5CA_I2C_ADDR),
		.platform_data = &zoom2_s5k5ca_platform_data,
	},
#endif

#if (defined(CONFIG_VIDEO_MT9V115) || defined(CONFIG_VIDEO_MT9V115_MODULE)) && \
    defined(CONFIG_VIDEO_OMAP3)
	{
		I2C_BOARD_INFO("mt9v115", MT9V115_I2C_ADDR),
		.platform_data = &zoom2_mt9v115_platform_data,
	},
#endif
	{
		I2C_BOARD_INFO("tlv320aic3111",  0x18),
	},
//&*&*&*SJ1_201103116, Add ELAN IOCF668 device.
#if defined(CONFIG_IOCF668)
	{
		I2C_BOARD_INFO("iocf668", 0x33), 
	},
#endif /* End CONFIG_IOCF668 */
//&*&*&*SJ2_201103116, Add ELAN IOCF668 device.	
#if defined(CONFIG_BATTERY_MAX17043)
	{ 
		I2C_BOARD_INFO("max17043", 0x36), 
		.platform_data= &max17043_pdata,
	},
#endif

#if defined(CONFIG_BATTERY_BQ27541)
	{
		I2C_BOARD_INFO("bq27541",  0x55),
		.platform_data= &bq27541_pdata,
	},
#endif /* CONFIG_BATTERY_BQ27510 */

/*<-- LH_SWRD_CL1_Richard@20110511  morgan touch for CL1 */
#if defined(CONFIG_TOUCHSCREEN_MG_I2C)
	{
	   I2C_BOARD_INFO("mg-i2c-mtouch", 0x44),
        .irq = OMAP_GPIO_IRQ(MORGAN_IRQ_GPIO),
	},
#endif
/*LH_SWRD_CL1_Richard@201104511 morgan touch for CL1  -->*/
};


static int __init omap_i2c_init(void)
{
	/* Disable OMAP 3630 internal pull-ups for I2Ci */
	if (cpu_is_omap3630()) {

		u32 prog_io;

		prog_io = omap_ctrl_readl(OMAP343X_CONTROL_PROG_IO1);
		/* Program (bit 19)=1 to disable internal pull-up on I2C1 */
		prog_io |= OMAP3630_PRG_I2C1_PULLUPRESX;
                /* Program (bit 0)=1 to disable internal pull-up on I2C2 */
		prog_io |= OMAP3630_PRG_I2C2_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP343X_CONTROL_PROG_IO1);

		prog_io = omap_ctrl_readl(OMAP36XX_CONTROL_PROG_IO2);
                /* Program (bit 7)=1 to disable internal pull-up on I2C3 */
		prog_io |= OMAP3630_PRG_I2C3_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP36XX_CONTROL_PROG_IO2);

		prog_io = omap_ctrl_readl(OMAP36XX_CONTROL_PROG_IO_WKUP1);
                /* Program (bit 5)=1 to disable internal pull-up on I2C4(SR) */
		prog_io |= OMAP3630_PRG_SR_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP36XX_CONTROL_PROG_IO_WKUP1);
	}
	
	omap_register_i2c_bus(1, 100, NULL, 
                              evt_i2c_boardinfo,
                              ARRAY_SIZE(evt_i2c_boardinfo));
	
        omap_register_i2c_bus(2, 400, NULL, 
                              evt_i2c_bus2_info,
                              ARRAY_SIZE(evt_i2c_bus2_info));
	
        return 0;
}

static struct omap_musb_board_data musb_board_data = {
	.interface_type		= MUSB_INTERFACE_ULPI,
#ifdef CONFIG_USB_MUSB_OTG
	.mode			= MUSB_OTG,
#elif defined(CONFIG_USB_MUSB_HDRC_HCD)
	.mode			= MUSB_HOST,
#elif defined(CONFIG_USB_GADGET_MUSB_HDRC)
	.mode			= MUSB_PERIPHERAL,
#endif
	.power			= 100,
};

static struct omap_uart_port_info omap_serial_platform_data[] = {
	{
#if defined(CONFIG_SERIAL_OMAP_UART1_DMA)
		.use_dma	= CONFIG_SERIAL_OMAP_UART1_DMA,
		.dma_rx_buf_size = CONFIG_SERIAL_OMAP_UART1_RXDMA_BUFSIZE,
		.dma_rx_timeout	= CONFIG_SERIAL_OMAP_UART1_RXDMA_TIMEOUT,
#else
		.use_dma	= 0,
		.dma_rx_buf_size = 0,
		.dma_rx_timeout	= 0,
#endif /* CONFIG_SERIAL_OMAP_UART1_DMA */
		.idle_timeout	= CONFIG_SERIAL_OMAP_IDLE_TIMEOUT,
		.flags		= 1,
	},
	{
#if defined(CONFIG_SERIAL_OMAP_UART2_DMA)
		.use_dma	= CONFIG_SERIAL_OMAP_UART2_DMA,
		.dma_rx_buf_size = CONFIG_SERIAL_OMAP_UART2_RXDMA_BUFSIZE,
		.dma_rx_timeout	= CONFIG_SERIAL_OMAP_UART2_RXDMA_TIMEOUT,
#else
		.use_dma	= 0,
		.dma_rx_buf_size = 0,
		.dma_rx_timeout	= 0,
#endif /* CONFIG_SERIAL_OMAP_UART2_DMA */
		.idle_timeout	= CONFIG_SERIAL_OMAP_IDLE_TIMEOUT,
		.flags		= 1,
	},
	{
#if defined(CONFIG_SERIAL_OMAP_UART3_DMA)
		.use_dma	= CONFIG_SERIAL_OMAP_UART3_DMA,
		.dma_rx_buf_size = CONFIG_SERIAL_OMAP_UART3_RXDMA_BUFSIZE,
		.dma_rx_timeout	= CONFIG_SERIAL_OMAP_UART3_RXDMA_TIMEOUT,
#else
		.use_dma	= 0,
		.dma_rx_buf_size = 0,
		.dma_rx_timeout	= 0,
#endif /* CONFIG_SERIAL_OMAP_UART3_DMA */
		.idle_timeout	= 0,//CONFIG_SERIAL_OMAP_IDLE_TIMEOUT,
		.flags		= 1,
	},
	{
#if defined(CONFIG_SERIAL_OMAP_UART4_DMA)
		.use_dma	= CONFIG_SERIAL_OMAP_UART4_DMA,
		.dma_rx_buf_size = CONFIG_SERIAL_OMAP_UART4_RXDMA_BUFSIZE,
		.dma_rx_timeout	= CONFIG_SERIAL_OMAP_UART4_RXDMA_TIMEOUT,
#else
		.use_dma	= 0,
		.dma_rx_buf_size = 0,
		.dma_rx_timeout	= 0,
#endif /* CONFIG_SERIAL_OMAP_UART4_DMA */
		.idle_timeout	= 0,//CONFIG_SERIAL_OMAP_IDLE_TIMEOUT,
		.flags		= 1,
	},
	{
		.flags		= 0
	}
};

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource = {
    .start  = ENCORE_RAM_CONSOLE_START,
    .end    = (ENCORE_RAM_CONSOLE_START + ENCORE_RAM_CONSOLE_SIZE - 1),
    .flags  = IORESOURCE_MEM,
};

static struct platform_device ram_console_device = {
    .name           = "ram_console",
    .id             = 0,
    .num_resources  = 1,
    .resource       = &ram_console_resource,
};
#endif 

/*---------------------------- CHARGER -------------------------------------*/

#if defined(CONFIG_REGULATOR_MAX8903)
static struct max8903_mach_info max8903_init_dev_data = {
	.gpio_cen = 42,
	.gpio_dcm = 104,
	//.gpio_iusb = 61,
	.gpio_cen_state = 1,
	.gpio_dcm_state = 0,
//	.gpio_iusb_state = 0,
	.irq_info[MAX8903_PIN_FLAUTn] = {
		.irq = OMAP_GPIO_IRQ(43),
		.gpio = 43,/*34(EVT1)*//*43(EVT2)*/
		.gpio_state = 1,
	},
	.irq_info[MAX8903_PIN_CHARHEn] = {
		.irq = OMAP_GPIO_IRQ(15),
		.gpio = 15,
		.gpio_state = 1,
	},
	.irq_info[MAX8903_PIN_DCOKn] = {
		.irq = OMAP_GPIO_IRQ(51),
		.gpio = 51,
		.gpio_state = 1,
	},
};

static struct regulator_consumer_supply max8903_vcharge_supply[] = {
	{
	       .supply         = "max8903",
	},
};

static struct regulator_init_data max8903_init  = {
	.constraints = {
		.min_uV                 = 0,
		.max_uV                 = 5000000,
		.min_uA                 = 0,
		.max_uA                 = 1500000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					  								| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_CURRENT
					  								| REGULATOR_CHANGE_MODE
					  								| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = ARRAY_SIZE(max8903_vcharge_supply),
	.consumer_supplies      = max8903_vcharge_supply,

	.driver_data = &max8903_init_dev_data,
};

/* GPIOS need to be in order of max8903 */
static struct platform_device edp1_max8903_regulator_device = {
	.name           = "max8903",
	.id             = -1,
	.dev		= {
		.platform_data = &max8903_init,
	},
};
#endif

#if defined(CONFIG_LEDS_EP10)
static struct ep10_led_platdata ep10_pdata_led0 = {
	.gpio_charge		= 59,	
	.gpio_charge_full		= 60,
	.flags      = LED_CORE_SUSPENDRESUME,	
	.name		= "led0",	
	.def_trigger	= "battery-charging",
};

static struct platform_device ep10_led0 = {	
	.name		= "ep10_led",	
	.id		= 0,	
	.dev		= {		
		.platform_data = &ep10_pdata_led0,	
	},
};
#endif

#if defined(CONFIG_LEDS_BUTTON_EP10)
static struct ep10_led_platdata ep10_pdata_button_led1 = {
	.gpio		= 91,	
	.flags      = LED_CORE_SUSPENDRESUME,	
	.name		= "keyboard-backlight",	
	.def_trigger	= "key-press",
};

static struct platform_device ep10_button_led1 = {	
	.name		= "ep10_button_led",	
	.id		= 0,	
	.dev		= {		
		.platform_data = &ep10_pdata_button_led1,	
	},
};
#endif

#if defined(CONFIG_TWL4030_MADC_BATTERY)
#if defined(CONFIG_REGULATOR_MAX8903)
static struct twl4030_madc_battery_platform_data twl_madc_battery_pdata = { 

	.battery_is_online = max8903_is_battery_online,
	.charger_is_online = max8903_is_charger_online,
	.battery_is_charging = max8903_is_charging,
	.charger_enable = max8903_charger_enable,
};
#endif
static struct platform_device twl4030_madc_battery = {
	.name	= "twl4030_madc_battery",
	.id             = -1,
	#if defined(CONFIG_REGULATOR_MAX8903)
	.dev		= {
		.platform_data = &twl_madc_battery_pdata,
	},	
	#endif
};
#endif

static struct platform_device *evt_board_devices_2[] __initdata = {
#if defined(CONFIG_REGULATOR_MAX8903)
	&edp1_max8903_regulator_device,
#endif
#if defined(CONFIG_LEDS_EP10)
	&ep10_led0,
#endif
#if defined(CONFIG_LEDS_BUTTON_EP10)
	&ep10_button_led1,
#endif
#if defined(CONFIG_TWL4030_MADC_BATTERY)
	&twl4030_madc_battery,
#endif

};

void __init evt_peripherals_init(void)
{
//&*&*&*SJ1_20110419, Add /proc file to display software and hardware version.
#if defined (CONFIG_PROCFS_DISPLAY_SW_HW_VERSION)
	get_share_region();	
#endif
//&*&*&*SJ2_20110419, Add /proc file to display software and hardware version.

//&*&*&*BC1_110615: fix the issue that system can not enter off mode
	twl4030_get_scripts(&evt_t2scripts_data);
//&*&*&*BC2_110615: fix the issue that system can not enter off mode

    
#ifdef CONFIG_ANDROID_RAM_CONSOLE
    platform_device_register(&ram_console_device);
#endif
    
    omap_i2c_init();
    platform_add_devices( evt_board_devices, ARRAY_SIZE(evt_board_devices) );
    platform_add_devices( evt_board_devices_2, ARRAY_SIZE(evt_board_devices_2) );
    
#ifdef CONFIG_ENCORE_MODEM_MGR
    if (is_encore_3g()) {
        platform_device_register(&encore_modem_mgr_device);
    }
#endif 

#ifdef CONFIG_SND_SOC_TLV320DAC3100
    audio_dac_3100_dev_init();
#endif
/***++++20110115, jimmySu add backlight driver for PWM***/
#ifdef CONFIG_LEDS_OMAP_DISPLAY
	gptimer8 = (unsigned long)ioremap(0x4903E000, 16);
	//zoom_pwm_init();
	omap_set_primary_brightness(100);
#endif // CONFIG_LEDS_OMAP_DISPLAY
/***-----20110115, jimmySu add backlight driver for PWM***/


    /* NOTE: Please deselect CONFIG_MACH_OMAP_USE_UART3 in order to init 
    *  only UART1 and UART2, all in the name of saving some power.
    */
    omap_serial_init(omap_serial_platform_data);
    evt_lcd_panel_init();
//    kxtf9_dev_init();
#ifdef CONFIG_BATTERY_MAX17042
    max17042_dev_init();
#endif

    usb_musb_init(&musb_board_data);
	
#if (defined(CONFIG_VIDEO_MT9V115) || defined(CONFIG_VIDEO_MT9V115_MODULE)) && \
    defined(CONFIG_VIDEO_OMAP3)
	zoom2_cam_mt9v115_init();
#endif
#if (defined(CONFIG_VIDEO_S5K5CA) || defined(CONFIG_VIDEO_S5K5CA_MODULE)) && \
    defined(CONFIG_VIDEO_OMAP3)
	zoom2_cam_s5k5ca_init();
#endif

//&*&*&*SJ1_20110607
#if defined (CONFIG_ANDROID_FACTORY_DEFAULT_REBOOT)
	android_factory_default_init();
#endif
//&*&*&*SJ2_20110607
}
