/*
 * mt9d115_regs.h
 *
 * Register definitions for the MT9D115 Sensor.
 *
 * Leverage MT9P012.h
 *
 * Copyright (C) 2008 Hewlett Packard.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef MT9D115_REGS_H
#define MT9D115_REGS_H

/* The ID values we are looking for */
#define MT9D115_MOD_ID			0x2580	/* chip ID */
#define MT9D115_MFR_ID			0x000B

/* MT9D115 has 8/16/32 I2C registers */
#define I2C_8BIT			1
#define I2C_16BIT			2
#define I2C_32BIT			4

/* Terminating list entry for reg */
#define I2C_REG_TERM		0xFFFF
/* Terminating list entry for val */
#define I2C_VAL_TERM		0xFFFFFFFF
/* Terminating list entry for len */
#define I2C_LEN_TERM		0xFFFF

/* CSI2 Virtual ID */
#define MT9D115_CSI2_VIRTUAL_ID	0x0

/* Used registers */
#define MT9D115_REG_MODEL_ID				0x0000
#define MT9D115_REG_MODE_SELECT			0x0100
#define MT9D115_REG_SW_RESET				0x001A
#define MT9D115_REG_COARSE_INT_TIME		0x0202

#define MT9D115_REG_ANALOG_GAIN_GLOBAL	0x0204


/*
 * The nominal xclk input frequency of the MT9D115 is 24MHz, maximum
 * frequency is 54MHz, and minimum frequency is 6MHz.
 */
#define MT9D115_XCLK_MIN   	6000000
#define MT9D115_XCLK_MAX   	54000000
#define MT9D115_XCLK_NOM_1 	24000000
#define MT9D115_XCLK_NOM_2 	24000000

/* FPS Capabilities */
#define MT9D115_MIN_FPS		7
#define MT9D115_DEF_FPS		15
#define MT9D115_MAX_FPS		30

#define I2C_RETRY_COUNT		5

/* Still capture 2 MP */
#define MT9D115_IMAGE_WIDTH_MAX	1600
#define MT9D115_IMAGE_HEIGHT_MAX	1200

/* Analog gain values */
#define MT9D115_EV_MIN_GAIN		0
#define MT9D115_EV_MAX_GAIN		30
#define MT9D115_EV_DEF_GAIN		21
#define MT9D115_EV_GAIN_STEP		1
/* maximum index in the gain EVT */
#define MT9D115_EV_TABLE_GAIN_MAX	30

/* Exposure time values */
//#define MT9D115_MIN_EXPOSURE		250
//#define MT9D115_MAX_EXPOSURE		128000
//#define MT9D115_DEF_EXPOSURE	    33000
//#define MT9D115_EXPOSURE_STEP	50
#define MT9D115_MIN_EXPOSURE		-20
#define MT9D115_MAX_EXPOSURE		20
#define MT9D115_DEF_EXPOSURE	    	0
#define MT9D115_EXPOSURE_STEP		10

/* Test Pattern Values */
#define MT9D115_MIN_TEST_PATT_MODE	0
#define MT9D115_MAX_TEST_PATT_MODE	4
#define MT9D115_MODE_TEST_PATT_STEP	1

#define MT9D115_TEST_PATT_SOLID_COLOR 	1
#define MT9D115_TEST_PATT_COLOR_BAR	2
#define MT9D115_TEST_PATT_PN9		4 

/* Effect Values */
#define MT9D115_MIN_EFFECT	0
#define MT9D115_MAX_EFFECT	4
#define MT9D115_EFFECT_STEP	1
#define MT9D115_DEF_EFFECT	0
#define	V4L2_COLORFX_NEGATIVE	3
#define	V4L2_COLORFX_SOLARIZE	4

/* White Balance Values */
#define MT9D115_MIN_WB		0
#define MT9D115_MAX_WB		7
#define MT9D115_WB_STEP		1
#define MT9D115_DEF_WB		0
#define WHITE_BALANCE_AUTO		0
#define WHITE_BALANCE_INCANDESCENT	1
#define WHITE_BALANCE_FLUORESCENT	2
#define WHITE_BALANCE_DAYLIGHT		3
#define WHITE_BALANCE_SHADE		4
#define WHITE_BALANCE_CLOUDY_DAYLIGHT	5
#define WHITE_BALANCE_HORIZON		6
#define WHITE_BALANCE_TUNGSTEN		7

/* Brightness Values */
#define MT9D115_MIN_BR		0
#define MT9D115_MAX_BR		64
#define MT9D115_BR_STEP		1
#define MT9D115_DEF_BR		32

/* Antibanding Values */
#define MT9D115_MIN_BANDING	0
#define MT9D115_MAX_BANDING	2
#define MT9D115_BANDING_STEP	1
#define MT9D115_DEF_BANDING	0

#define MT9D115_MAX_FRAME_LENGTH_LINES	0xFFFF

#define SENSOR_DETECTED		1
#define SENSOR_NOT_DETECTED	0
	
#define MT9D115_MCU_ADDRESS								0x098C
#define MT9D115_MCU_DATA_0								0x0990
#define MT9D115_MCU_SEQ_CAP_MODE					0xA115
#define MT9D115_MCU_SEQ_CMD								0xA103
#define MT9D115_MCU_DATA_PREVIEW					0x0001
#define MT9D115_MCU_DATA_CAPTURE					0x0002
#define MT9D115_MCU_DATA_REFRESH					0x0005

#define MT9D115_MCU_MODE_CROP_X0_A 				0x2739
#define MT9D115_MCU_MODE_CROP_X1_A 				0x273B
#define MT9D115_MCU_MODE_CROP_Y0_A 				0x273D
#define MT9D115_MCU_MODE_CROP_Y1_A 				0x273F
#define MT9D115_MCU_MODE_OUTPUT_WIDTH_A		0x2703
#define MT9D115_MCU_MODE_OUTPUT_HEIGHT_A	0x2705


#define MT9D115_MCU_MODE_CROP_X0_B 				0x2747
#define MT9D115_MCU_MODE_CROP_X1_B 				0x2749
#define MT9D115_MCU_MODE_CROP_Y0_B 				0x274B
#define MT9D115_MCU_MODE_CROP_Y1_B 				0x274D
#define MT9D115_MCU_MODE_OUTPUT_WIDTH_B		0x2707
#define MT9D115_MCU_MODE_OUTPUT_HEIGHT_B	0x2709

#define MT9D115_MCU_MODE_AE_BASETARGET 		0xA24F
#define MT9D115_MCU_MODE_SEQ_MODE 				0xA102

#define CONTEXT_A		1
#define CONTEXT_B		(0x1<<1)
//#define RATIO_16_9		(0x1<<2)
//#define RATIO_3_2		(0x1<<3)
//#define RATIO_5_3		(0x1<<4)
//#define RATIO_5_4		(0x1<<5)
//#define RATIO_11_9		(0x1<<6)
//#define RATIO_62_35		(0x1<<7)



/**
 * struct mt9d115_reg - mt9d115 register format
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 * @length: length of the register
 *
 * Define a structure for MT9D115 register initialization values
 */
struct mt9d115_reg {
	u16 	reg;
	u32 	val;
	u16	length;
};

/**
 * struct struct clk_settings - struct for storage of sensor
 * clock settings
 */
struct mt9d115_clk_settings {
	u16	pre_pll_div;
	u16	pll_mult;
	u16	post_pll_div;
	u16	vt_pix_clk_div;
	u16	vt_sys_clk_div;
};

/**
 * struct struct mipi_settings - struct for storage of sensor
 * mipi settings
 */
struct mt9d115_mipi_settings {
	u16	data_lanes;
	u16	ths_prepare;
	u16	ths_zero;
	u16	ths_settle_lower;
	u16	ths_settle_upper;
};

/**
 * struct struct frame_settings - struct for storage of sensor
 * frame settings
 */
struct mt9d115_frame_settings {
	u16	frame_len_lines_min;
	u16	frame_len_lines;
	u16	line_len_pck;
	u16	x_addr_start;
	u16	x_addr_end;
	u16	y_addr_start;
	u16	y_addr_end;
	u16	x_output_size;
	u16	y_output_size;
	u16	x_even_inc;
	u16	x_odd_inc;
	u16	y_even_inc;
	u16	y_odd_inc;
	u16	v_mode_add;
	u16	h_mode_add;
	u16	h_add_ave;
	u16	context;//Context: 1=A;2=B the normal ratio 4:3
};

/**
 * struct struct mt9d115_sensor_settings - struct for storage of
 * sensor settings.
 */
struct mt9d115_sensor_settings {
	struct mt9d115_clk_settings clk;
	struct mt9d115_mipi_settings mipi;
	struct mt9d115_frame_settings frame;
};

/**
 * struct struct mt9d115_clock_freq - struct for storage of sensor
 * clock frequencies
 */
struct mt9d115_clock_freq {
	u32 vco_clk;
	u32 mipi_clk;
	u32 ddr_clk;
	u32 vt_pix_clk;
};

/* Effect Settings */
const static struct mt9d115_reg colorfx_none[] = {
	{0x098C, 0x2759, I2C_16BIT},
	{0x0990, 0x6440, I2C_16BIT},
	{0x098C, 0x275B, I2C_16BIT},
	{0x0990, 0x6440, I2C_16BIT},
//	{0x098C, 0xA103, I2C_16BIT},
//	{0x0990, 0x0005, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
const static struct mt9d115_reg colorfx_bw[] = {
	{0x098C, 0x2759, I2C_16BIT},
	{0x0990, 0x6441, I2C_16BIT},
	{0x098C, 0x275B, I2C_16BIT},
	{0x0990, 0x6441, I2C_16BIT},
//	{0x098C, 0xA103, I2C_16BIT},
//	{0x0990, 0x0005, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
const static struct mt9d115_reg colorfx_sepia[] = {
	{0x098C, 0x2759, I2C_16BIT},
	{0x0990, 0x6442, I2C_16BIT},
	{0x098C, 0x275B, I2C_16BIT},
	{0x0990, 0x6442, I2C_16BIT},
//	{0x098C, 0xA103, I2C_16BIT},
//	{0x0990, 0x0005, I2C_16BIT},
	{0x098C, 0x2763, I2C_16BIT},
	{0x0990, 0xB023, I2C_16BIT},
//	{0x098C, 0xA103, I2C_16BIT},
//	{0x0990, 0x0006, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
const static struct mt9d115_reg colorfx_negative[] = {
	{0x098C, 0x2759, I2C_16BIT},
	{0x0990, 0x6443, I2C_16BIT},
	{0x098C, 0x275B, I2C_16BIT},
	{0x0990, 0x6443, I2C_16BIT},
//	{0x098C, 0xA103, I2C_16BIT},
//	{0x0990, 0x0005, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};     
const static struct mt9d115_reg colorfx_solarize[] = {
	{0x098C, 0x2759, I2C_16BIT},
	{0x0990, 0x6444, I2C_16BIT},
	{0x098C, 0x275B, I2C_16BIT},
	{0x0990, 0x6444, I2C_16BIT},
//	{0x098C, 0xA103, I2C_16BIT},
//	{0x0990, 0x0005, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       


/* White Balance settings */
const static struct mt9d115_reg wb_auto[] = {	//settings from Rich Power on 6/1
	{0x098C, 0xA11F, I2C_16BIT},    // MCU_ADDRESS [SEQ_PREVIEW_1_AWB]        
	{0x0990, 0x0001, I2C_16BIT},    // MCU_DATA_0                             
	{0x098C, 0xA351, I2C_16BIT},    // MCU_ADDRESS [AWB_CCM_POSITION_MIN]     
	{0x0990, 0x0000, I2C_16BIT},    // MCU_DATA_0                             
	{0x098C, 0xA352, I2C_16BIT},    // MCU_ADDRESS [AWB_CCM_POSITION_MAX]     
	{0x0990, 0x007F, I2C_16BIT},    // MCU_DATA_0                             
	{0x098C, 0xA34A, I2C_16BIT},    // MCU_ADDRESS [AWB_GAIN_MIN]             
	{0x0990, 0x0078, I2C_16BIT},    // MCU_DATA_0                             
	{0x098C, 0xA34B, I2C_16BIT},    // MCU_ADDRESS [AWB_GAIN_MAX]             
	{0x0990, 0x00C8, I2C_16BIT},    // MCU_DATA_0                             
	{0x098C, 0xA34C, I2C_16BIT},    // MCU_ADDRESS [AWB_GAINMIN_B]            
	{0x0990, 0x0059, I2C_16BIT},    // MCU_DATA_0                             
	{0x098C, 0xA34D, I2C_16BIT},    // MCU_ADDRESS [AWB_GAINMAX_B]            
	{0x0990, 0x00AA, I2C_16BIT},    // MCU_DATA_0                             
//	{0x098C, 0xA103, I2C_16BIT},    // MCU_ADDRESS [SEQ_CMD]                  
//	{0x0990, 0x0005, I2C_16BIT},    // MCU_DATA_0                             
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};     
const static struct mt9d115_reg wb_incandescent[] = {	//settings from Rich Power on 6/1
	{0x098C, 0xA11F, I2C_16BIT},	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]     
	{0x0990, 0x0000, I2C_16BIT},	// MCU_DATA_0                          
	{0x098C, 0xA102, I2C_16BIT},	// MCU_ADDRESS [SEQ_MODE]              
	{0x0990, 0x000B, I2C_16BIT},	// MCU_DATA_0                          
	{0x098C, 0xA351, I2C_16BIT},	// MCU_ADDRESS [AWB_CCM_POSITION_MIN]  
	{0x0990, 0x0000, I2C_16BIT},    // MCU_DATA_0                          
	{0x098C, 0xA352, I2C_16BIT},	// MCU_ADDRESS [AWB_CCM_POSITION_MAX]  
	{0x0990, 0x0000, I2C_16BIT},    // MCU_DATA_0                          
	{0x098C, 0xA34A, I2C_16BIT},	// MCU_ADDRESS [AWB_GAIN_MIN]          
	{0x0990, 0x0086, I2C_16BIT},    // MCU_DATA_0                          
	{0x098C, 0xA34B, I2C_16BIT},	// MCU_ADDRESS [AWB_GAIN_MAX]          
	{0x0990, 0x0086, I2C_16BIT},	// MCU_DATA_0                          
	{0x098C, 0xA34C, I2C_16BIT},	// MCU_ADDRESS [AWB_GAINMIN_B]         
	{0x0990, 0x00C9, I2C_16BIT},	// MCU_DATA_0                          
	{0x098C, 0xA34D, I2C_16BIT},	// MCU_ADDRESS [AWB_GAINMAX_B]         
	{0x0990, 0x00C9, I2C_16BIT},    // MCU_DATA_0                          
//	{0x098C, 0xA103, I2C_16BIT},	// MCU_ADDRESS [SEQ_CMD]               
//	{0x0990, 0x0005, I2C_16BIT},    // MCU_DATA_0                          
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
const static struct mt9d115_reg wb_fluorescent[] = {	//settings from Rich Power on 6/1
	{0x098C, 0xA11F, I2C_16BIT},    // MCU_ADDRESS [SEQ_PREVIEW_1_AWB]    
	{0x0990, 0x0000, I2C_16BIT},    // MCU_DATA_0                         
	{0x098C, 0xA102, I2C_16BIT},    // MCU_ADDRESS [SEQ_MODE]             
	{0x0990, 0x000B, I2C_16BIT},    // MCU_DATA_0                         
	{0x098C, 0xA351, I2C_16BIT},    // MCU_ADDRESS [AWB_CCM_POSITION_MIN] 
	{0x0990, 0x0000, I2C_16BIT},    // MCU_DATA_0                         
	{0x098C, 0xA352, I2C_16BIT},    // MCU_ADDRESS [AWB_CCM_POSITION_MAX] 
	{0x0990, 0x0000, I2C_16BIT},    // MCU_DATA_0                         
	{0x098C, 0xA34A, I2C_16BIT},    // MCU_ADDRESS [AWB_GAIN_MIN]         
	{0x0990, 0x00AE, I2C_16BIT},    // MCU_DATA_0                         
	{0x098C, 0xA34B, I2C_16BIT},    // MCU_ADDRESS [AWB_GAIN_MAX]         
	{0x0990, 0x00AE, I2C_16BIT},    // MCU_DATA_0                         
	{0x098C, 0xA34C, I2C_16BIT},    // MCU_ADDRESS [AWB_GAINMIN_B]        
	{0x0990, 0x00B2, I2C_16BIT},    // MCU_DATA_0                         
	{0x098C, 0xA34D, I2C_16BIT},    // MCU_ADDRESS [AWB_GAINMAX_B]        
	{0x0990, 0x00B2, I2C_16BIT},    // MCU_DATA_0                         
//	{0x098C, 0xA103, I2C_16BIT},    // MCU_ADDRESS [SEQ_CMD]              
//	{0x0990, 0x0005, I2C_16BIT},    // MCU_DATA_0                         

	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
const static struct mt9d115_reg wb_daylight[] = {	//settings from Rich Power on 6/1
	{0x098C, 0xA11F, I2C_16BIT},    // MCU_ADDRESS [SEQ_PREVIEW_1_AWB]      
	{0x0990, 0x0000, I2C_16BIT},    // MCU_DATA_0                           
	{0x098C, 0xA102, I2C_16BIT},    // MCU_ADDRESS [SEQ_MODE]               
	{0x0990, 0x000B, I2C_16BIT},    // MCU_DATA_0                           
	{0x098C, 0xA351, I2C_16BIT},    // MCU_ADDRESS [AWB_CCM_POSITION_MIN]   
	{0x0990, 0x007F, I2C_16BIT},    // MCU_DATA_0                           
	{0x098C, 0xA352, I2C_16BIT},    // MCU_ADDRESS [AWB_CCM_POSITION_MAX]   
	{0x0990, 0x007F, I2C_16BIT},    // MCU_DATA_0                           
	{0x098C, 0xA34A, I2C_16BIT},    // MCU_ADDRESS [AWB_GAIN_MIN]           
	{0x0990, 0x009E, I2C_16BIT},    // MCU_DATA_0                           
	{0x098C, 0xA34B, I2C_16BIT},    // MCU_ADDRESS [AWB_GAIN_MAX]           
	{0x0990, 0x009E, I2C_16BIT},    // MCU_DATA_0                           
	{0x098C, 0xA34C, I2C_16BIT},    // MCU_ADDRESS [AWB_GAINMIN_B]          
	{0x0990, 0x0096, I2C_16BIT},    // MCU_DATA_0                           
	{0x098C, 0xA34D, I2C_16BIT},    // MCU_ADDRESS [AWB_GAINMAX_B]          
	{0x0990, 0x0096, I2C_16BIT},    // MCU_DATA_0                           
//	{0x098C, 0xA103, I2C_16BIT},    // MCU_ADDRESS [SEQ_CMD]                
//	{0x0990, 0x0005, I2C_16BIT},    // MCU_DATA_0                           
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       


/* Antibanding Settings */
const static struct mt9d115_reg flicker_off[] = {
	{0x098C, 0xA11E, I2C_16BIT},
	{0x0990, 0x0001, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
const static struct mt9d115_reg flicker_50hz[] = {
	{0x098C, 0xA11E, I2C_16BIT},
	{0x0990, 0x0002, I2C_16BIT},
	{0x098C, 0xA404, I2C_16BIT},
	{0x0990, 0x0090, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
const static struct mt9d115_reg flicker_60hz[] = {
	{0x098C, 0xA11E, I2C_16BIT},
	{0x0990, 0x0002, I2C_16BIT},
	{0x098C, 0xA404, I2C_16BIT},
	{0x0990, 0x00F0, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};   

/* Resolution & Frame rate  Settings */
const static struct mt9d115_reg res2barcode_992x560[] = {
	{0x098C, 0x270D, I2C_16BIT},
	{0x0990, 0x0000, I2C_16BIT},
	{0x098C, 0x270F, I2C_16BIT},
	{0x0990, 0x0004, I2C_16BIT},
	{0x098C, 0x2711, I2C_16BIT},
	{0x0990, 0x04BD, I2C_16BIT},
	{0x098C, 0x2713, I2C_16BIT},
	{0x0990, 0x03EB, I2C_16BIT},
	{0x098C, 0x2715, I2C_16BIT},
	{0x0990, 0x0111, I2C_16BIT},
	{0x098C, 0x2717, I2C_16BIT},
	{0x0990, 0x002E, I2C_16BIT},	//mirror
//	{0x098C, 0x2717, I2C_16BIT},
//	{0x0990, 0x002C, I2C_16BIT},	//mirror
////	{0x098C, 0x2717, I2C_16BIT},
////	{0x0990, 0x002D, I2C_16BIT},	//mirror
////	{0x098C, 0x272D, I2C_16BIT},
////	{0x0990, 0x0025, I2C_16BIT},	//mirror
	{0x098C, 0x2719, I2C_16BIT},
	{0x0990, 0x003A, I2C_16BIT},
	{0x098C, 0x271B, I2C_16BIT},
	{0x0990, 0x00F6, I2C_16BIT},
	{0x098C, 0x271D, I2C_16BIT},
	{0x0990, 0x008B, I2C_16BIT},
	{0x098C, 0x271F, I2C_16BIT},
	{0x0990, 0x02B5, I2C_16BIT},
	{0x098C, 0x2721, I2C_16BIT},
	{0x0990, 0x07E4, I2C_16BIT},
	{0x098C, 0x222D, I2C_16BIT},
	{0x0990, 0x00AD, I2C_16BIT},
	{0x098C, 0xA408, I2C_16BIT},
	{0x0990, 0x002A, I2C_16BIT},
	{0x098C, 0xA409, I2C_16BIT},
	{0x0990, 0x002C, I2C_16BIT},
	{0x098C, 0xA40A, I2C_16BIT},
	{0x0990, 0x0033, I2C_16BIT},
	{0x098C, 0xA40B, I2C_16BIT},
	{0x0990, 0x0035, I2C_16BIT},
	{0x098C, 0x2411, I2C_16BIT},
	{0x0990, 0x00AD, I2C_16BIT},
	{0x098C, 0x2413, I2C_16BIT},
	{0x0990, 0x00D0, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};

	
const static struct mt9d115_reg res720p_1280x720[] = {
	{0x098C, 0x270D, I2C_16BIT},
	{0x0990, 0x00F6, I2C_16BIT},
	{0x098C, 0x270F, I2C_16BIT},
	{0x0990, 0x00A6, I2C_16BIT},
	{0x098C, 0x2711, I2C_16BIT},
	{0x0990, 0x03CD, I2C_16BIT},
	{0x098C, 0x2713, I2C_16BIT},
	{0x0990, 0x05AD, I2C_16BIT},
	{0x098C, 0x2717, I2C_16BIT},
	{0x0990, 0x0026, I2C_16BIT},	//mirror
	{0x098C, 0x272D, I2C_16BIT},
	{0x0990, 0x0026, I2C_16BIT},	//mirror
//	{0x098C, 0x2717, I2C_16BIT},
//	{0x0990, 0x0024, I2C_16BIT},	//mirror
////	{0x098C, 0x2717, I2C_16BIT},
////	{0x0990, 0x0025, I2C_16BIT},	//mirror
	{0x098C, 0x2719, I2C_16BIT},
	{0x0990, 0x003A, I2C_16BIT},
	{0x098C, 0x271B, I2C_16BIT},
	{0x0990, 0x00F6, I2C_16BIT},
	{0x098C, 0x271D, I2C_16BIT},
	{0x0990, 0x008B, I2C_16BIT},
	{0x098C, 0x271F, I2C_16BIT},
	{0x0990, 0x032D, I2C_16BIT},
	{0x098C, 0x2721, I2C_16BIT},
//	{0x0990, 0x0756, I2C_16BIT},
	{0x0990, 0x06DB, I2C_16BIT},
	{0x098C, 0x222D, I2C_16BIT},
	{0x0990, 0x00BA, I2C_16BIT},
	{0x098C, 0xA408, I2C_16BIT},
	{0x0990, 0x002D, I2C_16BIT},
	{0x098C, 0xA409, I2C_16BIT},
	{0x0990, 0x002F, I2C_16BIT},
	{0x098C, 0xA40A, I2C_16BIT},
	{0x0990, 0x0037, I2C_16BIT},
	{0x098C, 0xA40B, I2C_16BIT},
	{0x0990, 0x0039, I2C_16BIT},
	{0x098C, 0x2411, I2C_16BIT},
	{0x0990, 0x00BA, I2C_16BIT},
	{0x098C, 0x2413, I2C_16BIT},
	{0x0990, 0x00E0, I2C_16BIT},
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
    

const static struct mt9d115_reg initial_pll[] = {
	{0x001A, 0x0051, I2C_16BIT}, 	// RESET_AND_MISC_CONTROL
	{0x001A, 0x0050, I2C_16BIT}, 	// RESET_AND_MISC_CONTROL
	{0x0014, 0x2545, I2C_16BIT}, 	// PLL_CONTROL
	{0x0010, 0x0115, I2C_16BIT}, 	// PLL_DIVIDERS
	{0x0012, 0x00F5, I2C_16BIT}, 	// PLL_P_DIVIDERS
	{0x0014, 0x2547, I2C_16BIT}, 	// PLL_CONTROL
	{0x0014, 0x2447, I2C_16BIT}, 	// PLL_CONTROL
	{0x0014, 0x2047, I2C_16BIT}, 	// PLL_CONTROL
	{0x0014, 0x2046, I2C_16BIT}, 	// PLL_CONTROL
	{0x001A, 0x0058, I2C_16BIT}, 	// RESET_AND_MISC_CONTROL
	{0x0018, 0x4028, I2C_16BIT}, 	// STANDBY_CONTROL
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       

const static struct mt9d115_reg initial_list[] = {
	{0x321C, 0x0000, I2C_16BIT}, 	// OFIFO_CONTROL_STATUS                                   
//	{0x098C, 0x2703, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_WIDTH_A]                                                   
//	{0x0990, 0x0320, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x2705, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_HEIGHT_A]                                                  
//	{0x0990, 0x0258, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x2707, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_WIDTH_B]                                                   
//	{0x0990, 0x0640, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x2709, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_HEIGHT_B]                                                  
//	{0x0990, 0x04B0, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x270D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_START_A]                                               
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x270F, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_COL_START_A]                                               
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2711, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_END_A]                                                 
	{0x0990, 0x04BD, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2713, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_COL_END_A]                                                 
	{0x0990, 0x064D, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2715, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_SPEED_A]                                               
	{0x0990, 0x0111, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x2717, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_A]                                               
//	{0x0990, 0x046C, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2719, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_CORRECTION_A]                                         
	{0x0990, 0x005A, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x271B, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_IT_MIN_A]                                             
	{0x0990, 0x01BE, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x271D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_IT_MAX_MARGIN_A]                                      
	{0x0990, 0x0131, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x271F, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]                                            
	{0x0990, 0x02BB, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2721, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_LINE_LENGTH_PCK_A]                                         
	{0x0990, 0x0888, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2723, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_START_B]                                               
	{0x0990, 0x0004, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2725, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_COL_START_B]                                               
	{0x0990, 0x0004, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2727, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_END_B]                                                 
	{0x0990, 0x04BB, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2729, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_COL_END_B]                                                 
	{0x0990, 0x064B, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x272B, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_SPEED_B]                                               
	{0x0990, 0x0111, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x272D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]                                               
//	{0x0990, 0x0024, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x272F, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_CORRECTION_B]                                         
	{0x0990, 0x003A, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2731, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_IT_MIN_B]                                             
	{0x0990, 0x00F6, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2733, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_IT_MAX_MARGIN_B]                                      
	{0x0990, 0x008B, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2735, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_B]                                            
	{0x0990, 0x0521, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2737, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_LINE_LENGTH_PCK_B]                                         
	{0x0990, 0x0888, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x2739, I2C_16BIT}, 	// MCU_ADDRESS [MODE_CROP_X0_A]                                                        
//	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x273B, I2C_16BIT}, 	// MCU_ADDRESS [MODE_CROP_X1_A]                                                        
//	{0x0990, 0x031F, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x273D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_CROP_Y0_A]                                                        
//	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x273F, I2C_16BIT}, 	// MCU_ADDRESS [MODE_CROP_Y1_A]                                                        
//	{0x0990, 0x0257, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x2747, I2C_16BIT}, 	// MCU_ADDRESS [MODE_CROP_X0_B]                                                        
//	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x2749, I2C_16BIT}, 	// MCU_ADDRESS [MODE_CROP_X1_B]                                                        
//	{0x0990, 0x063F, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x274B, I2C_16BIT}, 	// MCU_ADDRESS [MODE_CROP_Y0_B]                                                        
//	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                                                                          
//	{0x098C, 0x274D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_CROP_Y1_B]                                                        
//	{0x0990, 0x04AF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2222, I2C_16BIT}, 	// MCU_ADDRESS [AE_R9]                                                                 
	{0x0990, 0x00A0, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA408, I2C_16BIT}, 	// MCU_ADDRESS [FD_SEARCH_F1_50]                                                       
	{0x0990, 0x0026, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA409, I2C_16BIT}, 	// MCU_ADDRESS [FD_SEARCH_F2_50]                                                       
	{0x0990, 0x0029, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA40A, I2C_16BIT}, 	// MCU_ADDRESS [FD_SEARCH_F1_60]                                                       
	{0x0990, 0x002E, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA40B, I2C_16BIT}, 	// MCU_ADDRESS [FD_SEARCH_F2_60]                                                       
	{0x0990, 0x0031, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2411, I2C_16BIT}, 	// MCU_ADDRESS [FD_R9_STEP_F60_A]                                                      
	{0x0990, 0x00A0, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2413, I2C_16BIT}, 	// MCU_ADDRESS [FD_R9_STEP_F50_A]                                                      
	{0x0990, 0x00C0, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2415, I2C_16BIT}, 	// MCU_ADDRESS [FD_R9_STEP_F60_B]                                                      
	{0x0990, 0x00A0, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2417, I2C_16BIT}, 	// MCU_ADDRESS [FD_R9_STEP_F50_B]                                                      
	{0x0990, 0x00C0, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA404, I2C_16BIT}, 	// MCU_ADDRESS [FD_MODE]                                                               
	{0x0990, 0x0010, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA40D, I2C_16BIT}, 	// MCU_ADDRESS [FD_STAT_MIN]                                                           
	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA40E, I2C_16BIT}, 	// MCU_ADDRESS [FD_STAT_MAX]                                                           
	{0x0990, 0x0003, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA410, I2C_16BIT}, 	// MCU_ADDRESS [FD_MIN_AMPLITUDE]                                                      
	{0x0990, 0x000A, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA117, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_PREVIEW_0_AE]                                                      
	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA11D, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]                                                      
	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA129, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_PREVIEW_3_AE]                                                      
	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA24F, I2C_16BIT}, 	// MCU_ADDRESS [AE_BASETARGET]                                                         
	{0x0990, 0x0032, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA20C, I2C_16BIT}, 	// MCU_ADDRESS [AE_MAX_INDEX]                                                          
	{0x0990, 0x0010, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA216, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x0091, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA20E, I2C_16BIT}, 	// MCU_ADDRESS [AE_MAX_VIRTGAIN]                                                       
	{0x0990, 0x0091, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2212, I2C_16BIT}, 	// MCU_ADDRESS [AE_MAX_DGAIN_AE1]                                                      
	{0x0990, 0x00A4, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x3210, 0x01B8, I2C_16BIT}, 	// COLOR_PIPELINE_CONTROL                                                              
	{0x098C, 0xAB36, I2C_16BIT}, 	// MCU_ADDRESS [HG_CLUSTERDC_TH]                                                       
	{0x0990, 0x0014, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2B66, I2C_16BIT}, 	// MCU_ADDRESS [HG_CLUSTER_DC_BM]                                                      
	{0x0990, 0x2AF8, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB20, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_SAT1]                                                            
	{0x0990, 0x0080, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB24, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_SAT2]                                                            
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB21, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_INTERPTHRESH1]                                                   
	{0x0990, 0x000A, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB25, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_INTERPTHRESH2]                                                   
	{0x0990, 0x002A, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB22, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_APCORR1]                                                         
	{0x0990, 0x0007, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB26, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_APCORR2]                                                         
	{0x0990, 0x0001, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB23, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_APTHRESH1]                                                       
	{0x0990, 0x0004, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB27, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_APTHRESH2]                                                       
	{0x0990, 0x0009, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2B28, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTART]                                                 
	{0x0990, 0x0BB8, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2B2A, I2C_16BIT}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTOP]                                                  
	{0x0990, 0x2968, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB2C, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_START_R]                                                         
	{0x0990, 0x00FF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB30, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_STOP_R]                                                          
	{0x0990, 0x00FF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB2D, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_START_G]                                                         
	{0x0990, 0x00FF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB31, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_STOP_G]                                                          
	{0x0990, 0x00FF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB2E, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_START_B]                                                         
	{0x0990, 0x00FF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB32, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_STOP_B]                                                          
	{0x0990, 0x00FF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB2F, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_START_OL]                                                        
	{0x0990, 0x000A, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB33, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_STOP_OL]                                                         
	{0x0990, 0x0006, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB34, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_GAINSTART]                                                       
	{0x0990, 0x0020, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB35, I2C_16BIT}, 	// MCU_ADDRESS [HG_NR_GAINSTOP]                                                        
	{0x0990, 0x0091, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA765, I2C_16BIT}, 	// MCU_ADDRESS [MODE_COMMONMODESETTINGS_FILTER_MODE]                                   
	{0x0990, 0x0006, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB37, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_MORPH_CTRL]                                                   
	{0x0990, 0x0003, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2B38, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMASTARTMORPH]                                                    
	{0x0990, 0x2968, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2B3A, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMASTOPMORPH]                                                     
	{0x0990, 0x2D50, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2B62, I2C_16BIT}, 	// MCU_ADDRESS [HG_FTB_START_BM]                                                       
	{0x0990, 0xFFFE, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2B64, I2C_16BIT}, 	// MCU_ADDRESS [HG_FTB_STOP_BM]                                                        
	{0x0990, 0xFFFF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB4F, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_0]                                                    
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB50, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_1]                                                    
	{0x0990, 0x0013, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB51, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_2]                                                    
	{0x0990, 0x0027, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB52, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_3]                                                    
	{0x0990, 0x0043, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB53, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_4]                                                    
	{0x0990, 0x0068, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB54, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_5]                                                    
	{0x0990, 0x0081, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB55, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_6]                                                    
	{0x0990, 0x0093, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB56, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_7]                                                    
	{0x0990, 0x00A3, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB57, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_8]                                                    
	{0x0990, 0x00B0, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB58, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_9]                                                    
	{0x0990, 0x00BC, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB59, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_10]                                                   
	{0x0990, 0x00C7, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB5A, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_11]                                                   
	{0x0990, 0x00D1, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB5B, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_12]                                                   
	{0x0990, 0x00DA, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB5C, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_13]                                                   
	{0x0990, 0x00E2, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB5D, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_14]                                                   
	{0x0990, 0x00E9, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB5E, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_15]                                                   
	{0x0990, 0x00EF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB5F, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_16]                                                   
	{0x0990, 0x00F4, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB60, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_17]                                                   
	{0x0990, 0x00FA, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xAB61, I2C_16BIT}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_18]                                                   
	{0x0990, 0x00FF, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2306, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_0]                                                           
	{0x0990, 0x01D6, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2308, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_1]                                                           
	{0x0990, 0xFF89, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x230A, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_2]                                                           
	{0x0990, 0xFFA1, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x230C, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_3]                                                           
	{0x0990, 0xFF73, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x230E, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_4]                                                           
	{0x0990, 0x019C, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2310, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_5]                                                           
	{0x0990, 0xFFF1, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2312, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_6]                                                           
	{0x0990, 0xFFB0, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2314, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_7]                                                           
	{0x0990, 0xFF2D, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2316, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_8]                                                           
	{0x0990, 0x0223, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2318, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_9]                                                           
	{0x0990, 0x001C, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x231A, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_10]                                                          
	{0x0990, 0x0048, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2318, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_9]                                                           
	{0x0990, 0x001C, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x231A, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_10]                                                          
	{0x0990, 0x0038, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2318, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_9]                                                           
	{0x0990, 0x001E, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x231A, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_10]                                                          
	{0x0990, 0x0038, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2318, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_9]                                                           
	{0x0990, 0x0022, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x231A, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_10]                                                          
	{0x0990, 0x0038, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2318, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_9]                                                           
	{0x0990, 0x002C, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x231A, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_10]                                                          
	{0x0990, 0x0038, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2318, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_9]                                                           
	{0x0990, 0x0024, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x231A, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_L_10]                                                          
	{0x0990, 0x0038, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x231C, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_0]                                                          
	{0x0990, 0xFFCD, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x231E, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_1]                                                          
	{0x0990, 0x0023, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2320, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_2]                                                          
	{0x0990, 0x0010, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2322, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_3]                                                          
	{0x0990, 0x0026, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2324, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_4]                                                          
	{0x0990, 0xFFE9, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2326, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_5]                                                          
	{0x0990, 0xFFF1, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2328, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_6]                                                          
	{0x0990, 0x003A, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x232A, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_7]                                                          
	{0x0990, 0x005D, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x232C, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_8]                                                          
	{0x0990, 0xFF69, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x232E, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_9]                                                          
	{0x0990, 0x000C, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2330, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_10]                                                         
	{0x0990, 0xFFE4, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x232E, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_9]                                                          
	{0x0990, 0x000C, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2330, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_10]                                                         
	{0x0990, 0xFFF4, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x232E, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_9]                                                          
	{0x0990, 0x000A, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2330, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_10]                                                         
	{0x0990, 0xFFF4, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x232E, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_9]                                                          
	{0x0990, 0x0006, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2330, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_10]                                                         
	{0x0990, 0xFFF4, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x232E, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_9]                                                          
	{0x0990, 0xFFFC, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2330, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_10]                                                         
	{0x0990, 0xFFF4, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x232E, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_9]                                                          
	{0x0990, 0x0004, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x2330, I2C_16BIT}, 	// MCU_ADDRESS [AWB_CCM_RL_10]                                                         
	{0x0990, 0xFFF4, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0x0415, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0xF601, I2C_16BIT}, 	                                                                                       
	{0x0992, 0x42C1, I2C_16BIT}, 	                                                                                       
	{0x0994, 0x0326, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x11F6, I2C_16BIT}, 	                                                                                       
	{0x0998, 0x0143, I2C_16BIT}, 	                                                                                       
	{0x099A, 0xC104, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x260A, I2C_16BIT}, 	                                                                                       
	{0x099E, 0xCC04, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0425, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x33BD, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xA362, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xBD04, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x3339, I2C_16BIT}, 	                                                                                       
	{0x0998, 0xC6FF, I2C_16BIT}, 	                                                                                       
	{0x099A, 0xF701, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x6439, I2C_16BIT}, 	                                                                                       
	{0x099E, 0xFE01, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0435, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x6918, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xCE03, I2C_16BIT}, 	                                                                                       
	{0x0994, 0x25CC, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x0013, I2C_16BIT}, 	                                                                                       
	{0x0998, 0xBDC2, I2C_16BIT}, 	                                                                                       
	{0x099A, 0xB8CC, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x0489, I2C_16BIT}, 	                                                                                       
	{0x099E, 0xFD03, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0445, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x27CC, I2C_16BIT}, 	                                                                                       
	{0x0992, 0x0325, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xFD01, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x69FE, I2C_16BIT}, 	                                                                                       
	{0x0998, 0x02BD, I2C_16BIT}, 	                                                                                       
	{0x099A, 0x18CE, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x0339, I2C_16BIT}, 	                                                                                       
	{0x099E, 0xCC00, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0455, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x11BD, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xC2B8, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xCC04, I2C_16BIT}, 	                                                                                       
	{0x0996, 0xC8FD, I2C_16BIT}, 	                                                                                       
	{0x0998, 0x0347, I2C_16BIT}, 	                                                                                       
	{0x099A, 0xCC03, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x39FD, I2C_16BIT}, 	                                                                                       
	{0x099E, 0x02BD, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0465, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0xDE00, I2C_16BIT}, 	                                                                                       
	{0x0992, 0x18CE, I2C_16BIT}, 	                                                                                       
	{0x0994, 0x00C2, I2C_16BIT}, 	                                                                                       
	{0x0996, 0xCC00, I2C_16BIT}, 	                                                                                       
	{0x0998, 0x37BD, I2C_16BIT}, 	                                                                                       
	{0x099A, 0xC2B8, I2C_16BIT}, 	                                                                                       
	{0x099C, 0xCC04, I2C_16BIT}, 	                                                                                       
	{0x099E, 0xEFDD, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0475, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0xE6CC, I2C_16BIT}, 	                                                                                       
	{0x0992, 0x00C2, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xDD00, I2C_16BIT}, 	                                                                                       
	{0x0996, 0xC601, I2C_16BIT}, 	                                                                                       
	{0x0998, 0xF701, I2C_16BIT}, 	                                                                                       
	{0x099A, 0x64C6, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x03F7, I2C_16BIT}, 	                                                                                       
	{0x099E, 0x0165, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0485, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x7F01, I2C_16BIT}, 	                                                                                       
	{0x0992, 0x6639, I2C_16BIT}, 	                                                                                       
	{0x0994, 0x3C3C, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x3C34, I2C_16BIT}, 	                                                                                       
	{0x0998, 0xCC32, I2C_16BIT}, 	                                                                                       
	{0x099A, 0x3EBD, I2C_16BIT}, 	                                                                                       
	{0x099C, 0xA558, I2C_16BIT}, 	                                                                                       
	{0x099E, 0x30ED, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0495, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x04BD, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xB2D7, I2C_16BIT}, 	                                                                                       
	{0x0994, 0x30E7, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x06CC, I2C_16BIT}, 	                                                                                       
	{0x0998, 0x323E, I2C_16BIT}, 	                                                                                       
	{0x099A, 0xED00, I2C_16BIT}, 	                                                                                       
	{0x099C, 0xEC04, I2C_16BIT}, 	                                                                                       
	{0x099E, 0xBDA5, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x04A5, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x44CC, I2C_16BIT}, 	                                                                                       
	{0x0992, 0x3244, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xBDA5, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x585F, I2C_16BIT}, 	                                                                                       
	{0x0998, 0x30ED, I2C_16BIT}, 	                                                                                       
	{0x099A, 0x02CC, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x3244, I2C_16BIT}, 	                                                                                       
	{0x099E, 0xED00, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x04B5, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0xF601, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xD54F, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xEA03, I2C_16BIT}, 	                                                                                       
	{0x0996, 0xAA02, I2C_16BIT}, 	                                                                                       
	{0x0998, 0xBDA5, I2C_16BIT}, 	                                                                                       
	{0x099A, 0x4430, I2C_16BIT}, 	                                                                                       
	{0x099C, 0xE606, I2C_16BIT}, 	                                                                                       
	{0x099E, 0x3838, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x04C5, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x3831, I2C_16BIT}, 	                                                                                       
	{0x0992, 0x39BD, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xD661, I2C_16BIT}, 	                                                                                       
	{0x0996, 0xF602, I2C_16BIT}, 	                                                                                       
	{0x0998, 0xF4C1, I2C_16BIT}, 	                                                                                       
	{0x099A, 0x0126, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x0BFE, I2C_16BIT}, 	                                                                                       
	{0x099E, 0x02BD, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x04D5, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0xEE10, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xFC02, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xF5AD, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x0039, I2C_16BIT}, 	                                                                                       
	{0x0998, 0xF602, I2C_16BIT}, 	                                                                                       
	{0x099A, 0xF4C1, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x0226, I2C_16BIT}, 	                                                                                       
	{0x099E, 0x0AFE, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x04E5, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x02BD, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xEE10, I2C_16BIT}, 	                                                                                       
	{0x0994, 0xFC02, I2C_16BIT}, 	                                                                                       
	{0x0996, 0xF7AD, I2C_16BIT}, 	                                                                                       
	{0x0998, 0x0039, I2C_16BIT}, 	                                                                                       
	{0x099A, 0x3CBD, I2C_16BIT}, 	                                                                                       
	{0x099C, 0xB059, I2C_16BIT}, 	                                                                                       
	{0x099E, 0xCC00, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x04F5, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x28BD, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xA558, I2C_16BIT}, 	                                                                                       
	{0x0994, 0x8300, I2C_16BIT}, 	                                                                                       
	{0x0996, 0x0027, I2C_16BIT}, 	                                                                                       
	{0x0998, 0x0BCC, I2C_16BIT}, 	                                                                                       
	{0x099A, 0x0026, I2C_16BIT}, 	                                                                                       
	{0x099C, 0x30ED, I2C_16BIT}, 	                                                                                       
	{0x099E, 0x00C6, I2C_16BIT}, 	                                                                                       
	{0x098C, 0x0505, I2C_16BIT}, 	// MCU_ADDRESS                                                                         
	{0x0990, 0x03BD, I2C_16BIT}, 	                                                                                       
	{0x0992, 0xA544, I2C_16BIT}, 	                                                                                       
	{0x0994, 0x3839, I2C_16BIT}, 	                                                                                       

	{0x098C, 0x2717, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_A] 	//verticalflip
	{0x0990, 0x046F, I2C_16BIT}, 	// MCU_DATA_0					//verticalflip
	{0x098C, 0x272D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]	//verticalflip
	{0x0990, 0x0027, I2C_16BIT}, 	// MCU_DATA_0					//verticalflip


	{0x098C, 0x2006, I2C_16BIT}, 	// MCU_ADDRESS [MON_ARG1]                                                              
	{0x0990, 0x0415, I2C_16BIT}, 	// MCU_DATA_0                                                                          
	{0x098C, 0xA005, I2C_16BIT}, 	// MCU_ADDRESS [MON_CMD]                                                               
	{0x0990, 0x0001, I2C_16BIT}, 	// MCU_DATA_0                                                                          
                
	{0x0018, 0x0028, I2C_16BIT}, 	// OFIFO_CONTROL_STATUS                                   
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       
	
const static struct mt9d115_reg initial_list_old[] = {
	{0x321C, 0x0000, I2C_16BIT}, 	// OFIFO_CONTROL_STATUS                                   
	{0x098C, 0x270D, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_ROW_START_A]           
	{0x0990, 0x0000, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x270F, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_COL_START_A]           
	{0x0990, 0x0000, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2711, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_ROW_END_A]             
	{0x0990, 0x04BD, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2713, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_COL_END_A]             
	{0x0990, 0x064D, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2715, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_ROW_SPEED_A]           
	{0x0990, 0x0111, I2C_16BIT},  //MCU_DATA_0                                      
//	{0x098C, 0x2717, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_READ_MODE_A]           
//	{0x0990, 0x046C, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2719, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_FINE_CORRECTION_A]     
	{0x0990, 0x005A, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x271B, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_FINE_IT_MIN_A]         
	{0x0990, 0x01BE, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x271D, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_FINE_IT_MAX_MARGIN_A]  
	{0x0990, 0x0131, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x271F, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]        
	{0x0990, 0x02B3, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x2721, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_LINE_LENGTH_PCK_A]     
//	{0x0990, 0x09B0, I2C_16BIT}, 	// MCU_DATA_0
	{0x0990, 0x0679, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x2723, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_ROW_START_B]           
	{0x0990, 0x0004, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2725, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_COL_START_B]           
	{0x0990, 0x0004, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2727, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_ROW_END_B]             
	{0x0990, 0x04BB, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2729, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_COL_END_B]             
	{0x0990, 0x064B, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x272B, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_ROW_SPEED_B]           
	{0x0990, 0x0111, I2C_16BIT},  //MCU_DATA_0                                      
//	{0x098C, 0x272D, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]           
//	{0x0990, 0x0024, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x272F, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_FINE_CORRECTION_B]     
	{0x0990, 0x003A, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2731, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_FINE_IT_MIN_B]         
	{0x0990, 0x00F6, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2733, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_FINE_IT_MAX_MARGIN_B]  
	{0x0990, 0x008B, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2735, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_B]        
	{0x0990, 0x050D, I2C_16BIT},  //MCU_DATA_0                                      
	{0x098C, 0x2737, I2C_16BIT},  //MCU_ADDRESS [MODE_SENSOR_LINE_LENGTH_PCK_B]     
	{0x0990, 0x0C24, I2C_16BIT},  //MCU_DATA_0                                                                       
	{0x098C, 0x222D, I2C_16BIT}, 	// MCU_ADDRESS
//	{0x0990, 0x008D, I2C_16BIT}, 	// MCU_DATA_0
	{0x0990, 0x00D3, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA408, I2C_16BIT},  //MCU_ADDRESS [FD_SEARCH_F1_50]                   
//	{0x0990, 0x0022, I2C_16BIT}, 	// MCU_DATA_0
	{0x0990, 0x0033, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA409, I2C_16BIT},  //MCU_ADDRESS [FD_SEARCH_F2_50]                   
//	{0x0990, 0x0024, I2C_16BIT}, 	// MCU_DATA_0
	{0x0990, 0x0035, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA40A, I2C_16BIT},  //MCU_ADDRESS [FD_SEARCH_F1_60]                   
//	{0x0990, 0x0029, I2C_16BIT}, 	// MCU_DATA_0
	{0x0990, 0x003E, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA40B, I2C_16BIT},  //MCU_ADDRESS [FD_SEARCH_F2_60]                   
//	{0x0990, 0x002B, I2C_16BIT}, 	// MCU_DATA_0
	{0x0990, 0x0040, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x2411, I2C_16BIT},  //MCU_ADDRESS [FD_R9_STEP_F60_A]                  
//	{0x0990, 0x008D, I2C_16BIT}, 	// MCU_DATA_0
	{0x0990, 0x00D3, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x2413, I2C_16BIT},  //MCU_ADDRESS [FD_R9_STEP_F50_A]                  
//	{0x0990, 0x00A9, I2C_16BIT}, 	// MCU_DATA_0
	{0x0990, 0x00FD, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x2415, I2C_16BIT},  //MCU_ADDRESS [FD_R9_STEP_F60_B]                  
	{0x0990, 0x0071, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x2417, I2C_16BIT},  //MCU_ADDRESS [FD_R9_STEP_F50_B]                  
	{0x0990, 0x0087, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA404, I2C_16BIT},  //MCU_ADDRESS [FD_MODE]                           
	{0x0990, 0x0010, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA40D, I2C_16BIT},  //MCU_ADDRESS [FD_STAT_MIN]                       
	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA40E, I2C_16BIT},  //MCU_ADDRESS [FD_STAT_MAX]                       
	{0x0990, 0x0003, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA410, I2C_16BIT},  //MCU_ADDRESS [FD_MIN_AMPLITUDE]                  
	{0x0990, 0x000A, I2C_16BIT}, 	// MCU_DATA_0
	
	{0x098C, 0x2717, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_A] 	//verticalflip
	{0x0990, 0x046F, I2C_16BIT}, 	// MCU_DATA_0					//verticalflip
	{0x098C, 0x272D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]	//verticalflip
	{0x0990, 0x0027, I2C_16BIT}, 	// MCU_DATA_0					//verticalflip
//	{0x3040, 0x046F, I2C_16BIT}, 	// READ_MODE
	
	{0x098C, 0x2755, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]		//init_pixelformat
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0					//init_pixelformat
	{0x098C, 0x2757, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]		//init_pixelformat
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0					//init_pixelformat

	{0x098C, 0xA117, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_PREVIEW_0_AE]		//Rich Power's suggestion on 6/1
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0

	{0x098C, 0xA11D, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]		//Rich Power's suggestion on 6/1
	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0

//PATCH	-- settings from Rich Power on 6/1

	{0x098C, 0x0415, I2C_16BIT},  // MCU_ADDRESS                   
	{0x0990, 0xF601, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x0992, 0x42C1, I2C_16BIT},  // MCU_DATA_1                                                
	{0x0994, 0x0326, I2C_16BIT}, 	// MCU_DATA_2                                
	{0x0996, 0x11F6, I2C_16BIT},  // MCU_DATA_3                                                
	{0x0998, 0x0143, I2C_16BIT}, 	// MCU_DATA_4                                
	{0x099A, 0xC104, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0x260A, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0xCC04, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x0425, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x33BD, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0xA362, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0xBD04, I2C_16BIT},  // MCU_DATA_2                                                
	{0x0996, 0x3339, I2C_16BIT}, 	// MCU_DATA_3                                
	{0x0998, 0xC6FF, I2C_16BIT},  // MCU_DATA_4                                                
	{0x099A, 0xF701, I2C_16BIT}, 	// MCU_DATA_5                                
	{0x099C, 0x6439, I2C_16BIT},  // MCU_DATA_6                                                
	{0x099E, 0xFE01, I2C_16BIT}, 	// MCU_DATA_7                                
	{0x098C, 0x0435, I2C_16BIT},  // MCU_ADDRESS                                               
	{0x0990, 0x6918, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x0992, 0xCE03, I2C_16BIT},  // MCU_DATA_1                                                
	{0x0994, 0x25CC, I2C_16BIT}, 	// MCU_DATA_2                                
	{0x0996, 0x0013, I2C_16BIT},  // MCU_DATA_3                                                
	{0x0998, 0xBDC2, I2C_16BIT}, 	// MCU_DATA_4                                
	{0x099A, 0xB8CC, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0x0489, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0xFD03, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x0445, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x27CC, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0x0325, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0xFD01, I2C_16BIT},  // MCU_DATA_2                                                
	{0x0996, 0x69FE, I2C_16BIT}, 	// MCU_DATA_3                                
	{0x0998, 0x02BD, I2C_16BIT},  // MCU_DATA_4                                                
	{0x099A, 0x18CE, I2C_16BIT}, 	// MCU_DATA_5                                
	{0x099C, 0x0339, I2C_16BIT},  // MCU_DATA_6                                                
	{0x099E, 0xCC00, I2C_16BIT}, 	// MCU_DATA_7                                
	{0x098C, 0x0455, I2C_16BIT},  // MCU_ADDRESS                                               
	{0x0990, 0x11BD, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x0992, 0xC2B8, I2C_16BIT},  // MCU_DATA_1                                                
	{0x0994, 0xCC04, I2C_16BIT}, 	// MCU_DATA_2                                
	{0x0996, 0xC8FD, I2C_16BIT},  // MCU_DATA_3                                                
	{0x0998, 0x0347, I2C_16BIT}, 	// MCU_DATA_4                                
	{0x099A, 0xCC03, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0x39FD, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0x02BD, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x0465, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0xDE00, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0x18CE, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0x00C2, I2C_16BIT},  // MCU_DATA_2                                                
	{0x0996, 0xCC00, I2C_16BIT}, 	// MCU_DATA_3                                
	{0x0998, 0x37BD, I2C_16BIT},  // MCU_DATA_4                                                
	{0x099A, 0xC2B8, I2C_16BIT}, 	// MCU_DATA_5                                
	{0x099C, 0xCC04, I2C_16BIT},  // MCU_DATA_6                                                
	{0x099E, 0xEFDD, I2C_16BIT}, 	// MCU_DATA_7                                
	{0x098C, 0x0475, I2C_16BIT},  // MCU_ADDRESS                                               
	{0x0990, 0xE6CC, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x0992, 0x00C2, I2C_16BIT},  // MCU_DATA_1                                                
	{0x0994, 0xDD00, I2C_16BIT}, 	// MCU_DATA_2                                
	{0x0996, 0xC601, I2C_16BIT},  // MCU_DATA_3                                                
	{0x0998, 0xF701, I2C_16BIT}, 	// MCU_DATA_4                                
	{0x099A, 0x64C6, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0x03F7, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0x0165, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x0485, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x7F01, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0x6639, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0x3C3C, I2C_16BIT},  // MCU_DATA_2                                                
	{0x0996, 0x3C34, I2C_16BIT}, 	// MCU_DATA_3                                
	{0x0998, 0xCC32, I2C_16BIT},  // MCU_DATA_4                                                
	{0x099A, 0x3EBD, I2C_16BIT}, 	// MCU_DATA_5                                
	{0x099C, 0xA558, I2C_16BIT},  // MCU_DATA_6                                                
	{0x099E, 0x30ED, I2C_16BIT}, 	// MCU_DATA_7                                
	{0x098C, 0x0495, I2C_16BIT},  // MCU_ADDRESS                                               
	{0x0990, 0x04BD, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x0992, 0xB2D7, I2C_16BIT},  // MCU_DATA_1                                                
	{0x0994, 0x30E7, I2C_16BIT}, 	// MCU_DATA_2                                
	{0x0996, 0x06CC, I2C_16BIT},  // MCU_DATA_3                                                
	{0x0998, 0x323E, I2C_16BIT}, 	// MCU_DATA_4                                
	{0x099A, 0xED00, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0xEC04, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0xBDA5, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x04A5, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x44CC, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0x3244, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0xBDA5, I2C_16BIT},  // MCU_DATA_2                                                
	{0x0996, 0x585F, I2C_16BIT}, 	// MCU_DATA_3                                
	{0x0998, 0x30ED, I2C_16BIT},  // MCU_DATA_4                                                
	{0x099A, 0x02CC, I2C_16BIT}, 	// MCU_DATA_5                                
	{0x099C, 0x3244, I2C_16BIT},  // MCU_DATA_6                                                
	{0x099E, 0xED00, I2C_16BIT}, 	// MCU_DATA_7                                
	{0x098C, 0x04B5, I2C_16BIT},  // MCU_ADDRESS                                               
	{0x0990, 0xF601, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x0992, 0xD54F, I2C_16BIT},  // MCU_DATA_1                                                
	{0x0994, 0xEA03, I2C_16BIT}, 	// MCU_DATA_2                                
	{0x0996, 0xAA02, I2C_16BIT},  // MCU_DATA_3                                                
	{0x0998, 0xBDA5, I2C_16BIT}, 	// MCU_DATA_4                                
	{0x099A, 0x4430, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0xE606, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0x3838, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x04C5, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x3831, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0x39BD, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0xD661, I2C_16BIT},  // MCU_DATA_2                                                
	{0x0996, 0xF602, I2C_16BIT},  // MCU_DATA_3                                                
	{0x0998, 0xF4C1, I2C_16BIT}, 	// MCU_DATA_4                                
	{0x099A, 0x0126, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0x0BFE, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0x02BD, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x04D5, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0xEE10, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0xFC02, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0xF5AD, I2C_16BIT},  // MCU_DATA_2                                                
	{0x0996, 0x0039, I2C_16BIT}, 	// MCU_DATA_3                                
	{0x0998, 0xF602, I2C_16BIT},  // MCU_DATA_4                                                
	{0x099A, 0xF4C1, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0x0226, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0x0AFE, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x04E5, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x02BD, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0xEE10, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0xFC02, I2C_16BIT},  // MCU_DATA_2                                                
	{0x0996, 0xF7AD, I2C_16BIT}, 	// MCU_DATA_3                                
	{0x0998, 0x0039, I2C_16BIT},  // MCU_DATA_4                                                
	{0x099A, 0x3CBD, I2C_16BIT}, 	// MCU_DATA_5                                
	{0x099C, 0xB059, I2C_16BIT},  // MCU_DATA_6                                                
	{0x099E, 0xCC00, I2C_16BIT}, 	// MCU_DATA_7                                
	{0x098C, 0x04F5, I2C_16BIT},  // MCU_ADDRESS                                               
	{0x0990, 0x28BD, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x0992, 0xA558, I2C_16BIT},  // MCU_DATA_1                                                
	{0x0994, 0x8300, I2C_16BIT}, 	// MCU_DATA_2                                
	{0x0996, 0x0027, I2C_16BIT},  // MCU_DATA_3                                                
	{0x0998, 0x0BCC, I2C_16BIT}, 	// MCU_DATA_4                                
	{0x099A, 0x0026, I2C_16BIT},  // MCU_DATA_5                                                
	{0x099C, 0x30ED, I2C_16BIT}, 	// MCU_DATA_6                                
	{0x099E, 0x00C6, I2C_16BIT},  // MCU_DATA_7                                                
	{0x098C, 0x0505, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x03BD, I2C_16BIT},  // MCU_DATA_0                                                
	{0x0992, 0xA544, I2C_16BIT}, 	// MCU_DATA_1                                
	{0x0994, 0x3839, I2C_16BIT},  // MCU_DATA_2                                                
	{0x098C, 0x2006, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x0415, I2C_16BIT},  // MCU_DATA_0                                                
	{0x098C, 0xA005, I2C_16BIT}, 	// MCU_ADDRESS                               
	{0x0990, 0x0001, I2C_16BIT},  // MCU_DATA_0                                                
	{0x098C, 0xA768, I2C_16BIT}, 	// MCU_ADDRESS [RESERVED_MODE_68]            
	{0x0990, 0x0005, I2C_16BIT},  // MCU_DATA_0                                                

//Char setting -- settings from Rich Power on 6/1

	{0x098C, 0x2715, I2C_16BIT},  // MCU_ADDRESS [MODE_SENSOR_ROW_SPEED_A]                     
	{0x0990, 0x0111, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0x2B28, I2C_16BIT},  // MCU_ADDRESS [HG_LL_BRIGHTNESSSTART]                       
	{0x0990, 0x1E14, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0x2B2A, I2C_16BIT},  // MCU_ADDRESS [HG_LL_BRIGHTNESSSTOP]                        
	{0x0990, 0x6590, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0x2B38, I2C_16BIT},  // MCU_ADDRESS [HG_GAMMASTARTMORPH]                          
	{0x0990, 0x1E14, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0x2B3A, I2C_16BIT},  // MCU_ADDRESS [HG_GAMMASTOPMORPH]                           
	{0x0990, 0x6590, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x3244, 0x0328, I2C_16BIT},  // RESERVED_SOC1_3244                                        
	{0x323E, 0xC22C, I2C_16BIT}, 	// RESERVED_SOC1_323E                        
	{0x098C, 0xA34A, I2C_16BIT},  // MCU_ADDRESS [AWB_GAIN_MIN]                                
	{0x0990, 0x0078, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0xA34B, I2C_16BIT},  // MCU_ADDRESS [AWB_GAIN_MAX]                                
	{0x0990, 0x00C8, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0xA34D, I2C_16BIT},  // MCU_ADDRESS [AWB_GAINMAX_B]                               
	{0x0990, 0x00AA, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0x2411, I2C_16BIT},  // MCU_ADDRESS [FD_R9_STEP_F60_A]                            
	{0x0990, 0x008C, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0x2413, I2C_16BIT},  // MCU_ADDRESS [FD_R9_STEP_F50_A]                            
	{0x0990, 0x00A7, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0x2415, I2C_16BIT},  // MCU_ADDRESS [FD_R9_STEP_F60_B]                            
	{0x0990, 0x005F, I2C_16BIT}, 	// MCU_DATA_0                                
	{0x098C, 0x2417, I2C_16BIT},  // MCU_ADDRESS [FD_R9_STEP_F50_B]                            
	{0x0990, 0x0064, I2C_16BIT}, 	// MCU_DATA_0                                

//AWB -- settings from Rich Power on 6/1

	{0x098C, 0x2306, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_0]                          
	{0x0990, 0x0202, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2308, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_1]                          
	{0x0990, 0xFF2A, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x230A, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_2]                          
	{0x0990, 0x000D, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x230C, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_3]                          
	{0x0990, 0xFFD8, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x230E, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_4]                          
	{0x0990, 0x01BF, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2310, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_5]                          
	{0x0990, 0xFFBB, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2312, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_6]                          
	{0x0990, 0x0029, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2314, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_7]                          
	{0x0990, 0xFF21, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2316, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_8]                          
	{0x0990, 0x01E2, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2318, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_9]                          
	{0x0990, 0x0020, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x231A, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_L_10]                         
	{0x0990, 0x0031, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x231C, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_0]                         
	{0x0990, 0xFFEE, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x231E, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_1]                         
	{0x0990, 0xFFF7, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2320, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_2]                         
	{0x0990, 0x000E, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2322, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_3]                         
	{0x0990, 0x000C, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2324, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_4]                         
	{0x0990, 0x0022, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2326, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_5]                         
	{0x0990, 0xFFC3, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2328, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_6]                         
	{0x0990, 0xFFED, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x232A, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_7]                         
	{0x0990, 0x00A4, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x232C, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_8]                         
	{0x0990, 0xFF8B, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x232E, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_9]                         
	{0x0990, 0x0009, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2330, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_RL_10]                        
	{0x0990, 0xFFF7, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA348, I2C_16BIT},  // MCU_ADDRESS [AWB_GAIN_BUFFER_SPEED]                
	{0x0990, 0x0008, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA349, I2C_16BIT},  // MCU_ADDRESS [AWB_JUMP_DIVISOR]                     
	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA351, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_POSITION_MIN]                 
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA352, I2C_16BIT},  // MCU_ADDRESS [AWB_CCM_POSITION_MAX]                 
	{0x0990, 0x007F, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA354, I2C_16BIT},  // MCU_ADDRESS [AWB_SATURATION]                       
	{0x0990, 0x0043, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA355, I2C_16BIT},  // MCU_ADDRESS [AWB_MODE]                             
	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA35D, I2C_16BIT},  // MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MIN]             
	{0x0990, 0x0078, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA35E, I2C_16BIT},  // MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MAX]             
	{0x0990, 0x0086, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA35F, I2C_16BIT},  // MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MIN]              
	{0x0990, 0x007E, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA360, I2C_16BIT},  // MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MAX]              
	{0x0990, 0x0082, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0x2361, I2C_16BIT},  // MCU_ADDRESS [RESERVED_AWB_61]                      
	{0x0990, 0x0040, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA363, I2C_16BIT},  // MCU_ADDRESS [AWB_TG_MIN0]                          
	{0x0990, 0x00D2, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA364, I2C_16BIT},  // MCU_ADDRESS [AWB_TG_MAX0]                          
	{0x0990, 0x00EE, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA302, I2C_16BIT},  // MCU_ADDRESS [AWB_WINDOW_POS]                       
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0                             
	{0x098C, 0xA303, I2C_16BIT},  // MCU_ADDRESS [AWB_WINDOW_SIZE]                      
	{0x0990, 0x00EF, I2C_16BIT}, 	// MCU_DATA_0                             

//LSC -- settings from Rich Power on 5/25

	{0x3210, 0x01B0, I2C_16BIT},  // COLOR_PIPELINE_CONTROL                          
	{0x364E, 0x03F0, I2C_16BIT}, 	// P_GR_P0Q0                     
	{0x3650, 0xB06B, I2C_16BIT},  // P_GR_P0Q1                                       
	{0x3652, 0x5692, I2C_16BIT}, 	// P_GR_P0Q2                     
	{0x3654, 0x49AF, I2C_16BIT},  // P_GR_P0Q3                                       
	{0x3656, 0xB2B4, I2C_16BIT}, 	// P_GR_P0Q4                     
	{0x3658, 0x0010, I2C_16BIT},  // P_RD_P0Q0                                       
	{0x365A, 0xF6AA, I2C_16BIT}, 	// P_RD_P0Q1                     
	{0x365C, 0x02B3, I2C_16BIT},  // P_RD_P0Q2                                       
	{0x365E, 0x4D4F, I2C_16BIT}, 	// P_RD_P0Q3                     
	{0x3660, 0xB694, I2C_16BIT},  // P_RD_P0Q4                                       
	{0x3662, 0x758F, I2C_16BIT}, 	// P_BL_P0Q0                     
	{0x3664, 0xE8CA, I2C_16BIT},  // P_BL_P0Q1                                       
	{0x3666, 0x3232, I2C_16BIT}, 	// P_BL_P0Q2                     
	{0x3668, 0x79CE, I2C_16BIT},  // P_BL_P0Q3                                       
	{0x366A, 0x80B4, I2C_16BIT}, 	// P_BL_P0Q4                     
	{0x366C, 0x7EAF, I2C_16BIT},  // P_GB_P0Q0                                       
	{0x366E, 0xCB25, I2C_16BIT}, 	// P_GB_P0Q1                     
	{0x3670, 0x50D2, I2C_16BIT},  // P_GB_P0Q2                                       
	{0x3672, 0x494F, I2C_16BIT}, 	// P_GB_P0Q3                     
	{0x3674, 0xAF54, I2C_16BIT},  // P_GB_P0Q4                                       
	{0x3676, 0x4B6D, I2C_16BIT}, 	// P_GR_P1Q0                     
	{0x3678, 0xA64E, I2C_16BIT},  // P_GR_P1Q1                                       
	{0x367A, 0xF2D0, I2C_16BIT}, 	// P_GR_P1Q2                     
	{0x367C, 0x45D0, I2C_16BIT},  // P_GR_P1Q3                                       
	{0x367E, 0x6653, I2C_16BIT}, 	// P_GR_P1Q4                     
	{0x3680, 0x1DCE, I2C_16BIT},  // P_RD_P1Q0                                       
	{0x3682, 0x0B6E, I2C_16BIT}, 	// P_RD_P1Q1                     
	{0x3684, 0xF68F, I2C_16BIT},  // P_RD_P1Q2                                       
	{0x3686, 0xDBCF, I2C_16BIT}, 	// P_RD_P1Q3                     
	{0x3688, 0x20D3, I2C_16BIT},  // P_RD_P1Q4                                       
	{0x368A, 0xEB0A, I2C_16BIT}, 	// P_BL_P1Q0                     
	{0x368C, 0x164C, I2C_16BIT},  // P_BL_P1Q1                                       
	{0x368E, 0xA1B1, I2C_16BIT}, 	// P_BL_P1Q2                     
	{0x3690, 0xAA50, I2C_16BIT},  // P_BL_P1Q3                                       
	{0x3692, 0x2F34, I2C_16BIT}, 	// P_BL_P1Q4                     
	{0x3694, 0x750B, I2C_16BIT},  // P_GB_P1Q0                                       
	{0x3696, 0x3E4E, I2C_16BIT}, 	// P_GB_P1Q1                     
	{0x3698, 0xDD90, I2C_16BIT},  // P_GB_P1Q2                                       
	{0x369A, 0x166F, I2C_16BIT}, 	// P_GB_P1Q3                     
	{0x369C, 0x5CF3, I2C_16BIT},  // P_GB_P1Q4                                       
	{0x369E, 0x1D93, I2C_16BIT}, 	// P_GR_P2Q0                     
	{0x36A0, 0xECCC, I2C_16BIT},  // P_GR_P2Q1                                       
	{0x36A2, 0xDE56, I2C_16BIT}, 	// P_GR_P2Q2                     
	{0x36A4, 0x0FF2, I2C_16BIT},  // P_GR_P2Q3                                       
	{0x36A6, 0x1339, I2C_16BIT}, 	// P_GR_P2Q4                     
	{0x36A8, 0x2E53, I2C_16BIT},  // P_RD_P2Q0                                       
	{0x36AA, 0xD6AD, I2C_16BIT}, 	// P_RD_P2Q1                     
	{0x36AC, 0xC1D6, I2C_16BIT},  // P_RD_P2Q2                                       
	{0x36AE, 0x2593, I2C_16BIT}, 	// P_RD_P2Q3                     
	{0x36B0, 0x5D18, I2C_16BIT},  // P_RD_P2Q4                                       
	{0x36B2, 0x0EF3, I2C_16BIT}, 	// P_BL_P2Q0                     
	{0x36B4, 0x134F, I2C_16BIT},  // P_BL_P2Q1                                       
	{0x36B6, 0xC716, I2C_16BIT}, 	// P_BL_P2Q2                     
	{0x36B8, 0xE010, I2C_16BIT},  // P_BL_P2Q3                                       
	{0x36BA, 0x08D9, I2C_16BIT}, 	// P_BL_P2Q4                     
	{0x36BC, 0x1D53, I2C_16BIT},  // P_GB_P2Q0                                       
	{0x36BE, 0x940F, I2C_16BIT}, 	// P_GB_P2Q1                     
	{0x36C0, 0xD2F6, I2C_16BIT},  // P_GB_P2Q2                                       
	{0x36C2, 0x3CD2, I2C_16BIT}, 	// P_GB_P2Q3                     
	{0x36C4, 0x0B99, I2C_16BIT},  // P_GB_P2Q4                                       
	{0x36C6, 0xDFD1, I2C_16BIT}, 	// P_GR_P3Q0                     
	{0x36C8, 0x0B51, I2C_16BIT},  // P_GR_P3Q1                                       
	{0x36CA, 0x16F5, I2C_16BIT}, 	// P_GR_P3Q2                     
	{0x36CC, 0xF1F2, I2C_16BIT},  // P_GR_P3Q3                                       
	{0x36CE, 0xA837, I2C_16BIT}, 	// P_GR_P3Q4                     
	{0x36D0, 0xA131, I2C_16BIT},  // P_RD_P3Q0                                       
	{0x36D2, 0x4750, I2C_16BIT}, 	// P_RD_P3Q1                     
	{0x36D4, 0x0714, I2C_16BIT},  // P_RD_P3Q2                                       
	{0x36D6, 0xC133, I2C_16BIT}, 	// P_RD_P3Q3                     
	{0x36D8, 0x9DD6, I2C_16BIT},  // P_RD_P3Q4                                       
	{0x36DA, 0xD770, I2C_16BIT}, 	// P_BL_P3Q0                     
	{0x36DC, 0xB72F, I2C_16BIT},  // P_BL_P3Q1                                       
	{0x36DE, 0x26D5, I2C_16BIT}, 	// P_BL_P3Q2                     
	{0x36E0, 0x5513, I2C_16BIT},  // P_BL_P3Q3                                       
	{0x36E2, 0xE917, I2C_16BIT}, 	// P_BL_P3Q4                     
	{0x36E4, 0xA511, I2C_16BIT},  // P_GB_P3Q0                                       
	{0x36E6, 0x2C91, I2C_16BIT}, 	// P_GB_P3Q1                     
	{0x36E8, 0x0035, I2C_16BIT},  // P_GB_P3Q2                                       
	{0x36EA, 0x86B5, I2C_16BIT}, 	// P_GB_P3Q3                     
	{0x36EC, 0x8E77, I2C_16BIT},  // P_GB_P3Q4                                       
	{0x36EE, 0xD1D5, I2C_16BIT}, 	// P_GR_P4Q0                     
	{0x36F0, 0x0A73, I2C_16BIT},  // P_GR_P4Q1                                       
	{0x36F2, 0x1D99, I2C_16BIT}, 	// P_GR_P4Q2                     
	{0x36F4, 0xA4F7, I2C_16BIT},  // P_GR_P4Q3                                       
	{0x36F6, 0x83DB, I2C_16BIT}, 	// P_GR_P4Q4                     
	{0x36F8, 0xAB35, I2C_16BIT},  // P_RD_P4Q0                                       
	{0x36FA, 0x6653, I2C_16BIT}, 	// P_RD_P4Q1                     
	{0x36FC, 0x1E15, I2C_16BIT},  // P_RD_P4Q2                                       
	{0x36FE, 0x84D8, I2C_16BIT}, 	// P_RD_P4Q3                     
	{0x3700, 0xA398, I2C_16BIT},  // P_RD_P4Q4                                       
	{0x3702, 0xA995, I2C_16BIT}, 	// P_BL_P4Q0                     
	{0x3704, 0x2431, I2C_16BIT},  // P_BL_P4Q1                                       
	{0x3706, 0x1699, I2C_16BIT}, 	// P_BL_P4Q2                     
	{0x3708, 0xC5F6, I2C_16BIT},  // P_BL_P4Q3                                       
	{0x370A, 0x887B, I2C_16BIT}, 	// P_BL_P4Q4                     
	{0x370C, 0xCBB5, I2C_16BIT},  // P_GB_P4Q0                                       
	{0x370E, 0x5F53, I2C_16BIT}, 	// P_GB_P4Q1                     
	{0x3710, 0x0D59, I2C_16BIT},  // P_GB_P4Q2                                       
	{0x3712, 0xD3F7, I2C_16BIT}, 	// P_GB_P4Q3                     
	{0x3714, 0xE05A, I2C_16BIT},  // P_GB_P4Q4                                       
	{0x3644, 0x0308, I2C_16BIT}, 	// POLY_ORIGIN_C                 
	{0x3642, 0x027C, I2C_16BIT},  // POLY_ORIGIN_R                                   
	{0x3210, 0x01B8, I2C_16BIT}, 	// COLOR_PIPELINE_CONTROL        


//	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
//	{0x0990, 0x0006, I2C_16BIT}, 	// MCU_DATA_0
//	{0x098C, 0xA103, I2C_16BIT}, 	// // MCU_ADDRESS [SEQ_CMD]
//	{0x0990, 0x0005, I2C_16BIT}, 	// MCU_DATA_0	
	
	
//	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
//	{0x0990, 0x0006, I2C_16BIT}, 	// MCU_DATA_0
//	{0x098C, 0xA103, I2C_16BIT}, 	// // MCU_ADDRESS [SEQ_CMD]
//	{0x0990, 0x0005, I2C_16BIT}, 	// MCU_DATA_0	
//
////mirror
//	{0x098C, 0x2717, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_A]
//	{0x0990, 0x046D, I2C_16BIT}, 	// MCU_DATA_0
//	{0x098C, 0x272D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]
//	{0x0990, 0x0025, I2C_16BIT}, 	// MCU_DATA_0	
//	{0x3040, 0x046D, I2C_16BIT}, 	// READ_MODE
//	{0x098C, 0xA115, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CAP_MODE]
//	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0
//	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
//	{0x0990, 0x0001, I2C_16BIT}, 	// MCU_DATA_0	
//	
////[CB-Y-CR-Y]
////	{0x098C, 0x2755, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
////	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0
////	{0x098C, 0x2757, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
////	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0
////	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
////	{0x0990, 0x0005, I2C_16BIT}, 	// MCU_DATA_0
//
//////[CR-Y-CB-Y]
////	{0x098C, 0x2755, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
////	{0x0990, 0x0001, I2C_16BIT}, 	// MCU_DATA_0
////	{0x098C, 0x2757, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
////	{0x0990, 0x0001, I2C_16BIT}, 	// MCU_DATA_0
////	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
////	{0x0990, 0x0005, I2C_16BIT}, 	// MCU_DATA_0
//
//////[Y-CB-Y-CR]
////	{0x098C, 0x2755, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
////	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0
////	{0x098C, 0x2757, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
////	{0x0990, 0x0002, I2C_16BIT}, 	// MCU_DATA_0
////	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
////	{0x0990, 0x0005, I2C_16BIT}, 	// MCU_DATA_0
//
////[Y-CR-Y-CB](o)
//	{0x098C, 0x2755, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
//	{0x0990, 0x0003, I2C_16BIT}, 	// MCU_DATA_0
//	{0x098C, 0x2757, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
//	{0x0990, 0x0003, I2C_16BIT}, 	// MCU_DATA_0
//	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
//	{0x0990, 0x0005, I2C_16BIT}, 	// MCU_DATA_0
//
//////[RGB565]
////	{0x098C, 0x2755, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
////	{0x0990, 0x0020, I2C_16BIT}, 	// MCU_DATA_0
////	{0x098C, 0x2757, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
////	{0x0990, 0x0020, I2C_16BIT}, 	// MCU_DATA_0
////	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
////	{0x0990, 0x0005, I2C_16BIT}, 	// MCU_DATA_0
	
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       

const static struct mt9d115_reg init_pixelformat[] = {
	{0x098C, 0x2755, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x2757, I2C_16BIT}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005, I2C_16BIT}, 	// MCU_DATA_0
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       

const static struct mt9d115_reg init_mirror[] = {
	{0x098C, 0x2717, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_A]
	{0x0990, 0x046D, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x272D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]
	{0x0990, 0x0025, I2C_16BIT}, 	// MCU_DATA_0	
	{0x3040, 0x046D, I2C_16BIT}, 	// READ_MODE
	{0x098C, 0xA115, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CAP_MODE]
	{0x0990, 0x0000, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0001, I2C_16BIT}, 	// MCU_DATA_0	
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};    
   
const static struct mt9d115_reg verticalflip[] = {
	{0x098C, 0x2717, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_A]
	{0x0990, 0x046E, I2C_16BIT}, 	// MCU_DATA_0
	{0x098C, 0x272D, I2C_16BIT}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]
	{0x0990, 0x0026, I2C_16BIT}, 	// MCU_DATA_0	
	{0x098C, 0xA103, I2C_16BIT}, 	// MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0006, I2C_16BIT}, 	// MCU_DATA_0	
	{I2C_REG_TERM, I2C_VAL_TERM, I2C_LEN_TERM}          
};       

#endif /* ifndef MT9D115_REGS_H */










