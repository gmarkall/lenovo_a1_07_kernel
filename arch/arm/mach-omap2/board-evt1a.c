/*
 * Copyright (C) 2009 Texas Instruments Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/bootmem.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <plat/common.h>
#include <plat/board.h>
#include <plat/usb.h>
#include <plat/opp_twl_tps.h>
#include <plat/timer-gp.h>
#include <mach/board-encore.h>

#include "mux.h"
#include "omap3-opp.h"
#include "smartreflex-class3.h"

#if defined(CONFIG_MACH_SDRAM_HYNIX_H8MBX00U0MER0EM_OR_SAMSUNG_K4X4G303PB)
#include "sdram-samsung-k4x4g303pb-or-hynix-h8mbx00u0mer-0em.h"
#elif defined (CONFIG_MACH_SDRAM_SAMSUNG_K4X4G303PB)
#include "sdram-samsung-k4x4g303pb.h"
#elif defined(CONFIG_MACH_SDRAM_HYNIX_H8MBX00U0MER0EM)
#include "sdram-hynix-h8mbx00u0mer-0em.h"
#elif defined(CONFIG_MACH_SDRAM_ELPIDA_EDK4332C2PB)
#include "sdram-elpida-pop-edk4332c2pb.h"
#endif

#define CONFIG_WIRELESS_BCM4329_RFKILL  1

#ifdef CONFIG_WIRELESS_BCM4329
#include <linux/fxn-rfkill.h>
#endif

#ifdef CONFIG_WIRELESS_BCM4329_RFKILL
static struct fxn_rfkill_platform_data fxn_plat_data = {
      .wifi_bt_pwr_en_gpio = FXN_GPIO_WL_BT_POW_EN,      /* WiFi/BT power enable GPIO */
	.wifi_reset_gpio = FXN_GPIO_WL_RST_N,   		/* WiFi reset GPIO */
	.bt_reset_gpio = FXN_GPIO_BT_RST_N,          	/* Bluetooth reset GPIO */
#ifdef CONFIG_RFKILL_GPS
	#ifdef  FXN_GPIO_GPS_PWR_EN
	.gps_pwr_en_gpio = FXN_GPIO_GPS_PWR_EN, 	/* GPS power enable GPIO */
	#endif
	.gps_on_off_gpio = FXN_GPIO_GPS_ON_OFF,		/* GPS on/off GPIO */
	.gps_reset_gpio = FXN_GPIO_GPS_RST,    		/* GPS reset GPIO */	
#endif
};

/*
static struct platform_device evt_wifi_device = {
	.name		= "device_wifi",
	.id		      = 1,
	.dev.platform_data = NULL,
};
*/

static struct platform_device conn_device = {
	.name           = "rfkill-fxn",
	.id             = -1,
	.dev.platform_data = &fxn_plat_data,
//	.dev.platform_data = NULL,
};

	
static struct platform_device *conn_plat_devices[] __initdata = {
    &conn_device,
//    &evt_wifi_device,
};

/*
void bluetooth_init(void)
{
	printk("Bluetooth init...\n");
	
	gpio_request(FXN_GPIO_BT_RST_N , "FXN_GPIO_BT_RST_N");
	gpio_direction_output(FXN_GPIO_BT_RST_N, 0);   //Enable the BT_EN GPIO-102
	
	gpio_request(FXN_GPIO_WL_BT_POW_EN , "FXN_GPIO_WL_BT_POW_EN");
	gpio_direction_output(FXN_GPIO_WL_BT_POW_EN, 1);
	msleep(100);

	gpio_request(FXN_GPIO_BT_RST_N , "FXN_GPIO_BT_RST_N");
	gpio_direction_output(FXN_GPIO_BT_RST_N, 1);   // Enable the BT_EN GPIO-102
}
*/

void __init conn_add_plat_device(void)
{
    printk(">>> conn_add_plat_device()\n");
    platform_add_devices(conn_plat_devices, ARRAY_SIZE(conn_plat_devices));
    // bluetooth_init();
    printk("<<< conn_add_plat_device()\n");
}
#endif

#ifdef CONFIG_PM
static struct omap_volt_vc_data vc_config = {
    /* MPU */
    .vdd0_on	= 1200000, /* 1.2v */
    .vdd0_onlp	= 1000000, /* 1.0v */
    .vdd0_ret	=  975000, /* 0.975v */
    .vdd0_off	=  600000, /* 0.6v */
    /* CORE */
    .vdd1_on	= 1150000, /* 1.15v */
    .vdd1_onlp	= 1000000, /* 1.0v */
    .vdd1_ret	=  975000, /* 0.975v */
    .vdd1_off	=  600000, /* 0.6v */
    
    .clksetup	= 0xff,
    .voltoffset	= 0xff,
    .voltsetup2	= 0xff,
    .voltsetup_time1 = 0xfff,
    .voltsetup_time2 = 0xfff,
};

#ifdef CONFIG_TWL4030_CORE
static struct omap_volt_pmic_info omap_pmic_mpu = { /* and iva */
    .name = "twl",
    .slew_rate = 4000,
    .step_size = 12500,
    .i2c_addr = 0x12,
    .i2c_vreg = 0x0, /* (vdd0) VDD1 -> VDD1_CORE -> VDD_MPU */
    .vsel_to_uv = omap_twl_vsel_to_uv,
    .uv_to_vsel = omap_twl_uv_to_vsel,
    .onforce_cmd = omap_twl_onforce_cmd,
    .on_cmd = omap_twl_on_cmd,
    .sleepforce_cmd = omap_twl_sleepforce_cmd,
    .sleep_cmd = omap_twl_sleep_cmd,
    .vp_config_erroroffset = 0,
    .vp_vstepmin_vstepmin = 0x01,
    .vp_vstepmax_vstepmax = 0x04,
    .vp_vlimitto_timeout_us = 0x200,
    .vp_vlimitto_vddmin = 0x14,
    .vp_vlimitto_vddmax = 0x44,
    };

static struct omap_volt_pmic_info omap_pmic_core = {
    .name = "twl",
    .slew_rate = 4000,
    .step_size = 12500,
    .i2c_addr = 0x12,
    .i2c_vreg = 0x1, /* (vdd1) VDD2 -> VDD2_CORE -> VDD_CORE */
    .vsel_to_uv = omap_twl_vsel_to_uv,
    .uv_to_vsel = omap_twl_uv_to_vsel,
    .onforce_cmd = omap_twl_onforce_cmd,
    .on_cmd = omap_twl_on_cmd,
    .sleepforce_cmd = omap_twl_sleepforce_cmd,
    .sleep_cmd = omap_twl_sleep_cmd,
    .vp_config_erroroffset = 0,
    .vp_vstepmin_vstepmin = 0x01,
    .vp_vstepmax_vstepmax = 0x04,
    .vp_vlimitto_timeout_us = 0x200,
    .vp_vlimitto_vddmin = 0x18,
    .vp_vlimitto_vddmax = 0x42,
};
#endif /* CONFIG_TWL4030_CORE */
#endif /* CONFIG_PM */


extern void __init evt_peripherals_init(void);

static inline void omap2_ramconsole_reserve_sdram(void)
{
    #ifdef CONFIG_ANDROID_RAM_CONSOLE
    reserve_bootmem(ENCORE_RAM_CONSOLE_START, ENCORE_RAM_CONSOLE_SIZE, 0);
    #endif 
}

static void __init omap_evt_map_io(void)
{
        omap2_set_globals_343x();/*omap2_set_globals_36xx();*/
        omap34xx_map_common_io();
        
        omap2_ramconsole_reserve_sdram();
}

static struct omap_board_config_kernel evt_config[] __initdata = {
};

static void __init omap_evt_init_irq(void)
{
	omap_board_config = evt_config;
	omap_board_config_size = ARRAY_SIZE(evt_config);

#if defined (CONFIG_MACH_SDRAM_HYNIX_H8MBX00U0MER0EM_OR_SAMSUNG_K4X4G303PB)
	omap2_init_common_hw(	h8mbx00u0mer0em_K4X4G303PB_sdrc_params,
				h8mbx00u0mer0em_K4X4G303PB_sdrc_params);

#elif defined(CONFIG_MACH_SDRAM_HYNIX_H8MBX00U0MER0EM)
	omap2_init_common_hw(	h8mbx00u0mer0em_sdrc_params,
				h8mbx00u0mer0em_sdrc_params);

#elif defined (CONFIG_MACH_SDRAM_SAMSUNG_K4X4G303PB)
	omap2_init_common_hw(	samsung_k4x4g303pb_sdrc_params,
				samsung_k4x4g303pb_sdrc_params);
				
#elif defined (CONFIG_MACH_SDRAM_ELPIDA_EDK4332C2PB)//select this one
	omap2_init_common_hw(	edk4332C2pb_sdrc_params,
				edk4332C2pb_sdrc_params);
#else
#error "Please select SDRAM chip."
#endif
	
        omap2_gp_clockevent_set_gptimer(1);
	omap_init_irq();

}

#ifdef CONFIG_OMAP_MUX
#warning "EVT1A port relies on the bootloader for MUX configuration."
        static struct omap_board_mux board_mux[] __initdata = {
                //OMAP3_MUX(SDMMC1_CLK, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT ),
                //OMAP3_MUX(SDMMC1_CMD, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT ),
                
                //OMAP3_MUX(SDMMC1_DAT0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT | OMAP_PIN_OUTPUT ),
                //OMAP3_MUX(SDMMC1_DAT1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT | OMAP_PIN_OUTPUT ),
                //OMAP3_MUX(SDMMC1_DAT2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT | OMAP_PIN_OUTPUT ),
                //OMAP3_MUX(SDMMC1_DAT3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT | OMAP_PIN_OUTPUT ),
                { .reg_offset = OMAP_MUX_TERMINATOR },
        };
#else
        #define board_mux       NULL
#endif

static struct ehci_hcd_omap_platform_data ehci_pdata __initconst = {
        .port_mode[0]           = 0,
        .port_mode[1]           = EHCI_HCD_OMAP_MODE_PHY,
        .port_mode[2]           = 0,       
        .phy_reset              = false,
        .reset_gpio_port[0]     = -EINVAL,
        .reset_gpio_port[1]     = 23,
        .reset_gpio_port[2]     = -EINVAL,
};

static void __init omap_evt_init(void)
{
        printk(">>> omap_evt_init\n");

        omap3_mux_init(board_mux, OMAP_PACKAGE_CBP);
        evt_peripherals_init();

        pr_info("CPU variant: %s\n", cpu_is_omap3622()? "OMAP3622": "OMAP3621");
    
#ifdef CONFIG_PM
#ifdef CONFIG_TWL4030_CORE
        omap_voltage_register_pmic(&omap_pmic_core, "core");
        omap_voltage_register_pmic(&omap_pmic_mpu, "mpu");
#endif
        omap_voltage_init_vc(&vc_config);
#endif
        //omap3xxx_hwmod_init();
        
        //omap_mux_init_gpio(64, OMAP_PIN_OUTPUT);
        usb_ehci_init(&ehci_pdata);
        
#ifdef CONFIG_WIRELESS_BCM4329_RFKILL
        conn_add_plat_device();
#endif
        
        printk("<<< omap_evt_init\n");
}


MACHINE_START(OMAP3621_EVT1A, "OMAP3621 EVT1A board")
	.phys_io        = L4_34XX_PHYS,
	.io_pg_offst    = ((L4_34XX_VIRT) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap_evt_map_io,
	.init_irq	= omap_evt_init_irq,
	.init_machine	= omap_evt_init,
	.timer		= &omap_timer,
MACHINE_END
