/*
 * drivers/media/video/mt9d115.c
 *
 * Aptina mt9d115 sensor driver
 *
 *
 * Copyright (C) 2008 Hewlett Packard
 *
 * Leverage mt9p012.c
 *
 * Contacts:
 *   Atanas Filipov <afilipov@mm-sol.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/i2c.h>
#include <linux/delay.h>

#include <media/v4l2-int-device.h>
#include <media/mt9d115.h>

#include "mt9d115_regs.h"
#include "omap34xxcam.h"
#include "isp/isp.h"
#include "isp/ispcsi2.h"


#define MT9D115_DRIVER_NAME  "mt9d115"
#define MT9D115_MOD_NAME "MT9D115: "

#define I2C_M_WR 0
//static unsigned int first_time=0;	//&*&*&*CJ_20110610 for improve captrure performance

/**
 * struct mt9d115_sensor - main structure for storage of sensor information
 * @pdata: access functions and data for platform level information
 * @v4l2_int_device: V4L2 device structure structure
 * @i2c_client: iic client device structure
 * @pix: V4L2 pixel format information structure
 * @timeperframe: time per frame expressed as V4L fraction
 * @scaler:
 * @ver: mt9d115 chip version
 * @fps: frames per second value
 */
struct mt9d115_sensor {
	const struct mt9d115_platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	int scaler;
	int ver;
	int fps;
	bool resuming;
};

static struct mt9d115_sensor mt9d115;
static struct i2c_driver mt9d115sensor_i2c_driver;
static unsigned long xclk_current = MT9D115_XCLK_NOM_1;

/* list of image formats supported by mt9d115 sensor */
const static struct v4l2_fmtdesc mt9d115_formats[] = {
	{
		.description = "YUYV (YUV 4:2:2), packed",
		.pixelformat = V4L2_PIX_FMT_UYVY,	
	}
};

#define NUM_CAPTURE_FORMATS ARRAY_SIZE(mt9d115_formats)

static unsigned detect_count=0;

static enum v4l2_power current_power_state;

/* Structure of Sensor settings that change with image size */
static struct mt9d115_sensor_settings mt9d115_settings[] = {
	 /* NOTE: must be in same order as image_size array */
//*************************************************************************************	
	/* 0. 128*96 SQCIF 4:3 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 799,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 599,			//sensor
			.x_output_size = 128,			//sensor
			.y_output_size = 96,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},
	/* 1. 176*144 QCIF 11:9 max 726*594 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 0,			//sensor
//			.x_addr_end = 799,			//sensor
//			.y_addr_start = 0,			//sensor
//			.y_addr_end = 599,			//sensor
			.x_addr_start = 37,			//sensor
			.x_addr_end = 762,			//sensor
			.y_addr_start = 3,			//sensor
			.y_addr_end = 596,			//sensor
			.x_output_size = 176,			//sensor
			.y_output_size = 144,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},
	/* 2. 240*160 Fring 3:2 max 798*532 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 0,			//sensor
//			.x_addr_end = 799,			//sensor
//			.y_addr_start = 0,			//sensor
//			.y_addr_end = 599,			//sensor
			.x_addr_start = 1,			//sensor
			.x_addr_end = 798,			//sensor
			.y_addr_start = 34,			//sensor
			.y_addr_end = 565,			//sensor
			.x_output_size = 240,			//sensor
			.y_output_size = 160,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},	
	/* 3. 320*240 QVGA 4:3 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 799,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 599,			//sensor
			.x_output_size = 320,			//sensor
			.y_output_size = 240,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},

	/* 4. 352*288 CIF 11:9 max 726*594 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 0,			//sensor
//			.x_addr_end = 799,			//sensor
//			.y_addr_start = 0,			//sensor
//			.y_addr_end = 599,			//sensor
			.x_addr_start = 36,			//sensor
			.x_addr_end = 762,			//sensor
			.y_addr_start = 3,			//sensor
			.y_addr_end = 596,			//sensor
			.x_output_size = 352,			//sensor
			.y_output_size = 288,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},
	/* 5. 640*480 VGA 4:3 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 799,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 599,			//sensor
			.x_output_size = 640,			//sensor
			.y_output_size = 480,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},	
	/* 6. 720*480 NTSC 3:2 max 798*532 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 0,			//sensor
//			.x_addr_end = 799,			//sensor
//			.y_addr_start = 0,			//sensor
//			.y_addr_end = 599,			//sensor
			.x_addr_start = 1,			//sensor
			.x_addr_end = 798,			//sensor
			.y_addr_start = 34,			//sensor
			.y_addr_end = 565,			//sensor
			.x_output_size = 720,			//sensor
			.y_output_size = 480,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},			
	/* 7. 720*576 PAL 5:4 max 750*600 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 0,			//sensor
//			.x_addr_end = 799,			//sensor
//			.y_addr_start = 0,			//sensor
//			.y_addr_end = 599,			//sensor
			.x_addr_start = 25,			//sensor
			.x_addr_end = 774,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 599,			//sensor
			.x_output_size = 720,			//sensor
			.y_output_size = 576,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},		
	/* 8. 768*576 PAL 4:3 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 799,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 599,			//sensor
			.x_output_size = 768,			//sensor
			.y_output_size = 576,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},				
	/* 9. 800*480 WVGA 5:3 max 800*480 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 0,			//sensor
//			.x_addr_end = 799,			//sensor
//			.y_addr_start = 0,			//sensor
//			.y_addr_end = 599,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 799,			//sensor
			.y_addr_start = 60,			//sensor
			.y_addr_end = 539,			//sensor			
			.x_output_size = 800,			//sensor
			.y_output_size = 480,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},	
	/* 10. 800*600 SVGA 4:3 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 799,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 599,			//sensor
			.x_output_size = 800,			//sensor
			.y_output_size = 600,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},		
	/* 11. 992*560 Barcode Scanner 62:35 context A max 744*420, Context B max 1488*840 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 0,	//35		//sensor
//			.x_addr_end = 1599,	// ->924	//sensor
//			.y_addr_start = 0,	//20		//sensor
//			.y_addr_end = 1199,	// ->	521	//sensor
			.x_addr_start = 56,			//sensor
			.x_addr_end = 1543,			//sensor
			.y_addr_start = 180,			//sensor
			.y_addr_end = 1019,			//sensor
			.x_output_size = 992,			//sensor
			.y_output_size = 560,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_B,
		},
	},		
	/* 12. 1024*768 1M 4:3 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 1599,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 1199,			//sensor
			.x_output_size = 1024,			//sensor
			.y_output_size = 768,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_B,
		},
	},			
	/* 13. 1280*720 720p 16:9 context A max 800*450, Context B max max 1600*900 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 166,			//sensor
//			.x_addr_end = 1453,			//sensor
//			.y_addr_start = 246,			//sensor
//			.y_addr_end = 973,			//sensor
//			.x_addr_start = 0,			//sensor
//			.x_addr_end = 1599,			//sensor
//			.y_addr_start = 150,			//sensor
//			.y_addr_end = 1049,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 1279,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 719,			//sensor
			.x_output_size = 1280,			//sensor
			.y_output_size = 720,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_A,
		},
	},			
	/* 14. 1280*1024 1.3M 5:4 max 1500*1200 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
//			.x_addr_start = 0,			//sensor
//			.x_addr_end = 1599,			//sensor
//			.y_addr_start = 0,			//sensor
//			.y_addr_end = 1199,			//sensor
			.x_addr_start = 50,			//sensor
			.x_addr_end = 1549,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 1199,			//sensor
			.x_output_size = 1280,			//sensor
			.y_output_size = 1024,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_B,
		},
	},		
	/* 15. 1600*1200 2M 4:3 */
	{
		.clk = {
			.pre_pll_div = 1,			//csi2
			.pll_mult = 18,				//csi2
			.post_pll_div = 1,			//csi2
			.vt_pix_clk_div = 10,			//csi2
			.vt_sys_clk_div = 1,			//csi2
		},
		.mipi = {
			.data_lanes = 1,			//csi2
			.ths_prepare = 2,			//sensor
			.ths_zero = 5,				//sensor
			.ths_settle_lower = 8,			//csi2
			.ths_settle_upper = 28,			//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,			//sensor
			.x_addr_start = 0,			//sensor
			.x_addr_end = 1599,			//sensor
			.y_addr_start = 0,			//sensor
			.y_addr_end = 1199,			//sensor
			.x_output_size = 1600,			//sensor
			.y_output_size = 1200,			//sensor
			.x_even_inc = 1,			//sensor
			.x_odd_inc = 1,				//sensor
			.y_even_inc = 1,			//sensor
			.y_odd_inc = 1,				//sensor
			.v_mode_add = 0,			//sensor
			.h_mode_add = 0,			//sensor
			.h_add_ave = 0,				//sensor
			.context = CONTEXT_B,
		},
	},
};

#define MT9D115_MODES_COUNT ARRAY_SIZE(mt9d115_settings)

static unsigned isize_current = MT9D115_MODES_COUNT - 1;
static struct mt9d115_clock_freq current_clk;

struct i2c_list {
	struct i2c_msg *reg_list;
	unsigned int list_size;
};

/**
 * struct vcontrol - Video controls
 * @v4l2_queryctrl: V4L2 VIDIOC_QUERYCTRL ioctl structure
 * @current_value: current value of this control
 */
static struct vcontrol {
	struct v4l2_queryctrl qc;
	int current_value;
} mt9d115sensor_video_control[] = {
	{
		{
			.id = V4L2_CID_EXPOSURE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Exposure",
			.minimum = MT9D115_MIN_EXPOSURE,
			.maximum = MT9D115_MAX_EXPOSURE,
			.step = MT9D115_EXPOSURE_STEP,
			.default_value = MT9D115_DEF_EXPOSURE,
		},
		.current_value = MT9D115_DEF_EXPOSURE,
	},
	{
		{
			.id = V4L2_CID_COLORFX,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Camera Effect",
			.minimum = MT9D115_MIN_EFFECT,
			.maximum = MT9D115_MAX_EFFECT,
			.step = MT9D115_EFFECT_STEP,
			.default_value = MT9D115_DEF_EFFECT,
		},
		.current_value = MT9D115_DEF_EFFECT,
	},
	{
		{
			.id = V4L2_CID_DO_WHITE_BALANCE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "White Balance",
			.minimum = MT9D115_MIN_WB,
			.maximum = MT9D115_MAX_WB,
			.step = MT9D115_WB_STEP,
			.default_value = MT9D115_DEF_WB,
		},
		.current_value = MT9D115_DEF_WB,
	},
	{
		{
			.id = V4L2_CID_POWER_LINE_FREQUENCY,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Camera Antibanding",
			.minimum = MT9D115_MIN_BANDING,
			.maximum = MT9D115_MAX_BANDING,
			.step = MT9D115_BANDING_STEP,
			.default_value = MT9D115_DEF_BANDING,
		},
		.current_value = MT9D115_DEF_BANDING,
	}
};

/**
 * find_vctrl - Finds the requested ID in the video control structure array
 * @id: ID of control to search the video control array for
 *
 * Returns the index of the requested ID from the control structure array
 */
static int find_vctrl(int id)
{
	int i;

	if (id < V4L2_CID_BASE)
		return -EDOM;

	for (i = (ARRAY_SIZE(mt9d115sensor_video_control) - 1); i >= 0; i--)
		if (mt9d115sensor_video_control[i].qc.id == id)
			break;
	if (i < 0)
		i = -EINVAL;
	return i;
}

/**
 * mt9d115_read_reg - Read a value from a register in an mt9d115 sensor device
 * @client: i2c driver client structure
 * @data_length: length of data to be read
 * @reg: register address / offset
 * @val: stores the value that gets read
 *
 * Read a value from a register in an mt9d115 sensor device.
 * The value is returned in 'val'.
 * Returns zero if successful, or non-zero otherwise.
 */
static int mt9d115_read_reg(struct i2c_client *client, u16 data_length, u16 reg,
			   u32 *val)
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[4] = {0};


	if (!client->adapter)
		return -ENODEV;
	if (data_length != I2C_8BIT && data_length != I2C_16BIT
			&& data_length != I2C_32BIT)
		return -EINVAL;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = data;

	/* Write addr - high byte goes out first */
	data[0] = (u8) (reg >> 8);
	data[1] = (u8) (reg & 0xff);
	err = i2c_transfer(client->adapter, msg, 1);

	/* Read back data */
	if (err >= 0) {
		msg->len = data_length;
		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
	}
	if (err >= 0) {
		*val = 0;
		/* high byte comes first */
		if (data_length == I2C_8BIT)
			*val = data[0];
		else if (data_length == I2C_16BIT)
			*val = data[1] + (data[0] << 8);
		else
			*val = data[3] + (data[2] << 8) +
				(data[1] << 16) + (data[0] << 24);
		return 0;
	}
	v4l_err(client, "read from offset 0x%x error %d\n", reg, err);
	return err;
}

/**
 * Write a value to a register in mt9d115 sensor device.
 * @client: i2c driver client structure.
 * @reg: Address of the register to read value from.
 * @val: Value to be written to a specific register.
 * Returns zero if successful, or non-zero otherwise.
 */
static int mt9d115_write_reg(struct i2c_client *client, u16 reg, u32 val,
			    u16 data_length)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[6];
	int retries = 0;


	if (!client->adapter)
		return -ENODEV;

	if (data_length != I2C_8BIT && data_length != I2C_16BIT
			&& data_length != I2C_32BIT)
		return -EINVAL;

retry:
	msg->addr = client->addr;
	msg->flags = I2C_M_WR;
	msg->len = data_length+2;  /* add address bytes */
	msg->buf = data;

	/* high byte goes out first */
	data[0] = (u8) (reg >> 8);
	data[1] = (u8) (reg & 0xff);
	if (data_length == I2C_8BIT) {
		data[2] = val & 0xff;
	} else if (data_length == I2C_16BIT) {
		data[2] = (val >> 8) & 0xff;
		data[3] = val & 0xff;
	} else {
		data[2] = (val >> 24) & 0xff;
		data[3] = (val >> 16) & 0xff;
		data[4] = (val >> 8) & 0xff;
		data[5] = val & 0xff;
	}

	if (data_length == 1)
		dev_dbg(&client->dev, "MT9D115 Wrt:[0x%04X]=0x%02X\n",
				reg, val);
	else if (data_length == 2)
		dev_dbg(&client->dev, "MT9D115 Wrt:[0x%04X]=0x%04X\n",
				reg, val);

	err = i2c_transfer(client->adapter, msg, 1);

//printk("MT9D115 Wrt:[0x%04X]=0x%04X\n",reg, val); 

	mdelay(3);
//printk("mt9d115_write_reg write data err= %d, retries = %d\n", err, retries); 
	if (err >= 0)
		return 0;

	if (retries <= 5) {
		v4l_info(client, "Retrying I2C... %d", retries);
		retries++;
		mdelay(20);
		goto retry;
	}

	return err;
}

/**
 * Initialize a list of mt9d115 registers.
 * The list of registers is terminated by the pair of values
 * {OV3640_REG_TERM, OV3640_VAL_TERM}.
 * @client: i2c driver client structure.
 * @reglist[]: List of address of the registers to write data.
 * Returns zero if successful, or non-zero otherwise.
 */
static int mt9d115_write_regs(struct i2c_client *client,
			     const struct mt9d115_reg reglist[])
{
	int err = 0;
	const struct mt9d115_reg *list = reglist;

	while (!((list->reg == I2C_REG_TERM)
		&& (list->val == I2C_VAL_TERM))) {
		err = mt9d115_write_reg(client, list->reg,
				list->val, list->length);
		if (err)
			return err;
		list++;
	}
	return 0;
}

/**
 * mt9d115_find_size - Find the best match for a requested image capture size
 * @width: requested image width in pixels
 * @height: requested image height in pixels
 *
 * Find the best match for a requested image capture size.  The best match
 * is chosen as the nearest match that has the same number or fewer pixels
 * as the requested size, or the smallest image size if the requested size
 * has fewer pixels than the smallest image.
 * Since the available sizes are subsampled in the vertical direction only,
 * the routine will find the size with a height that is equal to or less
 * than the requested height.
 */
static unsigned mt9d115_find_size(unsigned int width, unsigned int height)
{
	unsigned isize;

	for (isize = 0; isize < MT9D115_MODES_COUNT; isize++) {
		if ((mt9d115_settings[isize].frame.y_output_size >= height) &&
		    (mt9d115_settings[isize].frame.x_output_size >= width))
			break;
	}

//	printk(KERN_DEBUG "mt9d115_find_size: Req Size=%dx%d, "
//			"Calc Size=%dx%d\n", width, height,
//			mt9d115_settings[isize].frame.x_output_size,
//			mt9d115_settings[isize].frame.y_output_size);

	return isize;
}

///**
// * Set CSI2 Virtual ID.
// * @client: i2c client driver structure
// * @id: Virtual channel ID.
// *
// * Sets the channel ID which identifies data packets sent by this device
// * on the CSI2 bus.
// **/
//static int mt9d115_set_virtual_id(struct i2c_client *client, u32 id)
//{
//printk("1 [mt9d115 Sensor]Enter %s \n", __FUNCTION__); 
//	return mt9d115_write_reg(client, MT9D115_REG_CCP2_CHANNEL_ID,
//				(0x3 & id), I2C_16BIT);
//}

/**
 * mt9d115_set_framerate - Sets framerate by adjusting frame_length_lines reg.
 * @s: pointer to standard V4L2 device structure
 * @fper: frame period numerator and denominator in seconds
 *
 * The maximum exposure time is also updated since it is affected by the
 * frame rate.
 **/
static int mt9d115_set_framerate(struct v4l2_int_device *s,
				struct v4l2_fract *fper)
{
		
	int err = 0;
	u16 isize = isize_current;
	u32 frame_length_lines, line_time_q8;
	struct mt9d115_sensor *sensor = s->priv;
	struct mt9d115_sensor_settings *ss;

	if ((fper->numerator == 0) || (fper->denominator == 0)) {
		/* supply a default nominal_timeperframe */
		fper->numerator = 1;
		fper->denominator = MT9D115_DEF_FPS;
	}

	sensor->fps = fper->denominator / fper->numerator;
	if (sensor->fps < MT9D115_MIN_FPS) {
		sensor->fps = MT9D115_MIN_FPS;
		fper->numerator = 1;
		fper->denominator = sensor->fps;
	} else if (sensor->fps > MT9D115_MAX_FPS) {
		sensor->fps = MT9D115_MAX_FPS;
		fper->numerator = 1;
		fper->denominator = sensor->fps;
	}

	ss = &mt9d115_settings[isize_current];

	line_time_q8 = ((u32)ss->frame.line_len_pck * 1000000) /
			(current_clk.vt_pix_clk >> 8); /* usec's */

	frame_length_lines = (((u32)fper->numerator * 1000000 * 256 /
			       fper->denominator)) / line_time_q8;

	/* Range check frame_length_lines */
	if (frame_length_lines > MT9D115_MAX_FRAME_LENGTH_LINES)
		frame_length_lines = MT9D115_MAX_FRAME_LENGTH_LINES;
	else if (frame_length_lines < ss->frame.frame_len_lines_min)
		frame_length_lines = ss->frame.frame_len_lines_min;

	mt9d115_settings[isize].frame.frame_len_lines = frame_length_lines;

//	printk(KERN_DEBUG "MT9D115 Set Framerate: fper=%d/%d, "
//	       "frame_len_lines=%d, max_expT=%dus\n", fper->numerator,
//	       fper->denominator, frame_length_lines, MT9D115_MAX_EXPOSURE);

	return err;
}

/**
 * mt9d115sensor_calc_xclk - Calculate the required xclk frequency
 *
 * Xclk is not determined from framerate for the MT9D115
 */
static unsigned long mt9d115sensor_calc_xclk(void)
{
	return MT9D115_XCLK_NOM_1;
}

///**
// * Sets the correct orientation based on the sensor version.
// *   IU046F2-Z   version=2  orientation=3
// *   IU046F4-2D  version>2  orientation=0
// */
//static int mt9d115_set_orientation(struct i2c_client *client, u32 ver)
//{
//	int err;
//	u8 orient;
//
//printk("1 [mt9d115 Sensor]Enter %s \n", __FUNCTION__); 
//	orient = (ver <= 0x2) ? 0x3 : 0x0;
//	err = mt9d115_write_reg(client, MT9D115_REG_IMAGE_ORIENTATION,
//			       orient, I2C_16BIT);
//	return err;
//}

/**
 * mt9d115sensor_set_exposure_time - sets exposure time per input value
 * @exp_time: exposure time to be set on device (in usec)
 * @s: pointer to standard V4L2 device structure
 * @lvc: pointer to V4L2 exposure entry in mt9d115sensor_video_controls array
 *
 * If the requested exposure time is within the allowed limits, the HW
 * is configured to use the new exposure time, and the
 * mt9d115sensor_video_control[] array is updated with the new current value.
 * The function returns 0 upon success.  Otherwise an error code is
 * returned.
 */
static int mt9d115sensor_set_exposure_time(u32 exp_time,
					  struct v4l2_int_device *s,
					  struct vcontrol *lvc)
{
	int err = 0, i;
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 coarse_int_time = 0;
	u32 line_time_q8 = 0;
	struct mt9d115_sensor_settings *ss;


	if ((current_power_state == V4L2_POWER_ON) || sensor->resuming) {
		if (exp_time < MT9D115_MIN_EXPOSURE) {
			v4l_err(client, "Exposure time %d us not within"
					" the legal range.\n", exp_time);
			v4l_err(client, "Exposure time must be between"
					" %d us and %d us\n",
					MT9D115_MIN_EXPOSURE,
					MT9D115_MAX_EXPOSURE);
			exp_time = MT9D115_MIN_EXPOSURE;
		}

		if (exp_time > MT9D115_MAX_EXPOSURE) {
			v4l_err(client, "Exposure time %d us not within"
					" the legal range.\n", exp_time);
			v4l_err(client, "Exposure time must be between"
					" %d us and %d us\n",
					MT9D115_MIN_EXPOSURE,
					MT9D115_MAX_EXPOSURE);
			exp_time = MT9D115_MAX_EXPOSURE;
		}

		ss = &mt9d115_settings[isize_current];

		line_time_q8 = ((u32)ss->frame.line_len_pck * 1000000) /
				(current_clk.vt_pix_clk >> 8); /* usec's */

		coarse_int_time = ((exp_time * 256) + (line_time_q8 >> 1)) /
				  line_time_q8;

//		if (coarse_int_time > ss->frame.frame_len_lines - 2)
//			err = mt9d115_write_reg(client,
//					       MT9D115_REG_FRAME_LEN_LINES,
//					       coarse_int_time + 2,
//					       I2C_16BIT);
//		else
//			err = mt9d115_write_reg(client,
//					       MT9D115_REG_FRAME_LEN_LINES,
//					       ss->frame.frame_len_lines,
//					       I2C_16BIT);

		//err = mt9d115_write_reg(client, MT9D115_REG_COARSE_INT_TIME,
				       //coarse_int_time, I2C_16BIT);
	}

	if (err) {
		v4l_err(client, "Error setting exposure time: %d", err);
	} else {
		i = find_vctrl(V4L2_CID_EXPOSURE);
		if (i >= 0) {
			lvc = &mt9d115sensor_video_control[i];
			lvc->current_value = exp_time;
		}
	}
	return err;
}

static int mt9d115sensor_set_exposure(u32 data,
					  struct v4l2_int_device *s,
					  struct vcontrol *lvc)
{
		

	int err = 0, i;
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	struct mt9d115_sensor_settings *ss;
	
  if(first_time)
	{
//		if(mt9d115_settings[isize_current].frame.context == CONTEXT_B) //context_B
//		{
//				mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_SEQ_MODE, I2C_16BIT);
//				mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x000E, I2C_16BIT);	
//				mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_SEQ_MODE, I2C_16BIT);
//				mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x000F, I2C_16BIT);	
//				mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_SEQ_MODE, I2C_16BIT);
//				mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x000E, I2C_16BIT);	
//				mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_SEQ_MODE, I2C_16BIT);
//				mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x000F, I2C_16BIT);	
//		}

	mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_AE_BASETARGET, I2C_16BIT);
	mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	(data*2)+60, I2C_16BIT);
	lvc->current_value = data;
//printk("[mt9d115 Sensor]Enter %s data =%d \n", __FUNCTION__,data); 
	}
	
	return err;
}

static int mt9d115sensor_set_effect(u16 mode, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
	int err = 0;
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	
        if(first_time)
	{
		switch (mode) 
		{
		case V4L2_COLORFX_NONE:
			err = mt9d115_write_regs(client, colorfx_none);
			break;
			
		case V4L2_COLORFX_BW:
			err = mt9d115_write_regs(client, colorfx_bw);
			break;

		case V4L2_COLORFX_SEPIA:
			err = mt9d115_write_regs(client, colorfx_sepia);
			break;

		case V4L2_COLORFX_NEGATIVE:
			err = mt9d115_write_regs(client, colorfx_negative);
			break;

		case V4L2_COLORFX_SOLARIZE:
			err = mt9d115_write_regs(client, colorfx_solarize);
			break;
		}
		mdelay(10);
		lvc->current_value = mode;
	}	
		return err;
}


static int mt9d115sensor_set_wb(u16 mode, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
	int err = 0;
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
        if(first_time)
        {
		switch (mode) 
		{
		case WHITE_BALANCE_AUTO:
			err = mt9d115_write_regs(client, wb_auto);
		
			break;
		case WHITE_BALANCE_INCANDESCENT:
			err = mt9d115_write_regs(client, wb_incandescent);
		
			break;
		case WHITE_BALANCE_FLUORESCENT:
			err = mt9d115_write_regs(client, wb_fluorescent);
		
			break;
		case WHITE_BALANCE_DAYLIGHT:
			err = mt9d115_write_regs(client, wb_daylight);
		
			break;
		case WHITE_BALANCE_SHADE:
		
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT:
		
			break;
		case WHITE_BALANCE_HORIZON:
		
			break;
		case WHITE_BALANCE_TUNGSTEN:
		
			break;
			
		}
		mdelay(20);
		lvc->current_value = mode;
	}	
		return err;
}


static int mt9d115sensor_set_br(u16 mode, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
	int err = 0;
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;

        if(first_time)
        {

		mt9d115_write_reg(client, 0x098C, 0xA24F, I2C_16BIT);
		mt9d115_write_reg(client, 0x0990, mode, I2C_16BIT);
		lvc->current_value = mode;
	}	
		return err;
}


static int mt9d115sensor_set_banding(u16 mode, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
	int err = 0;
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;

        if(first_time)
        {
		switch (mode) 
		{
		case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
			err = mt9d115_write_regs(client, flicker_off);
			break;
        	
		case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
			err = mt9d115_write_regs(client, flicker_50hz);
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
			err = mt9d115_write_regs(client, flicker_60hz);
			break;
		}
		mdelay(10);
		lvc->current_value = mode;

	}	
		return err;
}

/**
 * mt9d115_update_clocks - calcs sensor clocks based on sensor settings.
 * @isize: image size enum
 */
static int mt9d115_update_clocks(u32 xclk, unsigned isize)
{
	
	current_clk.vco_clk =
			xclk * mt9d115_settings[isize].clk.pll_mult /
			mt9d115_settings[isize].clk.pre_pll_div /
			mt9d115_settings[isize].clk.post_pll_div;

	current_clk.vt_pix_clk = current_clk.vco_clk * 2 /
			(mt9d115_settings[isize].clk.vt_pix_clk_div *
			mt9d115_settings[isize].clk.vt_sys_clk_div);

	if (mt9d115_settings[isize].mipi.data_lanes == 2)
		current_clk.mipi_clk = current_clk.vco_clk;
	else
		current_clk.mipi_clk = current_clk.vco_clk / 2;

	current_clk.ddr_clk = current_clk.mipi_clk / 2;

//	printk(KERN_DEBUG "MT9D115: xclk=%u, vco_clk=%u, "
//		"vt_pix_clk=%u,  mipi_clk=%u,  ddr_clk=%u\n",
//		xclk, current_clk.vco_clk, current_clk.vt_pix_clk,
//		current_clk.mipi_clk, current_clk.ddr_clk);

	return 0;
}

/**
 * mt9d115_setup_pll - initializes sensor PLL registers.
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int mt9d115_setup_pll(struct v4l2_int_device *s)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;	
	unsigned isize = isize_current;
	int err;

	err = mt9d115_write_reg(client, MT9D115_REG_SW_RESET, 0x50, I2C_16BIT);
	mdelay(5);

	err = mt9d115_write_regs(client, initial_pll);
	mdelay(10);

	return 0;
}

/**
 * mt9d115_setup_mipi - initializes sensor & isp MIPI registers.
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int mt9d115_setup_mipi(struct v4l2_int_device *s, unsigned isize)
{
		
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;


//	mdelay(200);


	/* Set number of lanes in sensor */
//	if (mt9d115_settings[isize].mipi.data_lanes == 2)
//		mt9d115_write_reg(client, MT9D115_REG_RGLANESEL, 0x00, I2C_16BIT);
//	else
//		mt9d115_write_reg(client, MT9D115_REG_RGLANESEL, 0x01, I2C_16BIT);

	/* Set number of lanes in isp */
	sensor->pdata->csi2_lane_count(s,
				       mt9d115_settings[isize].mipi.data_lanes);

	/* Send settings to ISP-CSI2 Receiver PHY */
	sensor->pdata->csi2_calc_phy_cfg0(s, current_clk.mipi_clk,
		mt9d115_settings[isize].mipi.ths_settle_lower,
		mt9d115_settings[isize].mipi.ths_settle_upper);

//	/* Dump some registers for debug purposes */
//	printk(KERN_DEBUG "imx:THSPREPARE=0x%02X\n",
//		mt9d115_settings[isize].mipi.ths_prepare);
//	printk(KERN_DEBUG "imx:THSZERO=0x%02X\n",
//		mt9d115_settings[isize].mipi.ths_zero);
//	printk(KERN_DEBUG "imx:LANESEL=0x%02X\n",
//		(mt9d115_settings[isize].mipi.data_lanes == 2) ? 0 : 1);

	return 0;
}

/**
 * mt9d115_configure_frame - initializes image frame registers
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int mt9d115_configure_frame(struct i2c_client *client, unsigned isize)
{
	u32 val;

printk("(mt9d115.c)(%s)%d*%d = %d  \n",__FUNCTION__,mt9d115_settings[isize].frame.x_output_size,mt9d115_settings[isize].frame.y_output_size,
(mt9d115_settings[isize].frame.x_output_size * mt9d115_settings[isize].frame.y_output_size));		

	if(mt9d115_settings[isize].frame.context == CONTEXT_A) //context_A
	{
printk("[Jimmy_MSG](mt9d115.c)(%s)context_A isize = %d\n",__FUNCTION__,isize);

			if(isize == 13) 
			{
				mt9d115_write_regs(client, res720p_1280x720);
			}	

			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X0_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.x_addr_start, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X1_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.x_addr_end, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y0_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.y_addr_start, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y1_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.y_addr_end, I2C_16BIT);
  		
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_OUTPUT_WIDTH_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.x_output_size, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_OUTPUT_HEIGHT_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.y_output_size, I2C_16BIT);
			
//			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
//			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x0006, I2C_16BIT);
//			mdelay(50);
//			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
//			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_REFRESH, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_PREVIEW, I2C_16BIT);

			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CAP_MODE, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x0000, I2C_16BIT);
  		
//&*&*&*CJ1_20110610 for improve captrure performance
			mdelay(50);
//			if(isize == 13)
//				mdelay(200);
//			else
//				mdelay(100);
//&*&*&*CJ2_20110610 for improve captrure performance
 	}
	else	//context_B
	{
printk("[Jimmy_MSG](mt9d115.c)(%s)context_B  \n",__FUNCTION__);

			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X0_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.x_addr_start, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X1_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.x_addr_end, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y0_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.y_addr_start, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y1_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.y_addr_end, I2C_16BIT);
  		
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_OUTPUT_WIDTH_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.x_output_size, I2C_16BIT);
			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_OUTPUT_HEIGHT_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	mt9d115_settings[isize].frame.y_output_size, I2C_16BIT);
			
//			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
//			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x0006, I2C_16BIT);
//			mdelay(50);
//			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
//			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_REFRESH, I2C_16BIT);
 		
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_CAPTURE, I2C_16BIT);

			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CAP_MODE, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x0002, I2C_16BIT);
  		
//			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
//			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_CAPTURE, I2C_16BIT);
			mdelay(50);

	}

//		mt9d115_read_reg(client, I2C_16BIT, 0x3040, &val);///&*&*&*JJ test
//		mt9d115_write_reg(client, 0x3040, (val&(~0x3))|0x3, I2C_16BIT);//&*&*&*JJ test
	
	return 0;
}

/**
 * mt9d115_configure - Configure the mt9d115 for the specified image mode
 * @s: pointer to standard V4L2 device structure
 *
 * Configure the mt9d115 for a specified image size, pixel format, and frame
 * period.  xclk is the frequency (in Hz) of the xclk input to the mt9d115.
 * fper is the frame period (in seconds) expressed as a fraction.
 * Returns zero if successful, or non-zero otherwise.
 * The actual frame period is returned in fper.
 */
static int mt9d115_configure(struct v4l2_int_device *s)
{
		
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	unsigned isize = isize_current;
	int err, i;
	struct vcontrol *lvc = NULL;
 	u32 status;

   if(first_time==0)		//&*&*&*CJ_20110610 for improve captrure performance
    {

printk("[mt9d115 Sensor]Enter %s \n", __FUNCTION__); 
	err = mt9d115_write_reg(client, MT9D115_REG_SW_RESET, 0x50, I2C_16BIT);
	mdelay(5);

	err = mt9d115_write_regs(client, initial_pll);
	mdelay(50);
	err = mt9d115_write_regs(client, initial_list);
	mdelay(50);
//	mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
//	mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x0006, I2C_16BIT);
//	mdelay(10);
//	mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
//	mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_REFRESH, I2C_16BIT);
//	mdelay(10);
//	err = mt9d115_write_regs(client, verticalflip);
//	mdelay(10);
//	err = mt9d115_write_regs(client, init_pixelformat);
////	mdelay(10);
	
	mt9d115_setup_mipi(s, isize);

	mdelay(10);
 	first_time=1;
//&*&*&*CJ1_20110610 for improve captrure performance
    }
    else
    {
printk("[mt9d115 Sensor]Enter %s initial_pll\n", __FUNCTION__); 
//	err = mt9d115_write_regs(client, initial_pll);
//	mdelay(50);
	err =mt9d115_read_reg(client, I2C_16BIT, 0x3400, &status);
	status = status | 0x0003;
	mt9d115_write_reg(client, 0x3400, status, I2C_16BIT);
    }		
//&*&*&*CJ2_20110610 for improve captrure performance

	/* configure image size and pixel format */
	mt9d115_configure_frame(client, isize);

//	mt9d115_set_orientation(client, sensor->ver);
	first_time=1;

	sensor->pdata->csi2_cfg_vp_out_ctrl(s, 2);
	sensor->pdata->csi2_ctrl_update(s, false);

	sensor->pdata->csi2_cfg_virtual_id(s, 0, MT9D115_CSI2_VIRTUAL_ID);
	sensor->pdata->csi2_ctx_update(s, 0, false);
//	mt9d115_set_virtual_id(client, MT9D115_CSI2_VIRTUAL_ID);

	/* Set initial exposure and gain */
	i = find_vctrl(V4L2_CID_EXPOSURE);
	if (i >= 0) {
		lvc = &mt9d115sensor_video_control[i];
//		mt9d115sensor_set_exposure((lvc->current_value*2)+60, sensor->v4l2_int_device, lvc);
		mt9d115sensor_set_exposure(lvc->current_value, sensor->v4l2_int_device, lvc);
//		mt9d115sensor_set_exposure_time(lvc->current_value,
//					sensor->v4l2_int_device, lvc);
	}
	i = find_vctrl(V4L2_CID_COLORFX);
	if (i >= 0) {
		lvc = &mt9d115sensor_video_control[i];
		err = mt9d115sensor_set_effect(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}
	i = find_vctrl(V4L2_CID_DO_WHITE_BALANCE);
	if (i >= 0) {
		lvc = &mt9d115sensor_video_control[i];
		err = mt9d115sensor_set_wb(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}
	i = find_vctrl(V4L2_CID_POWER_LINE_FREQUENCY);
	if (i >= 0) {
		lvc = &mt9d115sensor_video_control[i];
		err = mt9d115sensor_set_banding(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}

	mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
	mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x0006, I2C_16BIT);
	mdelay(50);
	mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
	mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_REFRESH, I2C_16BIT);
	mdelay(50);


	err =mt9d115_read_reg(client, I2C_16BIT, 0x3400, &status);
	status = status & 0xfffc;
	mt9d115_write_reg(client, 0x3400, status, I2C_16BIT);
	mdelay(50);

	/* configure streaming ON */
	//err = mt9d115_write_reg(client, MT9D115_REG_MODE_SELECT, 0x01, I2C_16BIT);

printk("[mt9d115 Sensor]Enter %s first_time =%d ok\n", __FUNCTION__,first_time); 
	return err;
}

/**
 * mt9d115_detect - Detect if an mt9d115 is present, and if so which revision
 * @client: pointer to the i2c client driver structure
 *
 * Detect if an mt9d115 is present, and if so which revision.
 * A device is considered to be detected if the manufacturer ID (MIDH and MIDL)
 * and the product ID (PID) registers match the expected values.
 * Any value of the version ID (VER) register is accepted.
 * Returns a negative error number if no device is detected, or the
 * non-negative value of the version ID register if a device is detected.
 */
static int mt9d115_detect(struct i2c_client *client)
{
		
	u32 model_id;
	int i=0;
	struct mt9d115_sensor *sensor;

	if (!client)
		return -ENODEV;

	sensor = i2c_get_clientdata(client);
	if(detect_count)
	{
	for(i =0; i<100 ; i++)
	   mt9d115_read_reg(client, I2C_16BIT, MT9D115_REG_MODEL_ID, &model_id);
	}   
	
	detect_count++;
	
	if (mt9d115_read_reg(client, I2C_16BIT, MT9D115_REG_MODEL_ID, &model_id))
		return -ENODEV;

	v4l_info(client, "model id detected 0x%x\n",model_id);
	
	
	if ((model_id != MT9D115_MOD_ID)) {
		/* We didn't read the values we expected, so
		 * this must not be an MT9D115.
		 */

		v4l_warn(client, "model id mismatch 0x%x\n",model_id);

		return -ENODEV;
	}
//	return rev;
	return 0;
}

/**
 * ioctl_queryctrl - V4L2 sensor interface handler for VIDIOC_QUERYCTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @qc: standard V4L2 VIDIOC_QUERYCTRL ioctl structure
 *
 * If the requested control is supported, returns the control information
 * from the mt9d115sensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_queryctrl(struct v4l2_int_device *s, struct v4l2_queryctrl *qc)
{
	int i;
	
	i = find_vctrl(qc->id);
	if (i == -EINVAL)
		qc->flags = V4L2_CTRL_FLAG_DISABLED;

	if (i < 0)
		return -EINVAL;

	*qc = mt9d115sensor_video_control[i].qc;
	return 0;
}

/**
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the mt9d115sensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_g_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	struct vcontrol *lvc;
	int i;

	i = find_vctrl(vc->id);
	if (i < 0)
		return -EINVAL;
	lvc = &mt9d115sensor_video_control[i];

	switch (vc->id) {
	case  V4L2_CID_EXPOSURE:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_COLORFX:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_DO_WHITE_BALANCE:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		vc->value = lvc->current_value;
		break;
	}

	return 0;
}

/**
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the mt9d115sensor_video_control[] array).
 * Otherwise, * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	int retval = -EINVAL;
	int i;
	struct vcontrol *lvc;

	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;

	i = find_vctrl(vc->id);
	
	if (i < 0)
		return -EINVAL;
	lvc = &mt9d115sensor_video_control[i];

	switch (vc->id) {
	case V4L2_CID_EXPOSURE:
//		retval = mt9d115sensor_set_exposure_time(vc->value, s, lvc);
//		retval = mt9d115sensor_set_exposure((vc->value*2)+60, s, lvc);
		retval = mt9d115sensor_set_exposure(vc->value, s, lvc);
		break;
	case V4L2_CID_COLORFX:
		retval = mt9d115sensor_set_effect(vc->value, s, lvc);
        	if(first_time)
        	{
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_REFRESH, I2C_16BIT);
		}	
		break;
	case V4L2_CID_DO_WHITE_BALANCE:
		retval = mt9d115sensor_set_wb(vc->value, s, lvc);
        	if(first_time)
        	{
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_REFRESH, I2C_16BIT);
		}	
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		retval = mt9d115sensor_set_banding(vc->value, s, lvc);
		break;
	}

	return retval;
}

/**
 * ioctl_enum_fmt_cap - Implement the CAPTURE buffer VIDIOC_ENUM_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @fmt: standard V4L2 VIDIOC_ENUM_FMT ioctl structure
 *
 * Implement the VIDIOC_ENUM_FMT ioctl for the CAPTURE buffer type.
 */
static int ioctl_enum_fmt_cap(struct v4l2_int_device *s,
			      struct v4l2_fmtdesc *fmt)
{
	int index = fmt->index;
	enum v4l2_buf_type type = fmt->type;

	
	memset(fmt, 0, sizeof(*fmt));
	fmt->index = index;
	fmt->type = type;

	switch (fmt->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (index >= NUM_CAPTURE_FORMATS)
			return -EINVAL;
	break;
	default:
		return -EINVAL;
	}

	fmt->flags = mt9d115_formats[index].flags;
	strlcpy(fmt->description, mt9d115_formats[index].description,
					sizeof(fmt->description));
	fmt->pixelformat = mt9d115_formats[index].pixelformat;
	

	return 0;
}

/**
 * ioctl_try_fmt_cap - Implement the CAPTURE buffer VIDIOC_TRY_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_TRY_FMT ioctl structure
 *
 * Implement the VIDIOC_TRY_FMT ioctl for the CAPTURE buffer type.  This
 * ioctl is used to negotiate the image capture size and pixel format
 * without actually making it take effect.
 */
static int ioctl_try_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	unsigned isize;
	int ifmt;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct mt9d115_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix2 = &sensor->pix;
	struct i2c_client *client = sensor->i2c_client;

	isize = mt9d115_find_size(pix->width, pix->height);
	isize_current = isize;

	pix->width = mt9d115_settings[isize].frame.x_output_size;
	pix->height = mt9d115_settings[isize].frame.y_output_size;
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (pix->pixelformat == mt9d115_formats[ifmt].pixelformat)
			break;
	}
	if (ifmt == NUM_CAPTURE_FORMATS)
		ifmt = 0;
	pix->pixelformat = mt9d115_formats[ifmt].pixelformat;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->priv = 0;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	*pix2 = *pix;
	return 0;
}

/**
 * ioctl_s_fmt_cap - V4L2 sensor interface handler for VIDIOC_S_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_S_FMT ioctl structure
 *
 * If the requested format is supported, configures the HW to use that
 * format, returns error code if format not supported or HW can't be
 * correctly configured.
 */
static int ioctl_s_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int rval;

	rval = ioctl_try_fmt_cap(s, f);
	if (rval)
		return rval;
	else
		sensor->pix = *pix;


	return rval;
}

/**
 * ioctl_g_fmt_cap - V4L2 sensor interface handler for ioctl_g_fmt_cap
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 v4l2_format structure
 *
 * Returns the sensor's current pixel format in the v4l2_format
 * parameter.
 */
static int ioctl_g_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
		
	struct mt9d115_sensor *sensor = s->priv;
	f->fmt.pix = sensor->pix;

	return 0;
}

/**
 * ioctl_g_pixclk - V4L2 sensor interface handler for ioctl_g_pixclk
 * @s: pointer to standard V4L2 device structure
 * @pixclk: pointer to unsigned 32 var to store pixelclk in HZ
 *
 * Returns the sensor's current pixel clock in HZ
 */
static int ioctl_priv_g_pixclk(struct v4l2_int_device *s, u32 *pixclk)
{
		
	*pixclk = current_clk.vt_pix_clk;

	return 0;
}

/**
 * ioctl_g_activesize - V4L2 sensor interface handler for ioctl_g_activesize
 * @s: pointer to standard V4L2 device structure
 * @pix: pointer to standard V4L2 v4l2_pix_format structure
 *
 * Returns the sensor's current active image basesize.
 */
static int ioctl_priv_g_activesize(struct v4l2_int_device *s,
				   struct v4l2_rect *pix)
{
		
	struct mt9d115_frame_settings *frm;

	frm = &mt9d115_settings[isize_current].frame;
	pix->left = frm->x_addr_start /
		((frm->x_even_inc + frm->x_odd_inc) / 2);
	pix->top = frm->y_addr_start /
		((frm->y_even_inc + frm->y_odd_inc) / 2);
	pix->width = ((frm->x_addr_end + 1) - frm->x_addr_start) /
		((frm->x_even_inc + frm->x_odd_inc) / 2);
	pix->height = ((frm->y_addr_end + 1) - frm->y_addr_start) /
		((frm->y_even_inc + frm->y_odd_inc) / 2);

	return 0;
}

/**
 * ioctl_g_fullsize - V4L2 sensor interface handler for ioctl_g_fullsize
 * @s: pointer to standard V4L2 device structure
 * @pix: pointer to standard V4L2 v4l2_pix_format structure
 *
 * Returns the sensor's biggest image basesize.
 */
static int ioctl_priv_g_fullsize(struct v4l2_int_device *s,
				 struct v4l2_rect *pix)
{
	
	struct mt9d115_frame_settings *frm;

	frm = &mt9d115_settings[isize_current].frame;
	pix->left = 0;
	pix->top = 0;
	pix->width = frm->line_len_pck;
	pix->height = frm->frame_len_lines;

	return 0;
}

/**
 * ioctl_g_pixelsize - V4L2 sensor interface handler for ioctl_g_pixelsize
 * @s: pointer to standard V4L2 device structure
 * @pix: pointer to standard V4L2 v4l2_pix_format structure
 *
 * Returns the sensor's configure pixel size.
 */
static int ioctl_priv_g_pixelsize(struct v4l2_int_device *s,
				  struct v4l2_rect *pix)
{
	struct mt9d115_frame_settings *frm;

	frm = &mt9d115_settings[isize_current].frame;
	pix->left = 0;
	pix->top = 0;
	pix->width = (frm->x_even_inc + frm->x_odd_inc) / 2;
	pix->height = (frm->y_even_inc + frm->y_odd_inc) / 2;
	
	return 0;
}

/**
 * ioctl_priv_g_pixclk_active - V4L2 sensor interface handler
 *                              for ioctl_priv_g_pixclk_active
 * @s: pointer to standard V4L2 device structure
 * @pixclk: pointer to unsigned 32 var to store pixelclk in HZ
 *
 * The function calculates optimal pixel clock which can use
 * the data received from sensor complying with all the
 * peculiarities of the sensors and the currently selected mode.
 */
static int ioctl_priv_g_pixclk_active(struct v4l2_int_device *s, u32 *pixclk)
{
		
	struct v4l2_rect full, active;
	u32 sens_pixclk;

	ioctl_priv_g_activesize(s, &active);
	ioctl_priv_g_fullsize(s, &full);
	ioctl_priv_g_pixclk(s, &sens_pixclk);

	*pixclk = (sens_pixclk / full.width) * active.width;

	return 0;
}

/**
 * ioctl_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the sensor's video CAPTURE parameters.
 */
static int ioctl_g_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct v4l2_captureparm *cparm = &a->parm.capture;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(a, 0, sizeof(*a));
	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	cparm->capability = V4L2_CAP_TIMEPERFRAME;
	cparm->timeperframe = sensor->timeperframe;

	return 0;
}

/**
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int ioctl_s_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	
	struct mt9d115_sensor *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	int err = 0;

	sensor->timeperframe = *timeperframe;
	mt9d115sensor_calc_xclk();
	mt9d115_update_clocks(xclk_current, isize_current);
	err = mt9d115_set_framerate(s, &sensor->timeperframe);
	*timeperframe = sensor->timeperframe;

	return err;
}


/**
 * ioctl_g_priv - V4L2 sensor interface handler for vidioc_int_g_priv_num
 * @s: pointer to standard V4L2 device structure
 * @p: void pointer to hold sensor's private data address
 *
 * Returns device's (sensor's) private data area address in p parameter
 */
static int ioctl_g_priv(struct v4l2_int_device *s, void *p)
{
	struct mt9d115_sensor *sensor = s->priv;

	return sensor->pdata->priv_data_set(s, p);

}

static int ioctl_priv_g_crop(struct v4l2_int_device *s, struct v4l2_crop *f)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u32 data_x0,data_x1,data_y0,data_y1;
	int rval=0;
		
printk("[mt9d115 Sensor]Enter %s \n", __FUNCTION__); 
		return 0;
	memset(&f->c, 0, sizeof(struct v4l2_rect));		

	if(mt9d115_settings[isize_current].frame.context == 1) //context_A
	{
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X0_A, I2C_16BIT);
			mt9d115_read_reg(client, I2C_16BIT, MT9D115_MCU_DATA_0, &data_x0);//[MODE_CROP_X0_A]
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X1_A, I2C_16BIT);
			mt9d115_read_reg(client, I2C_16BIT, MT9D115_MCU_DATA_0, &data_x1);//[MODE_CROP_X1_A]
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y0_A, I2C_16BIT);
			mt9d115_read_reg(client, I2C_16BIT, MT9D115_MCU_DATA_0, &data_y0);//[MODE_CROP_Y0_A]
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y1_A, I2C_16BIT);
			mt9d115_read_reg(client, I2C_16BIT, MT9D115_MCU_DATA_0, &data_y1);//[MODE_CROP_Y1_A]
	}
	else //context_B
	{
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X0_B, I2C_16BIT);
			mt9d115_read_reg(client, I2C_16BIT, MT9D115_MCU_DATA_0, &data_x0);//[MODE_CROP_X0_B]
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X1_B, I2C_16BIT);
			mt9d115_read_reg(client, I2C_16BIT, MT9D115_MCU_DATA_0, &data_x1);//[MODE_CROP_X1_B]
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y0_B, I2C_16BIT);
			mt9d115_read_reg(client, I2C_16BIT, MT9D115_MCU_DATA_0, &data_y0);//[MODE_CROP_Y0_B]
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y1_B, I2C_16BIT);
			mt9d115_read_reg(client, I2C_16BIT, MT9D115_MCU_DATA_0, &data_y1);//[MODE_CROP_Y1_B]
	}

	f->c.left 	= data_x0;
	f->c.top 		= data_y0;
	f->c.width 	= data_x1 - data_x0 + 1;
	f->c.height = data_y1 - data_y0 + 1;

//printk("[mt9d115 Sensor]Enter %s, left = %d, top=%d, width = %d,height=%d \n", __FUNCTION__,f->c.left,f->c.top,f->c.width,f->c.height); 
	return rval;
}


static int ioctl_priv_s_crop(struct v4l2_int_device *s, struct v4l2_crop *f)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval=0;
	u32 c_right=0,c_bottom=0;
	
printk("[mt9d115 Sensor]Enter %s\n", __FUNCTION__); 
		return 0;

	
	if(mt9d115_settings[isize_current].frame.context == 1) //context_A
	{
		
			c_right = f->c.left + f->c.width - 1;
			c_bottom = f->c.top + f->c.height - 1;
//printk("[mt9d115 Sensor]context A left = %d, top=%d, width = %d,height=%d \n",f->c.left,f->c.top,f->c.width,f->c.height); 
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X0_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	f->c.left, I2C_16BIT);			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X1_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	c_right, I2C_16BIT);			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y0_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	f->c.top, I2C_16BIT);			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y1_A, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	c_bottom, I2C_16BIT);  				
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	0x0006, I2C_16BIT);
			mdelay(10);
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_REFRESH, I2C_16BIT);
			mdelay(10);
	}
	else //context_B
	{
		
			c_right = f->c.left + f->c.width - 1;
			c_bottom = f->c.top + f->c.height - 1;
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X0_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	f->c.left, I2C_16BIT);			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_X1_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	c_right, I2C_16BIT);			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y0_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	f->c.top, I2C_16BIT);			
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_MODE_CROP_Y1_B, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	c_bottom, I2C_16BIT);  				
			mt9d115_write_reg(client, MT9D115_MCU_ADDRESS, MT9D115_MCU_SEQ_CMD, I2C_16BIT);
			mt9d115_write_reg(client, MT9D115_MCU_DATA_0,	MT9D115_MCU_DATA_REFRESH, I2C_16BIT);
	}
	mdelay(10);
			
	return rval;
}


static int __mt9d115_power_off_standby(struct v4l2_int_device *s,
				      enum v4l2_power on)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

    if(first_time==0)		//&*&*&*CJ_20110610 for improve captrure performance
    {
//	first_time=0;		//&*&*&*CJ_20110610 for improve captrure performance
	rval = sensor->pdata->power_set(s, on);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			MT9D115_DRIVER_NAME " sensor\n");
		return rval;
	}

	sensor->pdata->set_xclk(s, 0);
    }				//&*&*&*CJ_20110610 for improve captrure performance
	return 0;
}

static int mt9d115_power_off(struct v4l2_int_device *s)
{
	return __mt9d115_power_off_standby(s, V4L2_POWER_OFF);
}

static int mt9d115_power_standby(struct v4l2_int_device *s)
{
	//mt9d115_setup_pll(s);//&*&*&*JJ1
	return __mt9d115_power_off_standby(s, V4L2_POWER_STANDBY);
}

static int mt9d115_power_on(struct v4l2_int_device *s)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

    if(first_time==0)		//&*&*&*CJ_20110610 for improve captrure performance
    {
	sensor->pdata->set_xclk(s, xclk_current);

	rval = sensor->pdata->power_set(s, V4L2_POWER_ON);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			MT9D115_DRIVER_NAME " sensor\n");
		sensor->pdata->set_xclk(s, 0);
		return rval;
	}
    }				//&*&*&*CJ_20110610 for improve captrure performance

	return 0;
}

/**
 * ioctl_s_power - V4L2 sensor interface handler for vidioc_int_s_power_num
 * @s: pointer to standard V4L2 device structure
 * @on: power state to which device is to be set
 *
 * Sets devices power state to requrested state, if possible.
 */
static int ioctl_s_power(struct v4l2_int_device *s, enum v4l2_power on)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct vcontrol *lvc;
	int i;
//	struct i2c_client *client = sensor->i2c_client;

	switch (on) {
	case V4L2_POWER_ON:
		mt9d115_power_on(s);
		if (current_power_state == V4L2_POWER_STANDBY) {
			sensor->resuming = true;
			mt9d115_configure(s);
			sensor->pdata->configure_interface(s);
//			isp_configure_interface(vdev->cam->isp, &mt9d115_if_config);	//&*&*&*CJ_20110610 for improve captrure performance
		}
		break;
	case V4L2_POWER_OFF:
		mt9d115_power_off(s);

		/* Reset defaults for controls */
		i = find_vctrl(V4L2_CID_EXPOSURE);
		if (i >= 0) {
			lvc = &mt9d115sensor_video_control[i];
			lvc->current_value = MT9D115_DEF_EXPOSURE;
		}
		i = find_vctrl(V4L2_CID_COLORFX);
		if (i >= 0) {
			lvc = &mt9d115sensor_video_control[i];
			lvc->current_value = MT9D115_DEF_EFFECT;
		}
		i = find_vctrl(V4L2_CID_DO_WHITE_BALANCE);
		if (i >= 0) {
			lvc = &mt9d115sensor_video_control[i];
			lvc->current_value = MT9D115_DEF_WB;
		}
		i = find_vctrl(V4L2_CID_POWER_LINE_FREQUENCY);
		if (i >= 0) {
			lvc = &mt9d115sensor_video_control[i];
			lvc->current_value = MT9D115_DEF_BANDING;
		}
		break;
	case V4L2_POWER_STANDBY:
		mt9d115_power_standby(s);
		
		break;
	}

	sensor->resuming = false;
	current_power_state = on;
	return 0;
}

/**
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 *
 * Initialize the sensor device (call mt9d115_configure())
 */
static int ioctl_init(struct v4l2_int_device *s)
{
	return 0;
}

/**
 * ioctl_dev_exit - V4L2 sensor interface handler for vidioc_int_dev_exit_num
 * @s: pointer to standard V4L2 device structure
 *
 * Delinitialise the dev. at slave detach.  The complement of ioctl_dev_init.
 */
static int ioctl_dev_exit(struct v4l2_int_device *s)
{
	return 0;
}

/**
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.  Returns 0 if
 * mt9d115 device could be found, otherwise returns appropriate error.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct mt9d115_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int err;

	err = mt9d115_power_on(s);
	if (err)
		return -ENODEV;

	err = mt9d115_detect(client);
	if (err < 0) {
		v4l_err(client, "Unable to detect "
				MT9D115_DRIVER_NAME " sensor\n");

		/*
		 * Turn power off before leaving the function.
		 * If not, CAM Pwrdm will be ON which is not needed
		 * as there is no sensor detected.
		 */
		mt9d115_power_off(s);

		return err;
	}
	sensor->ver = err;
	v4l_info(client, MT9D115_DRIVER_NAME
		" chip version 0x%02x detected\n", sensor->ver);

	err = mt9d115_power_off(s);
	if (err)
		return -ENODEV;

	return 0;
}

/**
 * ioctl_enum_framesizes - V4L2 sensor if handler for vidioc_int_enum_framesizes
 * @s: pointer to standard V4L2 device structure
 * @frms: pointer to standard V4L2 framesizes enumeration structure
 *
 * Returns possible framesizes depending on choosen pixel format
 **/
static int ioctl_enum_framesizes(struct v4l2_int_device *s,
				 struct v4l2_frmsizeenum *frms)
{
	int ifmt;
	struct mt9d115_sensor *sensor = s->priv;
	
//printk(KERN_DEBUG "(mt9d115.c)(%s)sensor->pix.width=%d,sensor->pix.height=%d  \n",__FUNCTION__,sensor->pix.width,sensor->pix.height);
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frms->pixel_format == mt9d115_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frms->index >= MT9D115_MODES_COUNT)
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width =
		mt9d115_settings[frms->index].frame.x_output_size;
	frms->discrete.height =
		mt9d115_settings[frms->index].frame.y_output_size;

	return 0;
}

static const struct v4l2_fract mt9d115_frameintervals[] = {
	{ .numerator = 3, .denominator = 30 },
	{ .numerator = 2, .denominator = 25 },
	{ .numerator = 1, .denominator = 15 },
	{ .numerator = 1, .denominator = 20 },
	{ .numerator = 1, .denominator = 25 },
	{ .numerator = 1, .denominator = 30 },
};

static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
				     struct v4l2_frmivalenum *frmi)
{
	int ifmt;

	/* Check that the requested format is one we support */
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frmi->pixel_format == mt9d115_formats[ifmt].pixelformat)
			break;
	}

	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frmi->index >= ARRAY_SIZE(mt9d115_frameintervals))
		return -EINVAL;

	/* Make sure that the 2MP size reports a max of 10fps */
	if (frmi->width == MT9D115_IMAGE_WIDTH_MAX &&
	    frmi->height == MT9D115_IMAGE_HEIGHT_MAX) {
		if (frmi->index != 0)
			return -EINVAL;
	}

	frmi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	frmi->discrete.numerator =
				mt9d115_frameintervals[frmi->index].numerator;
	frmi->discrete.denominator =
				mt9d115_frameintervals[frmi->index].denominator;

	return 0;
}

static struct v4l2_int_ioctl_desc mt9d115_ioctl_desc[] = {
	{ .num = vidioc_int_enum_framesizes_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_framesizes},	//1
	{ .num = vidioc_int_enum_frameintervals_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_frameintervals},	//1
	{ .num = vidioc_int_dev_init_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_dev_init},		//boot
	{ .num = vidioc_int_dev_exit_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_dev_exit},
	{ .num = vidioc_int_s_power_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_power },		//1
	{ .num = vidioc_int_g_priv_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_priv },		//1
	{ .num = vidioc_int_init_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_init },
	{ .num = vidioc_int_enum_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_fmt_cap },		//1
	{ .num = vidioc_int_try_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_try_fmt_cap },		//1
	{ .num = vidioc_int_g_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_fmt_cap },		//boot 1
	{ .num = vidioc_int_s_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_fmt_cap },		//1
	{ .num = vidioc_int_g_parm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_parm },		//1
	{ .num = vidioc_int_s_parm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_parm },		//1
	{ .num = vidioc_int_queryctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_queryctrl },
	{ .num = vidioc_int_g_ctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_ctrl },		//1
	{ .num = vidioc_int_s_ctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_ctrl },		//1
	{ .num = vidioc_int_priv_g_pixclk_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_pixclk },		//1
	{ .num = vidioc_int_priv_g_activesize_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_activesize },	//1
	{ .num = vidioc_int_priv_g_fullsize_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_fullsize },	//1
	{ .num = vidioc_int_priv_g_pixelsize_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_pixelsize },	//1
	{ .num = vidioc_int_priv_g_pixclk_active_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_pixclk_active },	//1	  
	{ .num = vidioc_int_g_crop_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_g_crop },
	{ .num = vidioc_int_s_crop_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_priv_s_crop },
	  
};

static struct v4l2_int_slave mt9d115_slave = {
	.ioctls = mt9d115_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(mt9d115_ioctl_desc),
};

static struct v4l2_int_device mt9d115_int_device = {
	.module = THIS_MODULE,
	.name = MT9D115_DRIVER_NAME,
	.priv = &mt9d115,
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &mt9d115_slave,
	},
};

/**
 * mt9d115_probe - sensor driver i2c probe handler
 * @client: i2c driver client device structure
 *
 * Register sensor as an i2c client device and V4L2
 * device.
 */
static int __devinit mt9d115_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct mt9d115_sensor *sensor = &mt9d115;
	int err;

	if (i2c_get_clientdata(client))
		return -EBUSY;

	sensor->pdata = client->dev.platform_data;

	if (!sensor->pdata) {
		v4l_err(client, "no platform data?\n");
		return -ENODEV;
	}

	sensor->v4l2_int_device = &mt9d115_int_device;
	sensor->i2c_client = client;

	i2c_set_clientdata(client, sensor);

	/* Make the default capture format QCIF V4L2_PIX_FMT_UYVY */
	sensor->pix.width = MT9D115_IMAGE_WIDTH_MAX;
	sensor->pix.height = MT9D115_IMAGE_HEIGHT_MAX;
	sensor->pix.pixelformat = V4L2_PIX_FMT_UYVY;
	

	err = v4l2_int_device_register(sensor->v4l2_int_device);
	if (err)
		i2c_set_clientdata(client, NULL);

	return 0;
}

/**
 * mt9d115_remove - sensor driver i2c remove handler
 * @client: i2c driver client device structure
 *
 * Unregister sensor as an i2c client device and V4L2
 * device.  Complement of mt9d115_probe().
 */
static int __exit mt9d115_remove(struct i2c_client *client)
{
	struct mt9d115_sensor *sensor = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id mt9d115_id[] = {
	{ MT9D115_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mt9d115_id);

static struct i2c_driver mt9d115sensor_i2c_driver = {
	.driver = {
		.name = MT9D115_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = mt9d115_probe,					//boot
	.remove = __exit_p(mt9d115_remove),
	.id_table = mt9d115_id,
};

static struct mt9d115_sensor mt9d115 = {
	.timeperframe = {
		.numerator = 1,
		.denominator = 30,
	},
};

/**
 * mt9d115sensor_init - sensor driver module_init handler
 *
 * Registers driver as an i2c client driver.  Returns 0 on success,
 * error code otherwise.
 */
static int __init mt9d115sensor_init(void)
{
	int err;

	err = i2c_add_driver(&mt9d115sensor_i2c_driver);
	if (err) {
		printk(KERN_ERR "Failed to register" MT9D115_DRIVER_NAME ".\n");
		return err;
	}
	return 0;
}
late_initcall(mt9d115sensor_init);

/**
 * mt9d115sensor_cleanup - sensor driver module_exit handler
 *
 * Unregisters/deletes driver as an i2c client driver.
 * Complement of mt9d115sensor_init.
 */
static void __exit mt9d115sensor_cleanup(void)
{
	i2c_del_driver(&mt9d115sensor_i2c_driver);
}
module_exit(mt9d115sensor_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("mt9d115 camera sensor driver");
