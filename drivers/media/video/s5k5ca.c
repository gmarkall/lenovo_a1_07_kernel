/*
 * drivers/media/video/s5k5ca.c
 *
 * Aptina s5k5ca sensor driver
 *
 *
 * Copyright (C) 2008 Hewlett Packard
 *
 * Leverage s5k5ca.c
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
#include <media/s5k5ca.h>
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/gpio.h>

#include "s5k5ca_regs.h"
#include "omap34xxcam.h"
#include "isp/isp.h"
#include "isp/ispcsi2.h"


#define S5K5CA_DRIVER_NAME  "s5k5ca"
#define S5K5CA_MOD_NAME "S5K5CA: "
#define MODULE_NAME S5K5CA_DRIVER_NAME

#define V4L2_CID_SET_WORK_MODE V4L2_CID_BASE+100
#define NORMAL_MODE		0
#define NIGHT_MODE		1

#define I2C_M_WR 0
static unsigned int first_time=0;
static unsigned char work_mode=NORMAL_MODE;

#if S5K5CA_REG_TURN
static struct proc_dir_entry *s5k5ca_dir, *s5k5ca_file;
#endif

/**
 * struct s5k5ca_sensor - main structure for storage of sensor information
 * @pdata: access functions and data for platform level information
 * @v4l2_int_device: V4L2 device structure structure
 * @i2c_client: iic client device structure
 * @pix: V4L2 pixel format information structure
 * @timeperframe: time per frame expressed as V4L fraction
 * @scaler:
 * @ver: s5k5ca chip version
 * @fps: frames per second value
 */
struct s5k5ca_sensor {
	const struct s5k5ca_platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	int scaler;
	int ver;
	int fps;
	bool resuming;
};

static struct s5k5ca_sensor s5k5ca;
static struct i2c_driver s5k5casensor_i2c_driver;
static unsigned long xclk_current = S5K5CA_XCLK_NOM_1;

/* list of image formats supported by s5k5ca sensor */
const static struct v4l2_fmtdesc s5k5ca_formats[] = {
	{
		.description = "YUYV (YUV 4:2:2), packed",
		.pixelformat = V4L2_PIX_FMT_UYVY,	
	}
};

#define NUM_CAPTURE_FORMATS ARRAY_SIZE(s5k5ca_formats)

static unsigned detect_count=0;

static enum v4l2_power current_power_state;

/* Structure of Sensor settings that change with image size */
static struct s5k5ca_sensor_settings s5k5ca_settings[] = {
	 /* NOTE: must be in same order as image_size array */
//*************************************************************************************	
	/* 128*96 SQCIF */
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 128,					//sensor
			.y_output_size = 96,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},
	/* 176*144 QCIF*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 84,						//sensor
			.x_addr_end = 1961,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 176,					//sensor
			.y_output_size = 144,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},
	/* 240*160 Fring*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 84,						//sensor
			.y_addr_end = 1449,						//sensor
			.x_output_size = 240,					//sensor
			.y_output_size = 160,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},
	/* 320*240 QVGA*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 320,					//sensor
			.y_output_size = 240,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},

	/* 352*288 CIF*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 84,						//sensor
			.x_addr_end = 1961,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 352,					//sensor
			.y_output_size = 288,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},
	/* 640*480 VGA*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor			
			.x_output_size = 640,					//sensor
			.y_output_size = 480,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},
	/* 720*480 NTSC*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 84,						//sensor
			.y_addr_end = 1449,						//sensor
			.x_output_size = 720,					//sensor
			.y_output_size = 480,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},			
	/* 800*480 WVGA*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor			
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 154,						//sensor
			.y_addr_end = 1381,						//sensor			
			.x_output_size = 800,					//sensor
			.y_output_size = 480,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},
	/* 720*576 NTSC*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 64,						//sensor
			.x_addr_end = 1983,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 720,					//sensor
			.y_output_size = 576,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},		
	/* 768*576 PAL*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 768,					//sensor
			.y_output_size = 576,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},				
	/* 800*600 */
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 800,					//sensor
			.y_output_size = 600,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},
	/* 960*540*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 192,						//sensor
			.y_addr_end = 1343,						//sensor
			.x_output_size = 960,					//sensor
			.y_output_size = 540,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},	
	/* 992*560 Barcode Scanner */
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 190,						//sensor
			.y_addr_end = 1345,						//sensor
			.x_output_size = 992,					//sensor
			.y_output_size = 560,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 1,
			.config_no = 0xff
		},
	},
	/* 1024*768*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 1024,					//sensor
			.y_output_size = 768,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 2,
			.config_no = 0
		},
	},
	/* 1280*720 720p*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 192,						//sensor
			.y_addr_end = 1343,						//sensor
			.x_output_size = 1280,					//sensor
			.y_output_size = 720,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 2,
			.config_no = 1
		},
	},		
	/* 1280*1024 */
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 64,						//sensor
			.x_addr_end = 1983,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 1280,					//sensor
			.y_output_size = 1024,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 2,
			.config_no = 2
		},
	},	
	/* 1600*1200 */
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 1600,					//sensor
			.y_output_size = 1200,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 2,
			.config_no = 3
		},
	},
	/* 2048*1536*/
	{
		.clk = {
			.pre_pll_div = 1,							//csi2
			.pll_mult = 18,								//csi2
			.post_pll_div = 1,						//csi2
			.vt_pix_clk_div = 10,					//csi2
			.vt_sys_clk_div = 1,					//csi2
		},
		.mipi = {
			.data_lanes = 1,							//csi2
			.ths_prepare = 2,							//sensor
			.ths_zero = 5,								//sensor
			.ths_settle_lower = 8,				//csi2
			.ths_settle_upper = 28,				//csi2
		},
		.frame = {
			.frame_len_lines_min = 629,		//sensor
			.line_len_pck = 3440,					//sensor
			.x_addr_start = 0,						//sensor
			.x_addr_end = 2047,						//sensor
			.y_addr_start = 0,						//sensor
			.y_addr_end = 1535,						//sensor
			.x_output_size = 2048,					//sensor
			.y_output_size = 1536,					//sensor
			.x_even_inc = 1,							//sensor
			.x_odd_inc = 1,								//sensor
			.y_even_inc = 1,							//sensor
			.y_odd_inc = 1,								//sensor
			.v_mode_add = 0,							//sensor
			.h_mode_add = 0,							//sensor
			.h_add_ave = 0,								//sensor
			.context = 2,
			.config_no = 4
		},
	},
};

#define S5K5CA_MODES_COUNT ARRAY_SIZE(s5k5ca_settings)

static unsigned isize_current = S5K5CA_MODES_COUNT - 1;
static struct s5k5ca_clock_freq current_clk;

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
} s5k5casensor_video_control[] = {
	{
		{
			.id = V4L2_CID_EXPOSURE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Exposure",
			.minimum = S5K5CA_MIN_EXPOSURE,
			.maximum = S5K5CA_MAX_EXPOSURE,
			.step = S5K5CA_EXPOSURE_STEP,
			.default_value = S5K5CA_DEF_EXPOSURE,
		},
		.current_value = S5K5CA_DEF_EXPOSURE,
	},
	{
		{
			.id = V4L2_CID_COLORFX,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Camera Effect",
			.minimum = S5K5CA_MIN_EFFECT,
			.maximum = S5K5CA_MAX_EFFECT,
			.step = S5K5CA_EFFECT_STEP,
			.default_value = S5K5CA_DEF_EFFECT,
		},
		.current_value = S5K5CA_DEF_EFFECT,
	},
	{
		{
			.id = V4L2_CID_DO_WHITE_BALANCE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "White Balance",
			.minimum = S5K5CA_MIN_WB,
			.maximum = S5K5CA_MAX_WB,
			.step = S5K5CA_WB_STEP,
			.default_value = S5K5CA_DEF_WB,
		},
		.current_value = S5K5CA_DEF_WB,
	},
	{
		{
			.id = V4L2_CID_BRIGHTNESS,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Camera Brightness",
			.minimum = S5K5CA_MIN_BR,
			.maximum = S5K5CA_MAX_BR,
			.step = S5K5CA_BR_STEP,
			.default_value = S5K5CA_DEF_BR,
		},
		.current_value = S5K5CA_DEF_BR,
	},
	{
		{
			.id = V4L2_CID_POWER_LINE_FREQUENCY,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Camera Antibanding",
			.minimum = S5K5CA_MIN_BANDING,
			.maximum = S5K5CA_MAX_BANDING,
			.step = S5K5CA_BANDING_STEP,
			.default_value = S5K5CA_DEF_BANDING,
		},
		.current_value = S5K5CA_DEF_BANDING,
	},
	{
		{
			.id = V4L2_CID_SET_WORK_MODE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Camera work mode",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.current_value = 0,
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

	for (i = (ARRAY_SIZE(s5k5casensor_video_control) - 1); i >= 0; i--)
		if (s5k5casensor_video_control[i].qc.id == id)
			break;
	if (i < 0)
		i = -EINVAL;
	return i;
}

/**
 * s5k5ca_read_reg - Read a value from a register in an s5k5ca sensor device
 * @client: i2c driver client structure
 * @data_length: length of data to be read
 * @reg: register address / offset
 * @val: stores the value that gets read
 *
 * Read a value from a register in an s5k5ca sensor device.
 * The value is returned in 'val'.
 * Returns zero if successful, or non-zero otherwise.
 */
static int s5k5ca_read_reg(struct i2c_client *client, u16 data_length, u16 reg,
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
	data[0] = (u8) (reg >> 8);;
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
 * Write a value to a register in s5k5ca sensor device.
 * @client: i2c driver client structure.
 * @reg: Address of the register to read value from.
 * @val: Value to be written to a specific register.
 * Returns zero if successful, or non-zero otherwise.
 */
static int s5k5ca_write_reg(struct i2c_client *client, u16 reg, u32 val,
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
		dev_dbg(&client->dev, "S5K5CA Wrt:[0x%04X]=0x%02X\n",
				reg, val);
	else if (data_length == 2)
		dev_dbg(&client->dev, "S5K5CA Wrt:[0x%04X]=0x%04X\n",
				reg, val);

	err = i2c_transfer(client->adapter, msg, 1);

	//mdelay(3);
//printk("s5k5ca_write_reg write data err= %d, retries = %d\n", err, retries); 
	if (err >= 0)
		return 0;

	/*if (retries <= 5) {
		v4l_info(client, "Retrying I2C... %d", retries);
		retries++;
		mdelay(20);
		goto retry;
	}*/

	return err;
}

/**
 * Initialize a list of s5k5ca registers.
 * The list of registers is terminated by the pair of values
 * {OV3640_REG_TERM, OV3640_VAL_TERM}.
 * @client: i2c driver client structure.
 * @reglist[]: List of address of the registers to write data.
 * Returns zero if successful, or non-zero otherwise.
 */
static int s5k5ca_write_regs(struct i2c_client *client,
			     const struct s5k5ca_reg reglist[])
{
	int err = 0;
	const struct s5k5ca_reg *list = reglist;

	while (!((list->reg == I2C_REG_TERM)
		&& (list->val == I2C_VAL_TERM)))
	{

		if(list->length == I2C_DELAY)
			mdelay(list->val);
		else
			err = s5k5ca_write_reg(client, list->reg,
				list->val, list->length);
		if (err)
		{
			v4l_info(client, "s5k5ca_write_regs error num = %d, l = %d\n", list - reglist, list->length);
			return err;
		}

		list++;
	}
	return 0;
}

/**
 * s5k5ca_find_size - Find the best match for a requested image capture size
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
static unsigned s5k5ca_find_size(unsigned int width, unsigned int height)
{
	unsigned isize;

	for (isize = 0; isize < S5K5CA_MODES_COUNT; isize++) {
		if ((s5k5ca_settings[isize].frame.y_output_size >= height) &&
		    (s5k5ca_settings[isize].frame.x_output_size >= width))
			break;
	}

	printk(KERN_DEBUG "s5k5ca_find_size: Req Size=%dx%d, "
			"Calc Size=%dx%d\n", width, height,
			s5k5ca_settings[isize].frame.x_output_size,
			s5k5ca_settings[isize].frame.y_output_size);

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
//static int s5k5ca_set_virtual_id(struct i2c_client *client, u32 id)
//{
//printk("1 [s5k5ca Sensor]Enter %s \n", __FUNCTION__); 
//	return s5k5ca_write_reg(client, S5K5CA_REG_CCP2_CHANNEL_ID,
//				(0x3 & id), I2C_16BIT);
//}

/**
 * s5k5ca_set_framerate - Sets framerate by adjusting frame_length_lines reg.
 * @s: pointer to standard V4L2 device structure
 * @fper: frame period numerator and denominator in seconds
 *
 * The maximum exposure time is also updated since it is affected by the
 * frame rate.
 **/
static int s5k5ca_set_framerate(struct v4l2_int_device *s,
				struct v4l2_fract *fper)
{
		
	int err = 0;
	u16 isize = isize_current;
	u32 frame_length_lines, line_time_q8;
	struct s5k5ca_sensor *sensor = s->priv;
	struct s5k5ca_sensor_settings *ss;

	if ((fper->numerator == 0) || (fper->denominator == 0)) {
		/* supply a default nominal_timeperframe */
		fper->numerator = 1;
		fper->denominator = S5K5CA_DEF_FPS;
	}

	sensor->fps = fper->denominator / fper->numerator;
	if (sensor->fps < S5K5CA_MIN_FPS) {
		sensor->fps = S5K5CA_MIN_FPS;
		fper->numerator = 1;
		fper->denominator = sensor->fps;
	} else if (sensor->fps > S5K5CA_MAX_FPS) {
		sensor->fps = S5K5CA_MAX_FPS;
		fper->numerator = 1;
		fper->denominator = sensor->fps;
	}

	ss = &s5k5ca_settings[isize_current];

	line_time_q8 = ((u32)ss->frame.line_len_pck * 1000000) /
			(current_clk.vt_pix_clk >> 8); /* usec's */

	frame_length_lines = (((u32)fper->numerator * 1000000 * 256 /
			       fper->denominator)) / line_time_q8;

	/* Range check frame_length_lines */
	if (frame_length_lines > S5K5CA_MAX_FRAME_LENGTH_LINES)
		frame_length_lines = S5K5CA_MAX_FRAME_LENGTH_LINES;
	else if (frame_length_lines < ss->frame.frame_len_lines_min)
		frame_length_lines = ss->frame.frame_len_lines_min;

	s5k5ca_settings[isize].frame.frame_len_lines = frame_length_lines;

	printk(KERN_DEBUG "S5K5CA Set Framerate: fper=%d/%d, "
	       "frame_len_lines=%d, max_expT=%dus\n", fper->numerator,
	       fper->denominator, frame_length_lines, S5K5CA_MAX_EXPOSURE);

	return err;
}

/**
 * s5k5casensor_calc_xclk - Calculate the required xclk frequency
 *
 * Xclk is not determined from framerate for the S5K5CA
 */
static unsigned long s5k5casensor_calc_xclk(void)
{
	return xclk_current;
}

///**
// * Sets the correct orientation based on the sensor version.
// *   IU046F2-Z   version=2  orientation=3
// *   IU046F4-2D  version>2  orientation=0
// */
//static int s5k5ca_set_orientation(struct i2c_client *client, u32 ver)
//{
//	int err;
//	u8 orient;
//
//printk("1 [s5k5ca Sensor]Enter %s \n", __FUNCTION__); 
//	orient = (ver <= 0x2) ? 0x3 : 0x0;
//	err = s5k5ca_write_reg(client, S5K5CA_REG_IMAGE_ORIENTATION,
//			       orient, I2C_16BIT);
//	return err;
//}

/**
 * s5k5casensor_set_exposure_time - sets exposure time per input value
 * @exp_time: exposure time to be set on device (in usec)
 * @s: pointer to standard V4L2 device structure
 * @lvc: pointer to V4L2 exposure entry in s5k5casensor_video_controls array
 *
 * If the requested exposure time is within the allowed limits, the HW
 * is configured to use the new exposure time, and the
 * s5k5casensor_video_control[] array is updated with the new current value.
 * The function returns 0 upon success.  Otherwise an error code is
 * returned.
 */
static int s5k5casensor_set_exposure_time(u32 exp_time,
					  struct v4l2_int_device *s,
					  struct vcontrol *lvc)
{
	int err = 0, i;
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 coarse_int_time = 0;
	u32 line_time_q8 = 0;
	struct s5k5ca_sensor_settings *ss;

	if ((current_power_state == V4L2_POWER_ON) || sensor->resuming) {
		if (exp_time < S5K5CA_MIN_EXPOSURE) {
			v4l_err(client, "Exposure time %d us not within"
					" the legal range.\n", exp_time);
			v4l_err(client, "Exposure time must be between"
					" %d us and %d us\n",
					S5K5CA_MIN_EXPOSURE,
					S5K5CA_MAX_EXPOSURE);
			exp_time = S5K5CA_MIN_EXPOSURE;
		}

		if (exp_time > S5K5CA_MAX_EXPOSURE) {
			v4l_err(client, "Exposure time %d us not within"
					" the legal range.\n", exp_time);
			v4l_err(client, "Exposure time must be between"
					" %d us and %d us\n",
					S5K5CA_MIN_EXPOSURE,
					S5K5CA_MAX_EXPOSURE);
			exp_time = S5K5CA_MAX_EXPOSURE;
		}

		ss = &s5k5ca_settings[isize_current];

		line_time_q8 = ((u32)ss->frame.line_len_pck * 1000000) /
				(current_clk.vt_pix_clk >> 8); /* usec's */

		coarse_int_time = ((exp_time * 256) + (line_time_q8 >> 1)) /
				  line_time_q8;

//		if (coarse_int_time > ss->frame.frame_len_lines - 2)
//			err = s5k5ca_write_reg(client,
//					       S5K5CA_REG_FRAME_LEN_LINES,
//					       coarse_int_time + 2,
//					       I2C_16BIT);
//		else
//			err = s5k5ca_write_reg(client,
//					       S5K5CA_REG_FRAME_LEN_LINES,
//					       ss->frame.frame_len_lines,
//					       I2C_16BIT);

		//err = s5k5ca_write_reg(client, S5K5CA_REG_COARSE_INT_TIME,
				       //coarse_int_time, I2C_16BIT);
	}

	if (err) {
		v4l_err(client, "Error setting exposure time: %d", err);
	} else {
		i = find_vctrl(V4L2_CID_EXPOSURE);
		if (i >= 0) {
			lvc = &s5k5casensor_video_control[i];
			lvc->current_value = exp_time;
		}
	}

	return err;
}

static int s5k5casensor_set_exposure(long data,
					  struct v4l2_int_device *s,
					  struct vcontrol *lvc)
{
		

	int err = 0, i;
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	struct s5k5ca_sensor_settings *ss;
	
	if(first_time)
	{
		if(data > 35) data = 35;
		
		if(data < -35) data = -35;

		s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
		s5k5ca_write_reg(client, 0x0028, 0x7000, I2C_16BIT);	
		//s5k5ca_write_reg(client, 0x002A, 0x020C, I2C_16BIT);
		s5k5ca_write_reg(client, 0x002A, 0x0F70, I2C_16BIT);
		s5k5ca_write_reg(client, 0x0F12, (data * 5 + 2) / 4 + 65, I2C_16BIT);
		
		lvc->current_value = data;
	}

	return err;
}

static int s5k5casensor_set_effect(u16 mode, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
	int err = 0;
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	
        if(first_time)
	{
		switch (mode) 
		{
		case V4L2_COLORFX_NONE:
			err = s5k5ca_write_regs(client, colorfx_none);
			break;
			
		case V4L2_COLORFX_BW:
			err = s5k5ca_write_regs(client, colorfx_bw);
			break;

		case V4L2_COLORFX_SEPIA:
			err = s5k5ca_write_regs(client, colorfx_sepia);
			break;

		case V4L2_COLORFX_NEGATIVE:
			err = s5k5ca_write_regs(client, colorfx_negative);
			break;
			
		case V4L2_COLORFX_AQUA:
			err = s5k5ca_write_regs(client, colorfx_aqua);
			break;
			
		case V4L2_COLORFX_SKETCH:
			err = s5k5ca_write_regs(client, colorfx_sketch);
			break;
			
		case V4L2_COLORFX_EMBOSS:
			err = s5k5ca_write_regs(client, colorfx_emboss);
			break;

		/* s5k5ca no support */
		/*case V4L2_COLORFX_SOLARIZE:
			err = s5k5ca_write_regs(client, colorfx_solarize);
			break;*/
		}
		mdelay(10);
		lvc->current_value = mode;
	}	
		return err;
}


static int s5k5casensor_set_wb(u16 mode, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
	int err = 0;
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
        if(first_time)
        {
		switch (mode) 
		{
		case WHITE_BALANCE_AUTO:
			err = s5k5ca_write_regs(client, wb_auto);
		
			break;
		case WHITE_BALANCE_INCANDESCENT:
			err = s5k5ca_write_regs(client, wb_incandescent);
		
			break;
		case WHITE_BALANCE_FLUORESCENT:
			err = s5k5ca_write_regs(client, wb_fluorescent);
		
			break;
		case WHITE_BALANCE_DAYLIGHT:
			err = s5k5ca_write_regs(client, wb_daylight);
		
			break;
		case WHITE_BALANCE_SHADE:
			err = s5k5ca_write_regs(client, wb_shade);
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT:
			err = s5k5ca_write_regs(client, wb_cloudy);
			break;
		case WHITE_BALANCE_HORIZON:
			err = s5k5ca_write_regs(client, wb_horizon);
			break;
		case WHITE_BALANCE_TUNGSTEN:
			err = s5k5ca_write_regs(client, wb_tungsten);
			break;
			
		}
		lvc->current_value = mode;
	}	
		return err;
}


static int s5k5casensor_set_br(u16 mode, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
	int err = 0;
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	short brv = (int)mode - 128;

	if(first_time)
	{
		s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
		s5k5ca_write_reg(client, 0x0028, 0x7000, I2C_16BIT);	
		s5k5ca_write_reg(client, 0x002A, 0x020C, I2C_16BIT);
		s5k5ca_write_reg(client, 0x0F12, brv, I2C_16BIT);
		lvc->current_value = mode;
	}	

	return err;
}


static int s5k5casensor_set_banding(u16 mode, struct v4l2_int_device *s,
				 struct vcontrol *lvc)
{
	int err = 0;
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;

	if(first_time)
	{
		switch (mode) 
		{
		case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
			err = s5k5ca_write_regs(client, flicker_off);
			break;
			
		case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
			err = s5k5ca_write_regs(client, flicker_50hz);
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
			err = s5k5ca_write_regs(client, flicker_60hz);
			break;
		}

		mdelay(10);
		lvc->current_value = mode;

	}

	return err;
}

/**
 * s5k5ca_update_clocks - calcs sensor clocks based on sensor settings.
 * @isize: image size enum
 */
static int s5k5ca_update_clocks(u32 xclk, unsigned isize)
{
	
	current_clk.vco_clk =
			xclk * s5k5ca_settings[isize].clk.pll_mult /
			s5k5ca_settings[isize].clk.pre_pll_div /
			s5k5ca_settings[isize].clk.post_pll_div;

	current_clk.vt_pix_clk = current_clk.vco_clk * 2 /
			(s5k5ca_settings[isize].clk.vt_pix_clk_div *
			s5k5ca_settings[isize].clk.vt_sys_clk_div);

	if (s5k5ca_settings[isize].mipi.data_lanes == 2)
		current_clk.mipi_clk = current_clk.vco_clk;
	else
		current_clk.mipi_clk = current_clk.vco_clk / 2;

	current_clk.ddr_clk = current_clk.mipi_clk / 2;

	printk(KERN_DEBUG "S5K5CA: xclk=%u, vco_clk=%u, "
		"vt_pix_clk=%u,  mipi_clk=%u,  ddr_clk=%u\n",
		xclk, current_clk.vco_clk, current_clk.vt_pix_clk,
		current_clk.mipi_clk, current_clk.ddr_clk);

	return 0;
}

/**
 * s5k5ca_setup_pll - initializes sensor PLL registers.
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int s5k5ca_setup_pll(struct v4l2_int_device *s)
{
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;	
	unsigned isize = isize_current;
	int err;

	return 0;
}

/**
 * s5k5ca_setup_mipi - initializes sensor & isp MIPI registers.
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int s5k5ca_setup_mipi(struct v4l2_int_device *s, unsigned isize)
{
		
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;


//	mdelay(200);


	/* Set number of lanes in sensor */
//	if (s5k5ca_settings[isize].mipi.data_lanes == 2)
//		s5k5ca_write_reg(client, S5K5CA_REG_RGLANESEL, 0x00, I2C_16BIT);
//	else
//		s5k5ca_write_reg(client, S5K5CA_REG_RGLANESEL, 0x01, I2C_16BIT);

	/* Set number of lanes in isp */
	sensor->pdata->csi2_lane_count(s,
				       s5k5ca_settings[isize].mipi.data_lanes);

	/* Send settings to ISP-CSI2 Receiver PHY */
	sensor->pdata->csi2_calc_phy_cfg0(s, current_clk.mipi_clk,
		s5k5ca_settings[isize].mipi.ths_settle_lower,
		s5k5ca_settings[isize].mipi.ths_settle_upper);

	/* Dump some registers for debug purposes */
	printk(KERN_DEBUG "imx:THSPREPARE=0x%02X\n",
		s5k5ca_settings[isize].mipi.ths_prepare);
	printk(KERN_DEBUG "imx:THSZERO=0x%02X\n",
		s5k5ca_settings[isize].mipi.ths_zero);
	printk(KERN_DEBUG "imx:LANESEL=0x%02X\n",
		(s5k5ca_settings[isize].mipi.data_lanes == 2) ? 0 : 1);

	return 0;
}

/*void reg_check(struct i2c_client *client)
{
	int x,y;
	
	s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x002C, 0x7000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x002E, 0x16B0, I2C_16BIT);
	s5k5ca_read_reg(client, I2C_16BIT, 0x0F12, &x);
	s5k5ca_read_reg(client, I2C_16BIT, 0x0F12, &y);
	
	printk("[s5k5ca Sensor]Enter %s width = %d, height = %d\n", __FUNCTION__, x, y);
	
	s5k5ca_write_reg(client, 0x002E, 0x026C, I2C_16BIT);
	s5k5ca_read_reg(client, I2C_16BIT, 0x0F12, &x);
	s5k5ca_read_reg(client, I2C_16BIT, 0x0F12, &y);
	
	printk("[s5k5ca Sensor]Enter %s config0 width = %d, height = %d\n", __FUNCTION__, x, y);
}*/

/**
 * s5k5ca_configure_frame - initializes image frame registers
 * @c: i2c client driver structure
 * @isize: image size enum
 */
static int s5k5ca_configure_frame(struct i2c_client *client, unsigned isize)
{
	u32 val;
	
	s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x0028, 0x7000, I2C_16BIT);

	if(s5k5ca_settings[isize].frame.context == 1) //context_A
	{
		/* Set crop window. */
		/*s5k5ca_write_reg(client, 0x002A, 0x0232, I2C_16BIT);
		s5k5ca_write_reg(client, 0x0F12, 
								s5k5ca_settings[isize].frame.x_addr_end - s5k5ca_settings[isize].frame.x_addr_start + 1,
								I2C_16BIT);
		s5k5ca_write_reg(client, 0x0F12, 
								s5k5ca_settings[isize].frame.y_addr_end - s5k5ca_settings[isize].frame.y_addr_start + 1,
								I2C_16BIT);
		s5k5ca_write_reg(client, 0x0F12, s5k5ca_settings[isize].frame.x_addr_start, I2C_16BIT);
		s5k5ca_write_reg(client, 0x0F12, s5k5ca_settings[isize].frame.y_addr_start, I2C_16BIT);
		s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);*/
	
		if(s5k5ca_settings[isize].frame.config_no == 0xff)
		{
			s5k5ca_write_reg(client, 0x002A, 0x026C, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, s5k5ca_settings[isize].frame.x_output_size, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, s5k5ca_settings[isize].frame.y_output_size, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0005, I2C_16BIT);	//#REG_0TC_PCFG_Format            0270
			s5k5ca_write_reg(client, 0x0F12, 0x3AA8, I2C_16BIT);	//#REG_0TC_PCFG_usMaxOut4KHzRate  0272
			s5k5ca_write_reg(client, 0x0F12, 0x3A88, I2C_16BIT);	//#REG_0TC_PCFG_usMinOut4KHzRate  0274
			s5k5ca_write_reg(client, 0x0F12, 0x0100, I2C_16BIT);	//#REG_0TC_PCFG_OutClkPerPix88    0276
			s5k5ca_write_reg(client, 0x0F12, 0x0800, I2C_16BIT);	//#REG_0TC_PCFG_uMaxBpp88         027
			s5k5ca_write_reg(client, 0x0F12, 0x0052, I2C_16BIT);	//#REG_0TC_PCFG_PVIMask //s0050 = FALSE in MSM6290 : s0052 = TRUE in MSM6800 //reg 027A
			s5k5ca_write_reg(client, 0x002A, 0x027E, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_usJpegPacketSize
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_usJpegTotalPackets
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_uClockInd
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_usFrTimeType
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);	//#REG_0TC_PCFG_FrRateQualityType
			
			if(work_mode == NIGHT_MODE)
				s5k5ca_write_reg(client, 0x0F12, 0x0535, I2C_16BIT);	//#REG_0TC_PCFG_usMaxFrTimeMsecMult10
			else
				s5k5ca_write_reg(client, 0x0F12, 0x014D, I2C_16BIT);	//#REG_0TC_PCFG_usMaxFrTimeMsecMult10

			s5k5ca_write_reg(client, 0x0F12, 0x014d, I2C_16BIT);	//#REG_0TC_PCFG_usMinFrTimeMsecMult10
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_bSmearOutput
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_sSaturation
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_sSharpBlur
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_sColorTemp
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_uDeviceGammaIndex
			s5k5ca_write_reg(client, 0x0F12, 0x0003, I2C_16BIT);	//#REG_0TC_PCFG_uPrevMirror
			s5k5ca_write_reg(client, 0x0F12, 0x0003, I2C_16BIT);	//#REG_0TC_PCFG_uCaptureMirror
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_PCFG_uRotation		

			s5k5ca_write_reg(client, 0x002A,0x023C, I2C_16BIT);        
			s5k5ca_write_reg(client, 0x0F12,0x0000, I2C_16BIT);	// #REG_TC_GP_ActivePrevConfig // Select preview configuration_0
			s5k5ca_write_reg(client, 0x002A,0x0240, I2C_16BIT);        
			s5k5ca_write_reg(client, 0x0F12,0x0001, I2C_16BIT);	//_TC_GP_PrevOpenAfterChange
			s5k5ca_write_reg(client, 0x002A,0x0230, I2C_16BIT);        
			s5k5ca_write_reg(client, 0x0F12,0x0001, I2C_16BIT);  	// TC_GP_NewConfigSync // Update preview configuration
			s5k5ca_write_reg(client, 0x002A,0x023E, I2C_16BIT);        
			s5k5ca_write_reg(client, 0x0F12,0x0001, I2C_16BIT);	// #REG_TC_GP_PrevConfigChanged
          
			//s5k5ca_write_reg(client, 0xB0A0, 0x0000, I2C_16BIT);
			s5k5ca_write_reg(client, 0x002A,0x0220, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12,0x0001, I2C_16BIT);	// #REG_TC_GP_EnablePreview // Start preview
			s5k5ca_write_reg(client, 0x0F12,0x0001, I2C_16BIT);	// #REG_TC_GP_EnablePreviewChanged
		}
		else
		{
			s5k5ca_write_reg(client, 0x002A, 0x023C, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, s5k5ca_settings[isize].frame.config_no, I2C_16BIT);
			s5k5ca_write_reg(client, 0x002A, 0x0240, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
			s5k5ca_write_reg(client, 0x002A, 0x0230, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
			s5k5ca_write_reg(client, 0x002A, 0x023E, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
			
			//s5k5ca_write_reg(client, 0xB0A0, 0x0000, I2C_16BIT);
			s5k5ca_write_reg(client, 0x002A, 0x0220, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
		}
	}
	else	//context_B
	{
		if(s5k5ca_settings[isize].frame.config_no == 0xff)
		{
			s5k5ca_write_reg(client, 0x002A, 0x035C, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, s5k5ca_settings[isize].frame.x_output_size, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, s5k5ca_settings[isize].frame.y_output_size, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0005, I2C_16BIT);	//#REG_0TC_CCFG_Format//5:YUV9:JPEG              
			s5k5ca_write_reg(client, 0x0F12, 0x3AA8, I2C_16BIT);	//#REG_0TC_CCFG_usMaxOut4KHzRate                 
			s5k5ca_write_reg(client, 0x0F12, 0x3A88, I2C_16BIT);	//#REG_0TC_CCFG_usMinOut4KHzRate                 
			s5k5ca_write_reg(client, 0x0F12, 0x0100, I2C_16BIT);	//#REG_0TC_CCFG_OutClkPerPix88                   
			s5k5ca_write_reg(client, 0x0F12, 0x0800, I2C_16BIT);	//#REG_0TC_CCFG_uMaxBpp88                        
			s5k5ca_write_reg(client, 0x0F12, 0x0052, I2C_16BIT);	//#REG_0TC_CCFG_PVIMask                          
			s5k5ca_write_reg(client, 0x0F12, 0x0050, I2C_16BIT);	//#REG_0TC_CCFG_OIFMask                          
			s5k5ca_write_reg(client, 0x0F12, 0x0600, I2C_16BIT);	//#REG_0TC_CCFG_usJpegPacketSize                 
			s5k5ca_write_reg(client, 0x0F12, 0x0400, I2C_16BIT);	//08FC	//#REG_0TC_CCFG_usJpegTotalPackets     
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_CCFG_uClockInd                      
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_CCFG_usFrTimeType                   
			s5k5ca_write_reg(client, 0x0F12, 0x0002, I2C_16BIT);	//#REG_0TC_CCFG_FrRateQualityType              
			s5k5ca_write_reg(client, 0x0F12, 0x0534, I2C_16BIT);	//#REG_0TC_CCFG_usMaxFrTimeMsecMult10 //7.5fps 
			s5k5ca_write_reg(client, 0x0F12, 0x029a, I2C_16BIT);	//#REG_0TC_CCFG_usMinFrTimeMsecMult10 //13.5fps
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_CCFG_bSmearOutput                   
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_CCFG_sSaturation                    
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_CCFG_sSharpBlur                     
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_CCFG_sColorTemp                     
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);	//#REG_0TC_CCFG_uDeviceGammaIndex

			s5k5ca_write_reg(client, 0x002A, 0x0244, I2C_16BIT); 	//Normal capture// 
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT); 	//config 0 // REG_TC_GP_ActiveCapConfig    0/1 
			s5k5ca_write_reg(client, 0x002A, 0x0240, I2C_16BIT); 
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);	//_TC_GP_PrevOpenAfterChange
			s5k5ca_write_reg(client, 0x002A, 0x0230, I2C_16BIT);                    
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);	//REG_TC_GP_NewConfigSync          
			s5k5ca_write_reg(client, 0x002A, 0x0246, I2C_16BIT);                    
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);   //Synchronize FW with new capture configuration
       
			//s5k5ca_write_reg(client, 0xB0A0, 0x0000, I2C_16BIT);
			s5k5ca_write_reg(client, 0x002A, 0x0224, I2C_16BIT);                    
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);	// REG_TC_GP_EnableCapture         
			s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);    // REG_TC_GP_EnableCaptureChanged
		}
		else
		{
			s5k5ca_write_reg(client, 0x002A, 0x0244, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, s5k5ca_settings[isize].frame.config_no, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x002A, 0x0240, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x002A, 0x0230, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x002A, 0x0246, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
			//s5k5ca_write_reg(client, 0xB0A0, 0x0000, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x002A, 0x0224, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
		    s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
		}
	}

	s5k5ca_write_reg(client, 0x0028, 0xD000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x002A, 0x1000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
	
	//if(s5k5ca_settings[isize].frame.context == 1) s5k5ca_write_reg(client, 0x014C, 0x0007, I2C_16BIT);

	mdelay(100);
	//s5k5ca_write_reg(client, 0xFCFC, 0x7000, I2C_16BIT);
	//s5k5ca_read_reg(client, I2C_16BIT, 0x04E8, &val);
	
	//printk("%s sensor bright = %d\n", __FUNCTION__, val);
	
	//reg_check(client);
	
	return 0;
}

/**
 * s5k5ca_configure - Configure the s5k5ca for the specified image mode
 * @s: pointer to standard V4L2 device structure
 *
 * Configure the s5k5ca for a specified image size, pixel format, and frame
 * period.  xclk is the frequency (in Hz) of the xclk input to the s5k5ca.
 * fper is the frame period (in seconds) expressed as a fraction.
 * Returns zero if successful, or non-zero otherwise.
 * The actual frame period is returned in fper.
 */
static int s5k5ca_configure(struct v4l2_int_device *s)
{
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	unsigned isize = isize_current;
	int err, i;
	struct vcontrol *lvc = NULL;

	printk("[s5k5ca Sensor]Enter %s \n", __FUNCTION__);
	
	if((first_time == 0)/* || (s5k5ca_settings[isize_current].frame.context == 1)*/)
	{
		err = s5k5ca_write_regs(client, initial_list);
		mdelay(10);
	}
	
	s5k5ca_setup_mipi(s, isize);

	mdelay(10);


//	s5k5ca_set_orientation(client, sensor->ver);
	first_time=1;

	sensor->pdata->csi2_cfg_vp_out_ctrl(s, 2);
	printk(KERN_ERR "start csi2_cfg_vp_out_ctrl.\n");
	sensor->pdata->csi2_ctrl_update(s, false);

	sensor->pdata->csi2_cfg_virtual_id(s, 0, S5K5CA_CSI2_VIRTUAL_ID);
	printk(KERN_ERR "csi2_cfg_virtual_id.\n");
	sensor->pdata->csi2_ctx_update(s, 0, false);
//	s5k5ca_set_virtual_id(client, S5K5CA_CSI2_VIRTUAL_ID);

	/* configure image size and pixel format */
	s5k5ca_configure_frame(client, isize);
	
	if(first_time == 1)
	{
		/* Enable mipi out. */
		//s5k5ca_write_reg(client, 0x014C, 0x0007, I2C_16BIT);
	}

	/* Set initial exposure and gain */
	i = find_vctrl(V4L2_CID_EXPOSURE);
	if (i >= 0) {
		lvc = &s5k5casensor_video_control[i];
		//s5k5casensor_set_exposure_time(lvc->current_value,
		//	sensor->v4l2_int_device, lvc);
		s5k5casensor_set_exposure(lvc->current_value,
			sensor->v4l2_int_device, lvc);
	}

	i = find_vctrl(V4L2_CID_COLORFX);
	if (i >= 0) {
		lvc = &s5k5casensor_video_control[i];
		err = s5k5casensor_set_effect(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}
	i = find_vctrl(V4L2_CID_DO_WHITE_BALANCE);
	if (i >= 0) {
		lvc = &s5k5casensor_video_control[i];
		err = s5k5casensor_set_wb(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}
	i = find_vctrl(V4L2_CID_BRIGHTNESS);
	if (i >= 0) {
		lvc = &s5k5casensor_video_control[i];
		err = s5k5casensor_set_br(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}
	i = find_vctrl(V4L2_CID_POWER_LINE_FREQUENCY);
	if (i >= 0) {
		lvc = &s5k5casensor_video_control[i];
		err = s5k5casensor_set_banding(lvc->current_value,
				sensor->v4l2_int_device, lvc);
	}

printk("[s5k5ca Sensor]Enter %s first_time =%d ok\n", __FUNCTION__,first_time); 
	return err;
}

/**
 * s5k5ca_detect - Detect if an s5k5ca is present, and if so which revision
 * @client: pointer to the i2c client driver structure
 *
 * Detect if an s5k5ca is present, and if so which revision.
 * A device is considered to be detected if the manufacturer ID (MIDH and MIDL)
 * and the product ID (PID) registers match the expected values.
 * Any value of the version ID (VER) register is accepted.
 * Returns a negative error number if no device is detected, or the
 * non-negative value of the version ID register if a device is detected.
 */
static int s5k5ca_detect(struct i2c_client *client)
{
		
	u32 model_id;
	int i=0;
	struct s5k5ca_sensor *sensor;

	if (!client)
		return -ENODEV;
		
	//0xFCFC, 0xD000, I2C_16BIT
	s5k5ca_write_reg(client, 0xFCFC, 0x0000, I2C_16BIT);

	sensor = i2c_get_clientdata(client);
	if(detect_count)
	{
	for(i =0; i<100 ; i++)
	   s5k5ca_read_reg(client, I2C_16BIT, S5K5CA_REG_MODEL_ID, &model_id);
	}   
	
	detect_count++;
	
	if (s5k5ca_read_reg(client, I2C_16BIT, S5K5CA_REG_MODEL_ID, &model_id))
		return -ENODEV;

	v4l_info(client, "model id detected 0x%x\n",model_id);
	
	
	if ((model_id != S5K5CA_MOD_ID)) {
		/* We didn't read the values we expected, so
		 * this must not be an S5K5CA.
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
 * from the s5k5casensor_video_control[] array.
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

	*qc = s5k5casensor_video_control[i].qc;
	return 0;
}

/**
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the s5k5casensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_g_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	struct vcontrol *lvc;
	int i;
printk("[s5k5ca Sensor]Enter %s \n", __FUNCTION__); 

	i = find_vctrl(vc->id);
	if (i < 0)
		return -EINVAL;
	lvc = &s5k5casensor_video_control[i];

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
	case V4L2_CID_BRIGHTNESS:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		vc->value = lvc->current_value;
		break;
	case V4L2_CID_SET_WORK_MODE:
		vc->value = work_mode;
	}

	return 0;
}

/**
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the s5k5casensor_video_control[] array).
 * Otherwise, * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	int retval = -EINVAL;
	int i;
	struct vcontrol *lvc;

	i = find_vctrl(vc->id);
printk("[s5k5ca.c]Enter %s , id = %d(%d), value = %d \n", __FUNCTION__,i,vc->id,vc->value ); 
	
	if (i < 0)
		return -EINVAL;
	lvc = &s5k5casensor_video_control[i];

	switch (vc->id) {
	case V4L2_CID_EXPOSURE:
//		retval = s5k5casensor_set_exposure_time(vc->value, s, lvc);
		retval = s5k5casensor_set_exposure(vc->value, s, lvc);
		break;
	case V4L2_CID_COLORFX:
		retval = s5k5casensor_set_effect(vc->value, s, lvc);
		break;
	case V4L2_CID_DO_WHITE_BALANCE:
		retval = s5k5casensor_set_wb(vc->value, s, lvc);
		break;
	case V4L2_CID_BRIGHTNESS:
		retval = s5k5casensor_set_br(vc->value, s, lvc);
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		retval = s5k5casensor_set_banding(vc->value, s, lvc);
		break;
	case V4L2_CID_SET_WORK_MODE:
		retval = 0;
		if(vc->value == NIGHT_MODE)
		{
			work_mode = NIGHT_MODE;
		}
		else
		{
			work_mode = NORMAL_MODE;
		}
		
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

	fmt->flags = s5k5ca_formats[index].flags;
	strlcpy(fmt->description, s5k5ca_formats[index].description,
					sizeof(fmt->description));
	fmt->pixelformat = s5k5ca_formats[index].pixelformat;
	

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
	struct s5k5ca_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix2 = &sensor->pix;
	struct i2c_client *client = sensor->i2c_client;

	isize = s5k5ca_find_size(pix->width, pix->height);
	isize_current = isize;

	pix->width = s5k5ca_settings[isize].frame.x_output_size;
	pix->height = s5k5ca_settings[isize].frame.y_output_size;
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (pix->pixelformat == s5k5ca_formats[ifmt].pixelformat)
			break;
	}
	if (ifmt == NUM_CAPTURE_FORMATS)
		ifmt = 0;
	pix->pixelformat = s5k5ca_formats[ifmt].pixelformat;
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
	struct s5k5ca_sensor *sensor = s->priv;
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
		
	struct s5k5ca_sensor *sensor = s->priv;
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
		
	struct s5k5ca_frame_settings *frm;

	frm = &s5k5ca_settings[isize_current].frame;
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
	
	struct s5k5ca_frame_settings *frm;

	frm = &s5k5ca_settings[isize_current].frame;
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
	struct s5k5ca_frame_settings *frm;

	frm = &s5k5ca_settings[isize_current].frame;
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
	struct s5k5ca_sensor *sensor = s->priv;
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
	
	struct s5k5ca_sensor *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	int err = 0;

	sensor->timeperframe = *timeperframe;
	s5k5casensor_calc_xclk();
	s5k5ca_update_clocks(xclk_current, isize_current);
	err = s5k5ca_set_framerate(s, &sensor->timeperframe);
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
	struct s5k5ca_sensor *sensor = s->priv;

	return sensor->pdata->priv_data_set(s, p);

}

static int ioctl_priv_g_crop(struct v4l2_int_device *s, struct v4l2_crop *f)
{
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u32 data_x0,data_x1,data_y0,data_y1;
	int rval=0;
		
	memset(&f->c, 0, sizeof(struct v4l2_rect));		

	s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x002C, 0x7000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x002E, 0x0232, I2C_16BIT);
	s5k5ca_read_reg(client, I2C_16BIT, 0x0F12, &data_x1);
	s5k5ca_read_reg(client, I2C_16BIT, 0x0F12, &data_y1);
	s5k5ca_read_reg(client, I2C_16BIT, 0x0F12, &data_x0);
	s5k5ca_read_reg(client, I2C_16BIT, 0x0F12, &data_y0);

	f->c.left 	= data_x0;
	f->c.top 	= data_y0;
	f->c.width 	= data_x1;
	f->c.height = data_y1;

	return rval;
}


static int ioctl_priv_s_crop(struct v4l2_int_device *s, struct v4l2_crop *f)
{
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval=0;

	s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x0028, 0x7000, I2C_16BIT);
	s5k5ca_write_reg(client, 0x002A, 0x0232, I2C_16BIT);
	s5k5ca_write_reg(client, 0x0F12, f->c.width, I2C_16BIT);
	s5k5ca_write_reg(client, 0x0F12, f->c.height, I2C_16BIT);
	s5k5ca_write_reg(client, 0x0F12, f->c.left, I2C_16BIT);
	s5k5ca_write_reg(client, 0x0F12, f->c.top, I2C_16BIT);
	s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
	mdelay(10);
			
	return rval;
}


static int __s5k5ca_power_off_standby(struct v4l2_int_device *s,
				      enum v4l2_power on)
{
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

	rval = sensor->pdata->power_set(s, on);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			S5K5CA_DRIVER_NAME " sensor\n");
		return rval;
	}

	sensor->pdata->set_xclk(s, 0);
	return 0;
}

static int s5k5ca_power_off(struct v4l2_int_device *s)
{
	first_time=0;
	
	return __s5k5ca_power_off_standby(s, V4L2_POWER_OFF);
}

static int s5k5ca_power_standby(struct v4l2_int_device *s)
{
	//s5k5ca_setup_pll(s);//&*&*&*JJ1
	return __s5k5ca_power_off_standby(s, V4L2_POWER_STANDBY);
}

static int s5k5ca_power_on(struct v4l2_int_device *s)
{
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int rval;

	sensor->pdata->set_xclk(s, xclk_current);

	rval = sensor->pdata->power_set(s, V4L2_POWER_ON);
	if (rval < 0) {
		v4l_err(client, "Unable to set the power state: "
			S5K5CA_DRIVER_NAME " sensor\n");
		sensor->pdata->set_xclk(s, 0);
		return rval;
	}

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
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	struct vcontrol *lvc;
	int i;

	switch (on) {
	case V4L2_POWER_ON:
		/*if(s5k5ca_settings[isize_current].frame.context == 1)
		{
			s5k5ca_power_off(s);
			s5k5ca_power_standby(s);
		}*/
		
		s5k5ca_power_on(s);
		if (current_power_state == V4L2_POWER_STANDBY) {
			sensor->resuming = true;
			/* Exit software standby. */
			if(first_time == 1)
			{
				mdelay(1);
				s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
				s5k5ca_write_reg(client, 0x107E, 0x0000, I2C_16BIT);
				/* Disable mipi out. */
				//s5k5ca_write_reg(client, 0x014C, 0x0000, I2C_16BIT);
				mdelay(10);
			}

			s5k5ca_configure(s);
		}
		break;
	case V4L2_POWER_OFF:
		s5k5ca_power_off(s);

		/* Reset defaults for controls */
		i = find_vctrl(V4L2_CID_EXPOSURE);
		if (i >= 0) {
			lvc = &s5k5casensor_video_control[i];
			lvc->current_value = S5K5CA_DEF_EXPOSURE;
		}
		i = find_vctrl(V4L2_CID_COLORFX);
		if (i >= 0) {
			lvc = &s5k5casensor_video_control[i];
			lvc->current_value = S5K5CA_DEF_EFFECT;
		}
		i = find_vctrl(V4L2_CID_DO_WHITE_BALANCE);
		if (i >= 0) {
			lvc = &s5k5casensor_video_control[i];
			lvc->current_value = S5K5CA_DEF_WB;
		}
		i = find_vctrl(V4L2_CID_BRIGHTNESS);
		if (i >= 0) {
			lvc = &s5k5casensor_video_control[i];
			lvc->current_value = S5K5CA_DEF_BR;
		}
		i = find_vctrl(V4L2_CID_POWER_LINE_FREQUENCY);
		if (i >= 0) {
			lvc = &s5k5casensor_video_control[i];
			lvc->current_value = S5K5CA_DEF_BANDING;
		}
		break;
	case V4L2_POWER_STANDBY:
		/* Pause preview and capture. */
		if (current_power_state == V4L2_POWER_ON) {
			s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0028, 0x7000, I2C_16BIT);
			/* Pause preview. */
			if(s5k5ca_settings[isize_current].frame.context == 1)
			{
				s5k5ca_write_reg(client, 0x002A, 0x0220, I2C_16BIT);
				s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);
				s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
			}
			/* Pause capture. */
			else
			{
				s5k5ca_write_reg(client, 0x002A, 0x0224, I2C_16BIT);
				s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);
				s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
				s5k5ca_write_reg(client, 0x002A, 0x0220, I2C_16BIT);
				s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);
				s5k5ca_write_reg(client, 0x0F12, 0x0001, I2C_16BIT);
			}
			
			mdelay(100);
			
			s5k5ca_write_reg(client, 0x0028, 0xD000, I2C_16BIT);
			s5k5ca_write_reg(client, 0x002A, 0x1000, I2C_16BIT);
			s5k5ca_write_reg(client, 0x0F12, 0x0000, I2C_16BIT);
			
			if(work_mode == NIGHT_MODE)
				mdelay(300);
			else
				mdelay(100);
			
			/* disable mipi out. */
			//s5k5ca_write_reg(client, 0x014C, 0x0000, I2C_16BIT);
			
			/* Enter software standby. */
			s5k5ca_write_reg(client, 0xFCFC, 0xD000, I2C_16BIT);
			s5k5ca_write_reg(client, 0x107E, 0x0001, I2C_16BIT);

			if(work_mode == NIGHT_MODE)
				mdelay(300);
			else
				mdelay(100);
		}

		s5k5ca_power_standby(s);
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
 * Initialize the sensor device (call s5k5ca_configure())
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
 * s5k5ca device could be found, otherwise returns appropriate error.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct s5k5ca_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int err;
	
	err = s5k5ca_power_standby(s);
	if (err)
		return -ENODEV;

	err = s5k5ca_power_on(s);
	if (err)
		return -ENODEV;

	err = s5k5ca_detect(client);
	if (err < 0) {
		v4l_err(client, "Unable to detect "
				S5K5CA_DRIVER_NAME " sensor\n");

		/*
		 * Turn power off before leaving the function.
		 * If not, CAM Pwrdm will be ON which is not needed
		 * as there is no sensor detected.
		 */
		s5k5ca_power_off(s);

		return err;
	}
	sensor->ver = err;
	v4l_info(client, S5K5CA_DRIVER_NAME
		" chip version 0x%02x detected\n", sensor->ver);

	err = s5k5ca_power_off(s);
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
	struct s5k5ca_sensor *sensor = s->priv;
	
printk(KERN_DEBUG "(s5k5ca.c)(%s)sensor->pix.width=%d,sensor->pix.height=%d  \n",__FUNCTION__,sensor->pix.width,sensor->pix.height);		
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frms->pixel_format == s5k5ca_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frms->index >= S5K5CA_MODES_COUNT)
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width =
		s5k5ca_settings[frms->index].frame.x_output_size;
	frms->discrete.height =
		s5k5ca_settings[frms->index].frame.y_output_size;

	return 0;
}

static const struct v4l2_fract s5k5ca_frameintervals[] = {
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
		if (frmi->pixel_format == s5k5ca_formats[ifmt].pixelformat)
			break;
	}

	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frmi->index >= ARRAY_SIZE(s5k5ca_frameintervals))
		return -EINVAL;

	/* Make sure that the 2MP size reports a max of 10fps */
	if (frmi->width == S5K5CA_IMAGE_WIDTH_MAX &&
	    frmi->height == S5K5CA_IMAGE_HEIGHT_MAX) {
		if (frmi->index != 0)
			return -EINVAL;
	}

	frmi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	frmi->discrete.numerator =
				s5k5ca_frameintervals[frmi->index].numerator;
	frmi->discrete.denominator =
				s5k5ca_frameintervals[frmi->index].denominator;

	return 0;
}

static struct v4l2_int_ioctl_desc s5k5ca_ioctl_desc[] = {
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

static struct v4l2_int_slave s5k5ca_slave = {
	.ioctls = s5k5ca_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(s5k5ca_ioctl_desc),
};

static struct v4l2_int_device s5k5ca_int_device = {
	.module = THIS_MODULE,
	.name = S5K5CA_DRIVER_NAME,
	.priv = &s5k5ca,
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &s5k5ca_slave,
	},
};

/**
 * s5k5ca_probe - sensor driver i2c probe handler
 * @client: i2c driver client device structure
 *
 * Register sensor as an i2c client device and V4L2
 * device.
 */
static int __devinit s5k5ca_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct s5k5ca_sensor *sensor = &s5k5ca;
	int err;

	if (i2c_get_clientdata(client))
		return -EBUSY;

	sensor->pdata = client->dev.platform_data;

	if (!sensor->pdata) {
		v4l_err(client, "no platform data?\n");
		return -ENODEV;
	}

	sensor->v4l2_int_device = &s5k5ca_int_device;
	sensor->i2c_client = client;

	i2c_set_clientdata(client, sensor);

	/* Make the default capture format QCIF V4L2_PIX_FMT_UYVY */
	sensor->pix.width = S5K5CA_IMAGE_WIDTH_MAX;
	sensor->pix.height = S5K5CA_IMAGE_HEIGHT_MAX;
	sensor->pix.pixelformat = V4L2_PIX_FMT_UYVY;
	

	err = v4l2_int_device_register(sensor->v4l2_int_device);
	if (err)
		i2c_set_clientdata(client, NULL);

	return 0;
}

/**
 * s5k5ca_remove - sensor driver i2c remove handler
 * @client: i2c driver client device structure
 *
 * Unregister sensor as an i2c client device and V4L2
 * device.  Complement of s5k5ca_probe().
 */
static int __exit s5k5ca_remove(struct i2c_client *client)
{
	struct s5k5ca_sensor *sensor = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id s5k5ca_id[] = {
	{ S5K5CA_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5k5ca_id);

static struct i2c_driver s5k5casensor_i2c_driver = {
	.driver = {
		.name = S5K5CA_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = s5k5ca_probe,					//boot
	.remove = __exit_p(s5k5ca_remove),
	.id_table = s5k5ca_id,
};

static struct s5k5ca_sensor s5k5ca = {
	.timeperframe = {
		.numerator = 1,
		.denominator = 30,
	},
};

#if S5K5CA_REG_TURN

struct s5k5ca_proc_priv
{
	int len;
	char *buf;
};

static struct s5k5ca_proc_priv init_priv =
{
	.len = sizeof(initial_list),
	.buf = (char *)initial_list,
};

static ssize_t
s5k5ca_proc_file_read(struct file *file, char __user *buf, size_t nbytes,
	       loff_t *ppos)
{
	struct inode * inode = file->f_path.dentry->d_inode;
	char 	*page;
	ssize_t	retval=0;
	ssize_t	n, count;
	struct proc_dir_entry * dp;
	unsigned long long pos;
	char *src;
	struct s5k5ca_proc_priv *priv = 0;

	/*
	 * Gaah, please just use "seq_file" instead. The legacy /proc
	 * interfaces cut loff_t down to off_t for reads, and ignore
	 * the offset entirely for writes..
	 */
	pos = *ppos;
	
	printk(KERN_ERR "%s pos is %d\n", __func__, pos);
	printk(KERN_ERR "%s nbytes is %d\n", __func__, nbytes);
	
	dp = PDE(inode);
	priv = (struct s5k5ca_proc_priv *)dp->data;
	if(priv == 0) return -ENOMEM;
	
	src = priv->buf;
	if(src == 0) return -ENOMEM;
	
	if (pos >= priv->len)
		return 0;

	if (nbytes > priv->len - pos)
		nbytes = priv->len - pos;
		
	printk(KERN_ERR "%s now nbytes is %d\n", __func__, nbytes);
	
	if (!(page = (char*) __get_free_page(GFP_TEMPORARY)))
		return -ENOMEM;

	while (nbytes > 0) {
		count = min_t(size_t, PAGE_SIZE, nbytes);
		n = priv->len - *ppos;
		
		printk(KERN_ERR "%s need read %d bytes.\n", __func__, n);
		
		if(n == 0)
		{
			if (retval == 0)
				retval = -EFAULT;
			break;
		}

		n = min_t(size_t, n, count);
		memcpy(page, src + (*ppos), n);
		printk(KERN_ERR "%s memcpy %d bytes.\n", __func__, n);
		
 		n -= copy_to_user(buf, page, n);
		printk(KERN_ERR "%s copy to user %d bytes.\n", __func__, n);
		if(n == 0)
		{
			if (retval == 0)
				retval = -EFAULT;
			break;
		}

		*ppos += n;
		nbytes -= n;
		buf += n;
		retval += n;
	}

	free_page((unsigned long) page);
	return retval;
}

const static struct s5k5ca_reg reg_end[] = {
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}
};

static ssize_t
s5k5ca_proc_file_write(struct file *file, const char __user *buf,
		size_t nbytes, loff_t *ppos)
{
	struct inode * inode = file->f_path.dentry->d_inode;
	char 	*page;
	ssize_t	retval=0;
	ssize_t	n, count;
	struct proc_dir_entry * dp;
	unsigned long long pos;
	char *dst;
	struct s5k5ca_proc_priv *priv = 0;

	/*
	 * Gaah, please just use "seq_file" instead. The legacy /proc
	 * interfaces cut loff_t down to off_t for reads, and ignore
	 * the offset entirely for writes..
	 */
	pos = *ppos;
	
	printk(KERN_ERR "%s pos is %d\n", __func__, pos);
	printk(KERN_ERR "%s nbytes is %d\n", __func__, nbytes);
	
	dp = PDE(inode);
	priv = (struct s5k5ca_proc_priv *)dp->data;
	if(priv == 0) return -ENOMEM;
	
	dst = priv->buf;
	if(dst == 0) return -ENOMEM;
	
	if (pos >= priv->len)
		return -ENOMEM;

	if (nbytes > priv->len - pos)
		nbytes = priv->len - pos;
		
	printk(KERN_ERR "%s now nbytes is %d\n", __func__, nbytes);

	if (!(page = (char*) __get_free_page(GFP_TEMPORARY)))
		return -ENOMEM;

	while (nbytes > 0) {
		count = min_t(size_t, PAGE_SIZE, nbytes);
		n = priv->len - *ppos;
		
		printk(KERN_ERR "%s need write %d bytes.\n", __func__, n);
		
		if(n == 0)
		{
			if (retval == 0)
				retval = -EFAULT;
			break;
		}

		n = min_t(size_t, n, count);
		
 		n -= copy_from_user(page, buf, n);
		printk(KERN_ERR "%s copy from user %d bytes.\n", __func__, n);
		if(n == 0)
		{
			if (retval == 0)
				retval = -EFAULT;
			break;
		}
		
		memcpy(dst + (*ppos), page, n);
		printk(KERN_ERR "%s memcpy %d bytes.\n", __func__, n);

		*ppos += n;
		nbytes -= n;
		buf += n;
		retval += n;
	}
	
	memcpy(priv->buf + priv->len - sizeof(reg_end), (char *)reg_end, sizeof(reg_end));

	free_page((unsigned long) page);
	return retval;
}


static loff_t
s5k5ca_proc_file_lseek(struct file *file, loff_t offset, int orig)
{
	struct inode * inode = file->f_path.dentry->d_inode;
	struct proc_dir_entry * dp;
	struct s5k5ca_proc_priv *priv = 0;
	loff_t retval = -EINVAL;
	
	dp = PDE(inode);
	priv = (struct s5k5ca_proc_priv *)dp->data;
	if(priv == 0) return retval;
	
	switch (orig) {
	case 1:
		offset += file->f_pos;
	/* fallthrough */
	case 0:
		if (offset < 0 || offset > priv->len)
			break;
		file->f_pos = retval = offset;
	}
	return retval;
}

static const struct file_operations s5k5ca_proc_file_operations = {
	.llseek		= s5k5ca_proc_file_lseek,
	.read		= s5k5ca_proc_file_read,
	.write		= s5k5ca_proc_file_write,
};

static int __init s5k5ca_proc_init(void)
{
	int rv = 0;
	
	s5k5ca_dir = proc_mkdir(MODULE_NAME,NULL);
	if(s5k5ca_dir == NULL){
			rv = -ENOMEM;
			goto out;
	}
	//s5k5ca_dir->owner = THIS_MODULE;
	s5k5ca_file = proc_create_data("init", 0666, s5k5ca_dir, &s5k5ca_proc_file_operations, &init_priv);
	if(s5k5ca_file == NULL){
			rv = -ENOMEM;
			goto no_s5k5ca;
	}

	//s5k5ca_file->owner = THIS_MODULE;

	printk(KERN_ERR "%s initialised\n",MODULE_NAME);
	return 0;
no_s5k5ca:
	remove_proc_entry("init",s5k5ca_dir);
	remove_proc_entry(MODULE_NAME,NULL);
out:
	return rv;
}

#endif

/**
 * s5k5casensor_init - sensor driver module_init handler
 *
 * Registers driver as an i2c client driver.  Returns 0 on success,
 * error code otherwise.
 */
static int __init s5k5casensor_init(void)
{
	int err;

#if S5K5CA_REG_TURN
	err = s5k5ca_proc_init();
	if (err) {
		printk(KERN_ERR "Failed to init" S5K5CA_DRIVER_NAME " proc.\n");
		return err;
	}
#endif

	err = i2c_add_driver(&s5k5casensor_i2c_driver);
	if (err) {
		printk(KERN_ERR "Failed to register" S5K5CA_DRIVER_NAME ".\n");

#if S5K5CA_REG_TURN		
		remove_proc_entry("init",s5k5ca_dir);
		remove_proc_entry(MODULE_NAME,NULL);
#endif

		return err;
	}
	return 0;
}
//late_initcall(s5k5casensor_init);
module_init(s5k5casensor_init);
/**
 * s5k5casensor_cleanup - sensor driver module_exit handler
 *
 * Unregisters/deletes driver as an i2c client driver.
 * Complement of s5k5casensor_init.
 */
static void __exit s5k5casensor_cleanup(void)
{
	i2c_del_driver(&s5k5casensor_i2c_driver);
#if S5K5CA_REG_TURN
	remove_proc_entry("init",s5k5ca_dir);
	remove_proc_entry(MODULE_NAME,NULL);
#endif
}
module_exit(s5k5casensor_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("s5k5ca camera sensor driver");
