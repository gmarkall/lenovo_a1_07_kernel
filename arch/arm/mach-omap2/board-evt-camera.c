/*
 * linux/arch/arm/mach-omap2/board-zoom2-camera.c
 *
 * Copyright (C) 2007 Texas Instruments
 *
 * Modified from mach-omap2/board-generic.c
 *
 * Initial code: Syed Mohammed Khasim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mm.h>

#if defined(CONFIG_TWL4030_CORE) && defined(CONFIG_VIDEO_OMAP3)

#include <linux/i2c/twl.h>

#include <asm/io.h>

#include <mach/gpio.h>
#include <plat/omap-pm.h>

#include <media/v4l2-int-device.h>
#include <../drivers/media/video/omap34xxcam.h>
#include <../drivers/media/video/isp/ispreg.h>
#define DEBUG_BASE		0x08000000

#define CAMZOOM2_USE_XCLKB  	1

#define ISP_MT9D115_MCLK		72000000

/* Sensor specific GPIO signals */
#define MT9D115_PWDN_GPIO  	45
#define MT9D115_RESET_GPIO  	46

#if defined(CONFIG_VIDEO_MT9D115) || defined(CONFIG_VIDEO_MT9D115_MODULE)
#include <media/mt9d115.h>
#include <../drivers/media/video/isp/ispcsi2.h>
#define MT9D115_CSI2_CLOCK_POLARITY	0	/* +/- pin order */
#define MT9D115_CSI2_DATA0_POLARITY	0	/* +/- pin order */
#define MT9D115_CSI2_DATA1_POLARITY	0	/* +/- pin order */
#define MT9D115_CSI2_CLOCK_LANE		1	 /* Clock lane position: 1 */
#define MT9D115_CSI2_DATA0_LANE		2	 /* Data0 lane position: 2 */
#define MT9D115_CSI2_DATA1_LANE		3	 /* Data1 lane position: 3 */
#define MT9D115_CSI2_PHY_THS_TERM	2
#define MT9D115_CSI2_PHY_THS_SETTLE	23
#define MT9D115_CSI2_PHY_TCLK_TERM	0
#define MT9D115_CSI2_PHY_TCLK_MISS	1
#define MT9D115_CSI2_PHY_TCLK_SETTLE	14
#define MT9D115_BIGGEST_FRAME_BYTE_SIZE	PAGE_ALIGN(ALIGN(1600, 0x20) * 1200 * 2)
#endif

#if defined(CONFIG_VIDEO_MT9D115) || defined(CONFIG_VIDEO_MT9D115_MODULE)

static struct omap34xxcam_sensor_config mt9d115_hwc = {
	.sensor_isp  = 1,
	.capture_mem = MT9D115_BIGGEST_FRAME_BYTE_SIZE * 4,
	.ival_default	= { 1, 10 },
	.isp_if = ISP_CSIA,
};

static int mt9d115_sensor_set_prv_data(struct v4l2_int_device *s, void *priv)
{
	struct omap34xxcam_hw_config *hwc = priv;

	hwc->u.sensor		= mt9d115_hwc;
	hwc->dev_index		= 2;
	hwc->dev_minor		= 5;
	hwc->dev_type		= OMAP34XXCAM_SLAVE_SENSOR;

	return 0;
}

static struct isp_interface_config mt9d115_if_config = {
	.ccdc_par_ser 		= ISP_CSIA,
	.dataline_shift 	= 0x0,
	.hsvs_syncdetect 	= ISPCTRL_SYNC_DETECT_VSRISE,
	.strobe 		= 0x0,
	.prestrobe 		= 0x0,
	.shutter 		= 0x0,
	.wenlog 		= ISPCCDC_CFG_WENLOG_AND,
	.wait_hs_vs		= 0,
	.cam_mclk		= ISP_MT9D115_MCLK,
	.raw_fmt_in		= ISPCCDC_INPUT_FMT_RG_GB, //not used
	.u.csi.crc 		= 0x0,
	.u.csi.mode 		= 0x0,
	.u.csi.edge 		= 0x0,
	.u.csi.signalling 	= 0x0,
	.u.csi.strobe_clock_inv = 0x0,
	.u.csi.vs_edge 		= 0x0,
	.u.csi.channel 		= 0x0,
	.u.csi.vpclk 		= 0x2,
	.u.csi.data_start 	= 0x0,
	.u.csi.data_size 	= 0x0,
	.u.csi.format 		= V4L2_PIX_FMT_UYVY,//V4L2_PIX_FMT_SGRBG10,	
};


static int mt9d115_sensor_power_set(struct v4l2_int_device *s, enum v4l2_power power)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);
	struct isp_csi2_lanes_cfg lanecfg;
	struct isp_csi2_phy_cfg phyconfig;
	static enum v4l2_power previous_power = V4L2_POWER_OFF;
	static struct pm_qos_request_list *qos_request;
	int err = 0;

	switch (power) {
	case V4L2_POWER_ON:
		/* Power Up Sequence */
		printk(KERN_DEBUG "mt9d115_sensor_power_set(ON)\n");

		/*
		 * Through-put requirement:
		 * Set max OCP freq for 3630 is 200 MHz through-put
		 * is in KByte/s so 200000 KHz * 4 = 800000 KByte/s
		 */
		omap_pm_set_min_bus_tput(vdev->cam->isp,
					 OCP_INITIATOR_AGENT, 800000);

		/* Hold a constraint to keep MPU in C1 */
		omap_pm_set_max_mpu_wakeup_lat(&qos_request, 12);

		isp_csi2_reset(&isp->isp_csi2);

		lanecfg.clk.pol = MT9D115_CSI2_CLOCK_POLARITY;
		lanecfg.clk.pos = MT9D115_CSI2_CLOCK_LANE;
		lanecfg.data[0].pol = MT9D115_CSI2_DATA0_POLARITY;
		lanecfg.data[0].pos = MT9D115_CSI2_DATA0_LANE;
		lanecfg.data[1].pol = 0;
		lanecfg.data[1].pos = 0;
		lanecfg.data[2].pol = 0;
		lanecfg.data[2].pos = 0;
		lanecfg.data[3].pol = 0;
		lanecfg.data[3].pos = 0;
		isp_csi2_complexio_lanes_config(&isp->isp_csi2, &lanecfg);
		isp_csi2_complexio_lanes_update(&isp->isp_csi2, true);

		isp_csi2_ctrl_config_ecc_enable(&isp->isp_csi2, true);

		phyconfig.ths_term = MT9D115_CSI2_PHY_THS_TERM;
		phyconfig.ths_settle = MT9D115_CSI2_PHY_THS_SETTLE;
		phyconfig.tclk_term = MT9D115_CSI2_PHY_TCLK_TERM;
		phyconfig.tclk_miss = MT9D115_CSI2_PHY_TCLK_MISS;
		phyconfig.tclk_settle = MT9D115_CSI2_PHY_TCLK_SETTLE;
		isp_csi2_phy_config(&isp->isp_csi2, &phyconfig);
		isp_csi2_phy_update(&isp->isp_csi2, true);

		isp_configure_interface(vdev->cam->isp, &mt9d115_if_config);

		/* Request and configure gpio pins */
		if (gpio_request(MT9D115_RESET_GPIO, "mt9d115_rst") != 0)
		{
			return -EIO;
		}	

		if (gpio_request(MT9D115_PWDN_GPIO, "mt9d115_ps") != 0)
		{
			
			return -EIO;
		}	

		/* nRESET is active LOW. set HIGH to release reset */
		gpio_set_value(MT9D115_RESET_GPIO, 1);

		/* set to output mode */
		gpio_direction_output(MT9D115_RESET_GPIO, true);

		udelay(100);

		/* PWDN is active High. set LOW to enable power of sensor */
		gpio_set_value(MT9D115_PWDN_GPIO, 0);

		/* set to output mode */
		gpio_direction_output(MT9D115_PWDN_GPIO, true);
		
		gpio_set_value(MT9D115_PWDN_GPIO, 0);

		udelay(100);
		

		/* have to put sensor to reset to guarantee detection */
		gpio_set_value(MT9D115_RESET_GPIO, 0);
		udelay(1500);

		/* nRESET is active LOW. set HIGH to release reset */
		gpio_set_value(MT9D115_RESET_GPIO, 1);
		udelay(300);
		
		break;
	case V4L2_POWER_OFF:
	case V4L2_POWER_STANDBY:
		printk(KERN_DEBUG "mt9d115_sensor_power_set(%s)\n",
			(power == V4L2_POWER_OFF) ? "OFF" : "STANDBY");
		/* Power Down Sequence */
		isp_csi2_complexio_power(&isp->isp_csi2, ISP_CSI2_POWER_OFF);

		gpio_free(MT9D115_RESET_GPIO);
		
		udelay(100);

		/* PWDN is active High. set HIGH to disable power of sensor */
		gpio_set_value(MT9D115_PWDN_GPIO, 1);
		udelay(100);
		
		gpio_free(MT9D115_PWDN_GPIO);

		/* Remove pm constraints */
		omap_pm_set_min_bus_tput(vdev->cam->isp, OCP_INITIATOR_AGENT, 0);
		omap_pm_set_max_mpu_wakeup_lat(&qos_request, -1);

		/* Make sure not to disable the MCLK twice in a row */
		if (previous_power == V4L2_POWER_ON)
		{
			isp_disable_mclk(isp);
		}	
		break;
	}

	/* Save powerstate to know what was before calling POWER_ON. */
	previous_power = power;
	return err;
}

static u32 mt9d115_sensor_set_xclk(struct v4l2_int_device *s, u32 xclkfreq)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;

	return isp_set_xclk(vdev->cam->isp, xclkfreq, CAMZOOM2_USE_XCLKB);
}

static int mt9d115_csi2_lane_count(struct v4l2_int_device *s, int count)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	return isp_csi2_complexio_lanes_count(&isp->isp_csi2, count);
}

static int mt9d115_csi2_cfg_vp_out_ctrl(struct v4l2_int_device *s,
				       u8 vp_out_ctrl)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	return isp_csi2_ctrl_config_vp_out_ctrl(&isp->isp_csi2, vp_out_ctrl);
}

static int mt9d115_csi2_ctrl_update(struct v4l2_int_device *s, bool force_update)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	return isp_csi2_ctrl_update(&isp->isp_csi2, force_update);
}

static int mt9d115_csi2_cfg_virtual_id(struct v4l2_int_device *s, u8 ctx, u8 id)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	return isp_csi2_ctx_config_virtual_id(&isp->isp_csi2, ctx, id);
}

static int mt9d115_csi2_ctx_update(struct v4l2_int_device *s, u8 ctx,
				  bool force_update)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	return isp_csi2_ctx_update(&isp->isp_csi2, ctx, force_update);
}

static int mt9d115_csi2_calc_phy_cfg0(struct v4l2_int_device *s,
				     u32 mipiclk, u32 lbound_hs_settle,
				     u32 ubound_hs_settle)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	struct isp_device *isp = dev_get_drvdata(vdev->cam->isp);

	return isp_csi2_calc_phy_cfg0(&isp->isp_csi2, mipiclk,
				      lbound_hs_settle, ubound_hs_settle);
}

static int mt9d115_configure_interface(struct v4l2_int_device *s)
{
	struct omap34xxcam_videodev *vdev = s->u.slave->master->priv;
	
	isp_configure_interface(vdev->cam->isp, &mt9d115_if_config);
}	


struct mt9d115_platform_data zoom2_mt9d115_platform_data = {
	.power_set            = mt9d115_sensor_power_set,			//boot 1
	.priv_data_set        = mt9d115_sensor_set_prv_data,
	.set_xclk             = mt9d115_sensor_set_xclk,			//boot 1
	.csi2_lane_count      = mt9d115_csi2_lane_count,			//1
	.csi2_cfg_vp_out_ctrl = mt9d115_csi2_cfg_vp_out_ctrl,			//1
	.csi2_ctrl_update     = mt9d115_csi2_ctrl_update,			//1
	.csi2_cfg_virtual_id  = mt9d115_csi2_cfg_virtual_id,			//1
	.csi2_ctx_update      = mt9d115_csi2_ctx_update,			//1
	.csi2_calc_phy_cfg0   = mt9d115_csi2_calc_phy_cfg0,			//1
	.configure_interface   = mt9d115_configure_interface,
};
#endif

void __init zoom2_cam_init(void)
{
}
#endif
