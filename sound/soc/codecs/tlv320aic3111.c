/*
 * linux/sound/soc/codecs/tlv320aic3111.c
 *
 *
 * Copyright (C) 2009 Texas Instruments, Inc.
 *
 * Based on sound/soc/codecs/wm8753.c by Liam Girdwood
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * History:
 *
 * Rev 0.1   ASoC driver support   AIC3111         27-11-2009
 *   
 *			 The AIC3252 ASoC driver is ported for the codec AIC3111.
 *     
 *
 * 
 * 
 */

/***************************** INCLUDES ************************************/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/rtc.h>
#include <linux/workqueue.h>


#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "tlv320aic3111.h"
#include <linux/gpio.h>

#include <linux/i2c/twl.h>

#include <linux/switch.h>
/*
#ifdef CONFIG_MINI_DSP
#include "tlv320aic3111_mini-dsp.c"
#endif
*/
//&*&*&*BC1_110722: fix the pop noise when system shutdown
#include <linux/reboot.h>
//&*&*&*BC2_110722: fix the pop noise when system shutdown

//#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define DBG(x...) printk(KERN_ALERT x)
#else
#define DBG(x...)
#endif

/*
 ***************************************************************************** 
 * Macros
 ***************************************************************************** 
 */
 #define AIC_FORCE_SWITCHES_ON 


#ifdef CONFIG_MINI_DSP
extern int aic3111_minidsp_program(struct snd_soc_codec *codec);
extern void aic3111_add_minidsp_controls(struct snd_soc_codec *codec);
#endif

#define SOC_SINGLE_AIC3111(xname) \
{\
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = __new_control_info, .get = __new_control_get,\
	.put = __new_control_put, \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
}



#define SOC_DOUBLE_R_AIC3111(xname, reg_left, reg_right, shift, mask, invert) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.info = snd_soc_info_volsw_2r_aic3111, \
	.get = snd_soc_get_volsw_2r_aic3111, .put = snd_soc_put_volsw_2r_aic3111, \
	.private_value = (reg_left) | ((shift) << 8)  | \
		((mask) << 12) | ((invert) << 20) | ((reg_right) << 24) }


#define    AUDIO_CODEC_HPH_DETECT_GPIO (99) //20110404, JimmySu change GPIO from 156 to 99
//#define    AUDIO_CODEC_PWR_ON_GPIO 	(15) // 20101112, JimmySu, change GPIO from 158 to 15
//&*&*&BC1_101001:implement headphone driver control
#if defined (CONFIG_CL1_DVT_BOARD)

#define    AUDIO_CODEC_HEADSET_DRIVER_GPIO 	(11)
#else
#define    AUDIO_CODEC_HEADSET_DRIVER_GPIO 	(96)
#endif
#define    AUDIO_CODEC_RESET_GPIO (103) /*<--LH_SWRD_CL1_Gavin@20110418,Add for Audio reset-->*/
//&*&*&BC2_101001:implement headphone driver control	
/*<--LH_SWRD_CL1_Gavin@20110910,Add for 3G Board*/
#if defined(CONFIG_CL1_3G_BOARD )
#define    AUDIO_CODEC_SPK_AMP_GPIO 	(5)
#else
#define    AUDIO_CODEC_SPK_AMP_GPIO 	(22)
#endif
/*LH_SWRD_CL1_Gavin@20110910,Add for 3G Board-->*/
#define    AUDIO_CODEC_PWR_ON_GPIO_NAME   "audio_codec_pwron"
#define    AUDIO_CODEC_RESET_GPIO_NAME    "audio_codec_reset"

//&*&*&BC1_101001:implement headphone driver control
#define    AUDIO_CODEC_HEADSET_DRIVER_GPIO_NAME 	"audio_headset_pwron"
//&*&*&BC2_101001:implement headphone driver control

#define    AUDIO_CODEC_SPK_AMP_GPIO_NAME 	"audio_spk_pwron"

/*
 ***************************************************************************** 
 * Function Prototype
 ***************************************************************************** 
 */
static int aic3111_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params, struct snd_soc_dai *dai);

static int aic3111_mute(struct snd_soc_dai *dai, int mute);

static int aic3111_pcm_trigger(struct snd_pcm_substream *substream,
			      int cmd, struct snd_soc_dai *codec_dai);

static int aic3111_pcm_shutdown(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *codec_dai);

static int aic3111_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir);

static int aic3111_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt);

static int aic3111_set_bias_level(struct snd_soc_codec *codec, enum snd_soc_bias_level level);

static unsigned int aic3111_read(struct snd_soc_codec *codec, unsigned int reg);

static int __new_control_info(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_info *uinfo);

static int __new_control_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol);

static int __new_control_put(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol);

static int snd_soc_info_volsw_2r_aic3111(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_info *uinfo);

static int snd_soc_get_volsw_2r_aic3111(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol);

static int snd_soc_put_volsw_2r_aic3111(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol);

static int aic3111_headset_detect(struct snd_soc_codec *codec);
static irqreturn_t aic3111_irq_handler(int irq, void *par);

static int aic3111_headset_driver_switch(int on);

static void aic3111_capture_io_control(struct snd_soc_codec *codec, int on);

static void aic3111_playback_io_control(struct snd_soc_codec *codec, int on);
//&*&*&*BC1_110407:implement fm routeing
static int fm_get_route (struct snd_kcontrol *kcontrol,	struct snd_ctl_elem_value *ucontrol);

static int fm_set_route (struct snd_kcontrol *kcontrol,	struct snd_ctl_elem_value *ucontrol);	


int aic3111_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value);
//&*&*&*BC2_110407:implement fm routeing

//&*&*&*BC1_110721: adjust the microphone gain for different input source
static int get_input_source (struct snd_kcontrol *kcontrol,	struct snd_ctl_elem_value *ucontrol);

static int set_input_source (struct snd_kcontrol *kcontrol,	struct snd_ctl_elem_value *ucontrol);	
//&*&*&*BC2_110721: adjust the microphone gain for different input source

static void aic3111_fm_path(struct snd_soc_codec *codec);
static void AudioPathSwitch( struct snd_soc_codec *codec );
static void MicPathSwitch( struct snd_soc_codec *codec );
/*
 ***************************************************************************** 
 * Global Variable
 ***************************************************************************** 
 */
 
static u8 aic3111_reg_ctl;

/* whenever aplay/arecord is run, aic3111_hw_params() function gets called. 
 * This function reprograms the clock dividers etc. this flag can be used to 
 * disable this when the clock dividers are programmed by pps config file
 */
static int soc_static_freq_config = 1;

static int audio_playback_status = 0;
static int audio_headset_plugin = 0;
static int audio_record_status = 0;
//&*&*&*BC1_110725: fix the issue that the device has no sounds for some applications when the device resume		
static int audio_playbackio_status = 0;
//&*&*&*BC2_110725: fix the issue that the device has no sounds for some applications when the device resume		
//&*&*&*BC1_110407:implement fm routeing
static int fm_route_path = 0;
//&*&*&*BC2_110407:implement fm routeing
/* <-- LH_SWRD_CL1_Henry@2011.7.24 fix bug 27825: Plug in/out earphone in sleeping process leads to playing video abnormally*/	
static int hph_detect_irq = 0;
/* LH_SWRD_CL1_Henry@2011.7.24 fix bug 27825: Plug in/out earphone in sleeping process leads to playing video abnormally  -->*/	
/*
 ***************************************************************************** 
 * Structure Declaration
 ***************************************************************************** 
 */
static struct switch_dev sdev;

enum {
	NO_DEVICE	= 0,
	FOX_HEADSET	= 1,
};

/*
 ***************************************************************************** 
 * Structure Initialization
 ***************************************************************************** 
 */
static const struct snd_kcontrol_new aic3111_snd_controls[] = {
//&*&*&*BC1_110407:implement fm routeing
#if 1
	/* Output */
	/* sound new kcontrol for PCM Playback volume control */
	SOC_DOUBLE_R_AIC3111("PCM Playback Volume", LDAC_VOL, RDAC_VOL, 0, 0xAf,
			     0),
	/* sound new kcontrol for HP driver gain */
	SOC_DOUBLE_R_AIC3111("HP Driver Gain", HPL_DRIVER, HPR_DRIVER, 0, 0x23, 0),
	/* sound new kcontrol for LO driver gain */
	SOC_DOUBLE_R_AIC3111("LO Driver Gain", SPL_DRIVER, SPR_DRIVER, 0, 0x23, 0),
	/* sound new kcontrol for HP mute */
	SOC_DOUBLE_R("HP DAC Playback Switch", HPL_DRIVER, HPR_DRIVER, 2,
		     0x01, 1),
	/* sound new kcontrol for LO mute */
	SOC_DOUBLE_R("LO DAC Playback Switch", SPL_DRIVER, SPR_DRIVER, 2,
		     0x01, 1),

	/* Input */
	/* sound new kcontrol for PGA capture volume */
	SOC_DOUBLE_R_AIC3111("PGA Capture Volume", ADC_VOL, ADC_VOL, 0, 0x3F,
			     0),

	/* BTodorov: MICBIAS output power ctrl: OFF(0x0), 2V, 2.5V, AVDD(0x3) */
	SOC_DOUBLE_R("MIC BIAS", MICBIAS_CTRL, MICBIAS_CTRL, 0, 0x3, 0),

	/* sound new kcontrol for Programming the registers from user space */
	SOC_SINGLE_AIC3111("Program Registers"),
	
#endif

	SOC_SINGLE_EXT("FM Route Set", 0x0, 0, 7, 0, fm_get_route, fm_set_route),
//&*&*&*BC2_110407:implement fm routeing
//&*&*&*BC1_110721: adjust the microphone gain for different input source
	SOC_SINGLE_EXT("Set input Source", 0x0, 0, 7, 0, get_input_source, set_input_source),
//&*&*&*BC1_110721: adjust the microphone gain for different input source

};


/* the sturcture contains the different values for mclk */
static const struct aic3111_rate_divs aic3111_divs[] = {
/* 
 * mclk, rate, p_val, pll_j, pll_d, dosr, ndac, mdac, aosr, nadc, madc, blck_N, 
 * codec_speficic_initializations 
 */
	/* 8k rate */
	// DDenchev (MMS)
	{12000000, 8000, 1, 7, 1680, 128, 2, 42, 128, 2, 42, 4,
	 {{DAC_INSTRUCTION_SET, 7}, {61, 4}}},
	{13000000, 8000, 1, 6, 3803, 128, 3, 27, 128, 3, 27, 4,
	 {{DAC_INSTRUCTION_SET, 7}, {61, 4}}},
	{24000000, 8000, 2, 7, 6800, 768, 15, 1, 64, 45, 4, 24,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 1}}},

	/* 11.025k rate */
	// DDenchev (MMS)
	{12000000, 11025, 1, 7, 560, 128, 5, 12, 128, 5, 12, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{13000000, 11025, 1, 6, 1876, 128, 3, 19, 128, 3, 19, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{24000000, 11025, 2, 7, 5264, 512, 16, 1, 64, 32, 4, 16,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 1}}},

	/* 12k rate */
	// DDenchev (MMS)
	{12000000, 12000, 1, 7, 1680, 128, 2, 28, 128, 2, 28, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{13000000, 12000, 1, 6, 3803, 128, 3, 18, 128, 3, 18, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},

	/* 16k rate */
	// DDenchev (MMS)
	{12000000, 16000, 1, 7, 1680, 128, 2, 21, 128, 2, 21, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{13000000, 16000, 1, 6, 6166, 128, 3, 14, 128, 3, 14, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{24000000, 16000, 2, 7, 6800, 384, 15, 1, 64, 18, 5, 12,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 1}}},

	/* 22.05k rate */
	// DDenchev (MMS)
	{12000000, 22050, 1, 7, 560, 128, 5, 6, 128, 5, 6, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{13000000, 22050, 1, 6, 5132, 128, 3, 10, 128, 3, 10, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{24000000, 22050, 2, 7, 5264, 256, 16, 1, 64, 16, 4, 8,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 1}}},

	/* 24k rate */
	// DDenchev (MMS)
	{12000000, 24000, 1, 7, 1680, 128, 2, 14, 128, 2, 14, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{13000000, 24000, 1, 6, 3803, 128, 3, 9, 128, 3, 9, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},

	/* 32k rate */
	// DDenchev (MMS)
	{12000000, 32000, 1, 6, 1440, 128, 2, 9, 128, 2, 9, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{13000000, 32000, 1, 6, 6166, 128, 3, 7, 128, 3, 7, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{24000000, 32000, 2, 7, 1680, 192, 7, 2, 64, 7, 6, 6,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 1}}},

	/* 44.1k rate */
	// DDenchev (MMS)
	{12000000, 44100, 1, 7, 560, 128, 5, 3, 128, 5, 3, 4,
	 {{DAC_INSTRUCTION_SET, 7}, {61, 4}}},
	{13000000, 44100, 1, 6, 5132, 128, 3, 5, 128, 3, 5, 4,
	 {{DAC_INSTRUCTION_SET, 2}, {61, 4}}},
	{24000000, 44100, 2, 7, 5264, 128, 8, 2, 64, 8, 4, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},

	/* 48k rate */
	// DDenchev (MMS)
	{12000000, 48000, 1, 7, 1680, 128, 2, 7, 128, 2, 7, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{13000000, 48000, 1, 6, 6166, 128, 7, 2, 128, 7, 2, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 4}}},
	{24000000, 48000, 2, 8, 1920, 128, 8, 2, 64, 8, 4, 4,
	 {{DAC_INSTRUCTION_SET, 1}, {61, 1}}},

	/*96k rate : GT 21/12/2009: NOT MODIFIED */
	{12000000, 96000, 1, 8, 1920, 64, 2, 8, 64, 2, 8, 2,
	 {{DAC_INSTRUCTION_SET, 7}, {61, 10}}},
	{13000000, 96000, 1, 6, 6166, 64, 7, 2, 64, 7, 2, 2,
	 {{DAC_INSTRUCTION_SET, 7}, {61, 10}}},
	{24000000, 96000, 2, 8, 1920, 64, 4, 4, 64, 8, 2, 2,
	 {{DAC_INSTRUCTION_SET, 7}, {61, 7}}},

	/*192k : GT 21/12/2009: NOT MODIFIED */
	{12000000, 192000, 1, 8, 1920, 32, 2, 8, 32, 2, 8, 1,
	 {{DAC_INSTRUCTION_SET, 17}, {61, 13}}},
	{13000000, 192000, 1, 6, 6166, 32, 7, 2, 32, 7, 2, 1,
	 {{DAC_INSTRUCTION_SET, 17}, {61, 13}}},
	{24000000, 192000, 2, 8, 1920, 32, 4, 4, 32, 4, 4, 1,
	 {{DAC_INSTRUCTION_SET, 17}, {61, 16}}},
};

/*
 *----------------------------------------------------------------------------
 * @struct  snd_soc_codec_dai |
 *          It is SoC Codec DAI structure which has DAI capabilities viz., 
 *          playback and capture, DAI runtime information viz. state of DAI 
 *			and pop wait state, and DAI private data. 
 *          The AIC3111 rates ranges from 8k to 192k
 *          The PCM bit format supported are 16, 20, 24 and 32 bits
 *----------------------------------------------------------------------------
 */

static struct snd_soc_dai_ops twl4030_dai_voice_ops = {
	.hw_params = aic3111_hw_params,
//	.digital_mute = aic3111_mute,
	.set_sysclk = aic3111_set_dai_sysclk,
	.set_fmt = aic3111_set_dai_fmt,
	.trigger = aic3111_pcm_trigger,
	.shutdown = aic3111_pcm_shutdown,
};


static struct  snd_soc_dai_driver tlv320aic3111_dai = {
	.name = "TLV320AIC3111-hifi",
	.playback = {
		     .stream_name = "HiFi Playback",
		     .channels_min = 2,
		     .channels_max = 2,
		     .rates = AIC3111_RATES,
		     .formats = AIC3111_FORMATS,},
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 2,
		    .channels_max = 2,
		    .rates = AIC3111_RATES,
		    .formats = AIC3111_FORMATS,},

	.ops = &twl4030_dai_voice_ops,
};

EXPORT_SYMBOL_GPL(tlv320aic3111_dai);

#ifdef DEBUG
void debug_print_registers (struct snd_soc_codec *codec)
{
/*
	int i;
	u32 data;

	printk(KERN_ALERT "PAGE 0");
	for (i = 0 ; i < 90 ; i++) {
		data = aic3111_read(codec, i);
		printk(KERN_ALERT "reg = %d val = %x\n", i, data);
	}
	printk(KERN_ALERT "PAGE 1");
	for (i = 128+0 ; i < 128+80 ; i++) {
		data = aic3111_read(codec, i);
		printk(KERN_ALERT "reg = %d val = %x\n", i, data);
	}
*/
}
#endif //DEBUG

/*
 ***************************************************************************** 
 * Initializations
 ***************************************************************************** 
 */
/*
 * AIC3111 register cache
 * We are caching the registers here.
 * There is no point in caching the reset register.
 * NOTE: In AIC32, there are 127 registers supported in both page0 and page1
 *       The following table contains the page0 and page 1registers values.
 */
static const u8 aic3111_reg[AIC3111_CACHEREGNUM] = {
	0x00, 0x00, 0x00, 0x02,	/* 0 */
	0x00, 0x11, 0x04, 0x00,	/* 4 */
	0x00, 0x00, 0x00, 0x01,	/* 8 */
	0x01, 0x00, 0x80, 0x80,	/* 12 */
	0x08, 0x00, 0x01, 0x01,	/* 16 */
	0x80, 0x80, 0x04, 0x00,	/* 20 */
	0x00, 0x00, 0x01, 0x00,	/* 24 */
	0x00, 0x00, 0x01, 0x00,	/* 28 */
	0x00, 0x00, 0x00, 0x00,	/* 32 */
	0x00, 0x00, 0x00, 0x00,	/* 36 */
	0x00, 0x00, 0x00, 0x00,	/* 40 */
	0x00, 0x00, 0x00, 0x00,	/* 44 */
	0x00, 0x00, 0x00, 0x00,	/* 48 */
	0x00, 0x02, 0x02, 0x00,	/* 52 */
	0x00, 0x00, 0x00, 0x00,	/* 56 */
	0x01, 0x04, 0x00, 0x14,	/* 60 */
	0x0C, 0x00, 0x00, 0x00,	/* 64 */
	0x0F, 0x38, 0x00, 0x00,	/* 68 */
	0x00, 0x00, 0x00, 0xEE,	/* 72 */
	0x10, 0xD8, 0x7E, 0xE3,	/* 76 */
	0x00, 0x00, 0x80, 0x00,	/* 80 */
	0x00, 0x00, 0x00, 0x00,	/* 84 */
	0x7F, 0x00, 0x00, 0x00,	/* 88 */
	0x00, 0x00, 0x00, 0x00,	/* 92 */
	0x00, 0x00, 0x00, 0x00,	/* 96 */
	0x00, 0x00, 0x00, 0x00,	/* 100 */
	0x00, 0x00, 0x00, 0x00,	/* 104 */
	0x00, 0x00, 0x00, 0x00,	/* 108 */
	0x00, 0x00, 0x00, 0x00,	/* 112 */
	0x00, 0x00, 0x00, 0x00,	/* 116 */
	0x00, 0x00, 0x00, 0x00,	/* 120 */
	0x00, 0x00, 0x00, 0x00,	/* 124 - PAGE0 Registers(127) ends here */
	0x00, 0x00, 0x00, 0x00,	/* 128, PAGE1-0 */
	0x00, 0x00, 0x00, 0x00,	/* 132, PAGE1-4 */
	0x00, 0x00, 0x00, 0x00,	/* 136, PAGE1-8 */
	0x00, 0x00, 0x00, 0x00,	/* 140, PAGE1-12 */
	0x00, 0x00, 0x00, 0x00,	/* 144, PAGE1-16 */
	0x00, 0x00, 0x00, 0x00,	/* 148, PAGE1-20 */
	0x00, 0x00, 0x00, 0x00,	/* 152, PAGE1-24 */
	0x00, 0x00, 0x00, 0x04,	/* 156, PAGE1-28 */
	0x06, 0x3E, 0x00, 0x00,	/* 160, PAGE1-32 */
	0x7F, 0x7F, 0x7F, 0x7F,	/* 164, PAGE1-36 */
	0x02, 0x02, 0x00, 0x00,	/* 168, PAGE1-40 */
	0x00, 0x00, 0x00, 0x80,	/* 172, PAGE1-44 */
	0x00, 0x00, 0x00, 0x00,	/* 176, PAGE1-48 */
	0x00, 0x00, 0x00, 0x00,	/* 180, PAGE1-52 */
	0x00, 0x00, 0x00, 0x80,	/* 184, PAGE1-56 */
	0x80, 0x00, 0x00, 0x00,	/* 188, PAGE1-60 */
	0x00, 0x00, 0x00, 0x00,	/* 192, PAGE1-64 */
	0x00, 0x00, 0x00, 0x00,	/* 196, PAGE1-68 */
	0x00, 0x00, 0x00, 0x00,	/* 200, PAGE1-72 */
	0x00, 0x00, 0x00, 0x00,	/* 204, PAGE1-76 */
	0x00, 0x00, 0x00, 0x00,	/* 208, PAGE1-80 */
	0x00, 0x00, 0x00, 0x00,	/* 212, PAGE1-84 */
	0x00, 0x00, 0x00, 0x00,	/* 216, PAGE1-88 */
	0x00, 0x00, 0x00, 0x00,	/* 220, PAGE1-92 */
	0x00, 0x00, 0x00, 0x00,	/* 224, PAGE1-96 */
	0x00, 0x00, 0x00, 0x00,	/* 228, PAGE1-100 */
	0x00, 0x00, 0x00, 0x00,	/* 232, PAGE1-104 */
	0x00, 0x00, 0x00, 0x00,	/* 236, PAGE1-108 */
	0x00, 0x00, 0x00, 0x00,	/* 240, PAGE1-112 */
	0x00, 0x00, 0x00, 0x00,	/* 244, PAGE1-116 */
	0x00, 0x00, 0x00, 0x00,	/* 248, PAGE1-120 */
	0x00, 0x00, 0x00, 0x00	/* 252, PAGE1-124 */
};

/* 
 * aic3111 initialization data 
 * This structure initialization contains the initialization required for
 * AIC3111.
 * These registers values (reg_val) are written into the respective AIC3111 
 * register offset (reg_offset) to  initialize AIC3111. 
 * These values are used in aic3111_init() function only. 
 */
#if 1
static const struct aic3111_configs aic3111_reg_init[] = {
  /* Carry out the software reset */
  {RESET, 0x01},
  /* Connect MIC1_L and MIC1_R to CM */
  {MICPGA_CM, 0xE0},//0xC0
  /* PLL is CODEC_CLKIN */
//  {CLK_REG_1, MCLK_2_CODEC_CLKIN}, //if pb set DIN
  {CLK_REG_1, PLLCLK_2_CODEC_CLKIN},
  /* DAC_MOD_CLK is BCLK source */
  {AIS_REG_3, DAC_MOD_CLK_2_BDIV_CLKIN},
  /* Setting up DAC Channel */
//  {DAC_CHN_REG, LDAC_2_LCHN | RDAC_2_RCHN | SOFT_STEP_2WCLK},
//  {DAC_CHN_REG, 0xD5},
  {DAC_CHN_REG, 0xD6}, //0x16
  /* Headphone powerup */
  {HP_OUT_DRIVERS, 0x3D},  // reset value
  /* DAC_L and DAC_R Output Mixer Routing */
  {DAC_MIXER_ROUTING, 0x44},  //DAC_X is routed to the channel mixer amplifier
  /* HPL unmute and gain 0db */
  {HPL_DRIVER, 0x6},
  /* HPR unmute and gain 0db */
  {HPR_DRIVER, 0x6},//e
  /* Headphone drivers */
  {HPHONE_DRIVERS, 0xC4}, //0x44
  /* HPL unmute and gain 0db */
  {LEFT_ANALOG_HPL, 0x8A}, //0
  /* HPR unmute and gain 0db */
  {RIGHT_ANALOG_HPR, 0x8A}, //0
  {LEFT_ANALOG_SPL, 0x8B}, //en plus 0x80 - 0x92
  {RIGHT_ANALOG_SPR, 0x8B},//en plus 0x80 - 0x92
  /* LOL unmute and gain 0db */
  {SPL_DRIVER, 0x0C}, //x04
  /* LOR unmute and gain 0db */
  {SPR_DRIVER, 0x0C}, //0x4
  /* Unmute DAC Left and Right channels */
  {DAC_MUTE_CTRL_REG, 0x0C},
   /*MIC Switch Detect*/
  //{HP_MIC_DECT,0x86},
  /* MICBIAS control */
  {MICBIAS_CTRL, 0x09}, //0x0A
  /* mic PGA unmuted */
  {MICPGA_VOL_CTRL, 0x20},//0x76
  /* IN1_L is selected for left P */
  {MICPGA_PIN_CFG,0x04},//0x40
  /* IN1_R is selected for right P */
  {MICPGA_MIN_CFG, 0x40},
  /* ADC volume control change by 2 gain step per ADC Word Clock */
  {ADC_REG_1,0x80}, //0ix82
  {LDAC_VOL, 0x1F},///*<--LH_RDSW@gavin_0903,debug for change volume of plugin/out headphone*/
  {RDAC_VOL, 0x1F},///*<--LH_RDSW@gavin_0903,debug for change volume of plugin/out headphone*/
  /* Unmute ADC left and right channels */
  {ADC_FGA, 0x20},//0x00
  {ADC_VOL, 0x7C},//0x60
  {CHN_AGC_1,00},
  /* DAC power on */
  {CLASS_D_SPK, 0x06},
  /* select PRB_P2 for DRC */
   {ADC_PRO_BLOCKS_SELCT, 0x00},
  {DAC_INSTRUCTION_SET, 0x02},
  /* disable left and right DRC */
  {DRC_CTRL1, 0x01},
  /* set hold time disable */
  {DRC_CTRL2, 0x00},
  /* set attack time and decay time */
  {DRC_CTRL3, 0x55},
  /*Timer Clock MCLK Driver8*/
  {TIMER_CLOCK_MCLK_DRIVER,0x81},
};
#else

//config  de gilles.
static const struct aic3111_configs aic3111_reg_init[] = {
	/* Carry out the software reset */
	{RESET, 0x01},
	/* PLL is CODEC_CLKIN */
	{CLK_REG_1, PLLCLK_2_CODEC_CLKIN},
	/* DAC_MOD_CLK is BCLK source */
//	{AIS_REG_3, DAC_MOD_CLK_2_BDIV_CLKIN},
	/* Setting up DAC Channel */
	{DAC_CHN_REG,
	 LDAC_2_LCHN | RDAC_2_RCHN },
	/* Headphone powerup */
	{HP_OUT_DRIVERS, 0xc2},  // reset value
	/* DAC_L and DAC_R Output Mixer Routing */ 
	{DAC_MIXER_ROUTING, 0x44},  //DAC_X is routed to the channel mixer amplifier
	/* HPL unmute and gain 0db */
	{HPL_DRIVER, 0xe},
	/* HPR unmute and gain 0db */
	{HPR_DRIVER, 0xe},
	/* HPL unmute and gain 0db */
	{LEFT_ANALOG_HPL, 0},
	/* HPR unmute and gain 0db */
	{RIGHT_ANALOG_HPR, 0},
//@@GT: taken from vincent's config[START]
	{LEFT_ANALOG_SPL, 0x80}, //en plus 0x80
	{RIGHT_ANALOG_SPR, 0x80},//en plus 0x80
//@@GT: taken from vincent's config[END]
	/* LOL unmute and gain 12db */
	{SPL_DRIVER, 0x0c},
	/* LOR unmute and gain 12db */
	{SPR_DRIVER, 0x0c},
//@@GT: taken from vincent's config[START]
	/* Unmute DAC Left and Right channels */
	{DAC_MUTE_CTRL_REG, 0x0C},
//@@GT: taken from vincent's config[END]
	/* MICBIAS control */
	{MICBIAS_CTRL, 0x0b},
	/* MICBIAS control */
	{MICPGA_VOL_CTRL, 0x40},
	/* IN1_L is selected for left P */
	{MICPGA_PIN_CFG, 0x40},
	/* IN1_R is selected for right P */
	{MICPGA_MIN_CFG, 0x00},
	/* ADC volume control change by 2 gain step per ADC Word Clock */
//	{ADC_REG_1, 0x80},
	/* Unmute ADC left and right channels */
//	{ADC_FGA, 0x00},
	{ CLASS_D_SPK, 0xC6 }, // forced for now
};
#endif

/* Left DAC_L Mixer */
static const struct snd_kcontrol_new hpl_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC switch", DAC_MIXER_ROUTING, 6, 2, 1),
	SOC_DAPM_SINGLE("MIC1_L switch", DAC_MIXER_ROUTING, 5, 1, 0),
//	SOC_DAPM_SINGLE("Left_Bypass switch", HPL_ROUTE_CTL, 1, 1, 0),
};

/* Right DAC_R Mixer */
static const struct snd_kcontrol_new hpr_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_DAC switch", DAC_MIXER_ROUTING, 2, 2, 1),
	SOC_DAPM_SINGLE("MIC1_R switch", DAC_MIXER_ROUTING, 1, 1, 0),
//	SOC_DAPM_SINGLE("Right_Bypass switch", HPR_ROUTE_CTL, 1, 1, 0),
};

static const struct snd_kcontrol_new lol_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC switch", DAC_MIXER_ROUTING, 6, 2, 1),    //SOC_DAPM_SINGLE("L_DAC switch", LOL_ROUTE_CTL, 3, 1, 0),
	SOC_DAPM_SINGLE("MIC1_L switch", DAC_MIXER_ROUTING, 5, 1, 0),
//	SOC_DAPM_SINGLE("Left_Bypass switch", HPL_ROUTE_CTL, 1, 1, 0),
};

static const struct snd_kcontrol_new lor_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_DAC switch", DAC_MIXER_ROUTING, 2, 2, 1),    //SOC_DAPM_SINGLE("R_DAC switch", LOR_ROUTE_CTL, 3, 1, 0),
	SOC_DAPM_SINGLE("MIC1_R switch", DAC_MIXER_ROUTING, 1, 1, 0),
//	SOC_DAPM_SINGLE("Right_Bypass switch", LOR_ROUTE_CTL, 1, 1, 0),
};

/* Right DAC_R Mixer */
static const struct snd_kcontrol_new left_input_mixer_controls[] = {
	SOC_DAPM_SINGLE("MIC1_L switch", MICPGA_PIN_CFG, 6, 1, 0),
	SOC_DAPM_SINGLE("MIC1_M switch", MICPGA_PIN_CFG, 2, 1, 0),
};

static const struct snd_kcontrol_new right_input_mixer_controls[] = {
	SOC_DAPM_SINGLE("MIC1_R switch", MICPGA_PIN_CFG, 4, 1, 0),
	SOC_DAPM_SINGLE("MIC1_M switch", MICPGA_PIN_CFG, 2, 1, 0),
};

static const struct snd_soc_dapm_widget aic3111_dapm_widgets[] = {
	/* Left DAC to Left Outputs */
	/* dapm widget (stream domain) for left DAC */
	SND_SOC_DAPM_DAC("Left DAC", "Left Playback", DAC_CHN_REG, 7, 1),
	/* dapm widget (path domain) for left DAC_L Mixer */

	SND_SOC_DAPM_MIXER("HPL Output Mixer", SND_SOC_NOPM, 0, 0,
			   &hpl_output_mixer_controls[0],
			   ARRAY_SIZE(hpl_output_mixer_controls)),
	SND_SOC_DAPM_PGA("HPL Power", HPHONE_DRIVERS, 7, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("LOL Output Mixer", SND_SOC_NOPM, 0, 0,
			   &lol_output_mixer_controls[0],
			   ARRAY_SIZE(lol_output_mixer_controls)),
	SND_SOC_DAPM_PGA("LOL Power", CLASS_D_SPK, 7, 1, NULL, 0),

	/* Right DAC to Right Outputs */
	/* dapm widget (stream domain) for right DAC */
	SND_SOC_DAPM_DAC("Right DAC", "Right Playback", DAC_CHN_REG, 6, 1),
	/* dapm widget (path domain) for right DAC_R mixer */
	SND_SOC_DAPM_MIXER("HPR Output Mixer", SND_SOC_NOPM, 0, 0,
			   &hpr_output_mixer_controls[0],
			   ARRAY_SIZE(hpr_output_mixer_controls)),
	SND_SOC_DAPM_PGA("HPR Power", HPHONE_DRIVERS, 6, 0, NULL, 0), 

	SND_SOC_DAPM_MIXER("LOR Output Mixer", SND_SOC_NOPM, 0, 0,
			   &lor_output_mixer_controls[0],
			   ARRAY_SIZE(lor_output_mixer_controls)),
	SND_SOC_DAPM_PGA("LOR Power", CLASS_D_SPK, 6, 1, NULL, 0),

	SND_SOC_DAPM_MIXER("Left Input Mixer", SND_SOC_NOPM, 0, 0,
			   &left_input_mixer_controls[0],
			   ARRAY_SIZE(left_input_mixer_controls)),

	SND_SOC_DAPM_MIXER("Right Input Mixer", SND_SOC_NOPM, 0, 0,
			   &right_input_mixer_controls[0],
			   ARRAY_SIZE(right_input_mixer_controls)),

	/* Inputs to Left ADC */
	SND_SOC_DAPM_ADC("ADC", "Capture", ADC_REG_1, 7, 0),

	/* Right Inputs to Right ADC */
	SND_SOC_DAPM_ADC("Right ADC", "Right Capture", ADC_REG_1, 6, 0),

	/* dapm widget (platform domain) name for HPLOUT */
	SND_SOC_DAPM_OUTPUT("HPL"),
	/* dapm widget (platform domain) name for HPROUT */
	SND_SOC_DAPM_OUTPUT("HPR"),
	/* dapm widget (platform domain) name for LOLOUT */
	SND_SOC_DAPM_OUTPUT("LOL"),
	/* dapm widget (platform domain) name for LOROUT */
	SND_SOC_DAPM_OUTPUT("LOR"),

	/* dapm widget (platform domain) name for MIC1LP */
	SND_SOC_DAPM_INPUT("MIC1LP"),
	/* dapm widget (platform domain) name for MIC1RP*/
	SND_SOC_DAPM_INPUT("MIC1RP"),
	/* dapm widget (platform domain) name for MIC1LM */
	SND_SOC_DAPM_INPUT("MIC1LM"),
};


/*
* DAPM audio route definition. *
* Defines an audio route originating at source via control and finishing 
* at sink. 
*/
static const struct snd_soc_dapm_route aic3111_dapm_routes[] = {
	/* ******** Right Output ******** */
	{"HPR Output Mixer", "R_DAC switch", "Right DAC"},
	{"HPR Output Mixer",  "MIC1_R switch", "MIC1RP"},
	//{"HPR Output Mixer", "Right_Bypass switch", "Right_Bypass"},

	{"HPR Power", NULL, "HPR Output Mixer"},
	{"HPR", NULL, "HPR Power"},

	{"LOR Output Mixer", "R_DAC switch", "Right DAC"},
	{"LOR Output Mixer",  "MIC1_R switch", "MIC1RP"},
//	{"LOR Output Mixer", "Right_Bypass switch", "Right_Bypass"},

	{"LOR Power", NULL, "LOR Output Mixer"},
	{"LOR", NULL, "LOR Power"},
	
	/* ******** Left Output ******** */
	{"HPL Output Mixer", "L_DAC switch", "Left DAC"},
	{"HPL Output Mixer", "MIC1_L switch", "MIC1LP"},
	//{"HPL Output Mixer", "Left_Bypass switch", "Left_Bypass"},

	{"HPL Power", NULL, "HPL Output Mixer"},
	{"HPL", NULL, "HPL Power"},

	{"LOL Output Mixer", "L_DAC switch", "Left DAC"},
	{"LOL Output Mixer", "MIC1_L switch", "MIC1LP"},
//	{"LOL Output Mixer", "Left_Bypass switch", "Left_Bypass"},

	{"LOL Power", NULL, "LOL Output Mixer"},
	{"LOL", NULL, "LOL Power"},


	/* ******** Left input ******** */
	{"Left Input Mixer", "MIC1_L switch", "MIC1LP"},
	//{"Left_Bypass", NULL, "Left Input Mixer"},

	//{"Left ADC", NULL, "Left Input Mixer"},
        {"ADC", NULL, "Left Input Mixer"},

	/* ******** Right Input ******** */
	{"Right Input Mixer", "MIC1_R switch", "MIC1RP"},
//	{"Right_Bypass", NULL, "Right Input Mixer"},

	//{"Right ADC", NULL, "Right Input Mixer"},
	{"ADC", NULL, "Right Input Mixer"},

	/* ******** terminator ******** */
	//{NULL, NULL, NULL},
};

#define AIC3111_DAPM_ROUTE_NUM (sizeof(aic3111_dapm_routes)/sizeof(struct snd_soc_dapm_route))

static void i2c_aic3111_headset_access_work(struct work_struct *work);
static struct work_struct works;
static struct snd_soc_codec *codec_work_var_glob;

/*
 ***************************************************************************** 
 * Function Definitions
 ***************************************************************************** 
 */

//&*&*&*BC1_110722: fix the pop noise when system shutdown
static int aic3111_prepare_for_shutdown(struct notifier_block *this,
		unsigned long cmd, void *p)
{

	fm_route_path = 0;
	aic3111_playback_io_control(codec_work_var_glob, 0);
	
	return NOTIFY_DONE;
}

static struct notifier_block aic3111_shutdown_notifier = {
		.notifier_call = aic3111_prepare_for_shutdown,
		.next = NULL,
		.priority = 0
};
//&*&*&*BC2_110722: fix the pop noise when system shutdown

//&*&*&*BC1_110407:implement fm routeing
static int fm_get_route (struct snd_kcontrol *kcontrol,	struct snd_ctl_elem_value *ucontrol){
	ucontrol->value.integer.value[0] = fm_route_path;
	return 0;
}

static int fm_set_route (struct snd_kcontrol *kcontrol,	struct snd_ctl_elem_value *ucontrol){

	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	fm_route_path = (ucontrol->value.integer.value[0]);
	printk("fm_set_route = %d\n", fm_route_path);

	aic3111_fm_path(codec);
	
	return 0;
}
//&*&*&*BC2_110407:implement fm routeing		

//&*&*&*BC1_110721: adjust the microphone gain for different input source
static int get_input_source (struct snd_kcontrol *kcontrol,	struct snd_ctl_elem_value *ucontrol){
	ucontrol->value.integer.value[0] = 0;
	return 0;
}

static int set_input_source (struct snd_kcontrol *kcontrol,	struct snd_ctl_elem_value *ucontrol){

	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int inputSource = 0;
	inputSource = (ucontrol->value.integer.value[0]);
	printk("set_input_source = %d\n", inputSource);

	if(inputSource == 6)
	aic3111_write(codec, MICPGA_VOL_CTRL, 0x32);
	else
	aic3111_write(codec, MICPGA_VOL_CTRL, 0x5a);
	
	return 0;
}
//&*&*&*BC2_110721: adjust the microphone gain for different input source

static ssize_t headset_h2w_print_name(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(sdev)) {
	case NO_DEVICE:
		return sprintf(buf, "No Device\n");
	case FOX_HEADSET:
		return sprintf(buf, "Headset\n");
	}
	return -EINVAL;
}


static int snd_soc_info_volsw_2r_aic3111(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_info *uinfo)
{
    int mask = (kcontrol->private_value >> 12) & 0xff;

	DBG("snd_soc_info_volsw_2r_aic3111 (%s)\n", kcontrol->id.name);

    uinfo->type =
        mask == 1 ? SNDRV_CTL_ELEM_TYPE_BOOLEAN : SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 2;
    uinfo->value.integer.min = 0;
    uinfo->value.integer.max = mask;
    return 0;
}
/*
 *----------------------------------------------------------------------------
 * Function : snd_soc_get_volsw_2r_aic3111
 * Purpose  : Callback to get the value of a double mixer control that spans
 *            two registers.
 *
 *----------------------------------------------------------------------------
 */
int snd_soc_get_volsw_2r_aic3111(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & AIC3111_8BITS_MASK;
	int reg2 = (kcontrol->private_value >> 24) & AIC3111_8BITS_MASK;
	int mask;
	int shift;
	unsigned short val, val2;

	DBG("snd_soc_get_volsw_2r_aic3111 %s\n", kcontrol->id.name);

	if (!strcmp(kcontrol->id.name, "PCM Playback Volume")) {
		mask = AIC3111_8BITS_MASK;
		shift = 0;
	} else if (!strcmp(kcontrol->id.name, "HP Driver Gain")) {
		mask = 0xF;
		shift = 3;
	} else if (!strcmp(kcontrol->id.name, "LO Driver Gain")) {
		mask = 0x3;
		shift = 3;
	} else if (!strcmp(kcontrol->id.name, "PGA Capture Volume")) {
		mask = 0x7F;
		shift = 0;
	} else {
		printk(KERN_ALERT "Invalid kcontrol name\n");
		return -1;
	}

	val = (snd_soc_read(codec, reg) >> shift) & mask;
	val2 = (snd_soc_read(codec, reg2) >> shift) & mask;

	if (!strcmp(kcontrol->id.name, "PCM Playback Volume")) {
		ucontrol->value.integer.value[0] =
		    (val <= 48) ? (val + 127) : (val - 129);
		ucontrol->value.integer.value[1] =
		    (val2 <= 48) ? (val2 + 127) : (val2 - 129);
	} else if (!strcmp(kcontrol->id.name, "HP Driver Gain")) {
		ucontrol->value.integer.value[0] =
		    (val <= 9) ? (val + 0) : (val - 15);
		ucontrol->value.integer.value[1] =
		    (val2 <= 9) ? (val2 + 0) : (val2 - 15);
	} else if (!strcmp(kcontrol->id.name, "LO Driver Gain")) {
		ucontrol->value.integer.value[0] =
		    ((val/6) <= 4) ? ((val/6 -1)*6) : ((val/6 - 0)*6);
		ucontrol->value.integer.value[1] =
		    ((val2/6) <= 4) ? ((val2/6-1)*6) : ((val2/6 - 0)*6);
	} else if (!strcmp(kcontrol->id.name, "PGA Capture Volume")) {
		ucontrol->value.integer.value[0] =
		    ((val*2) <= 40) ? ((val*2 + 24)/2) : ((val*2 - 254)/2);
		ucontrol->value.integer.value[1] =
		    ((val2*2) <= 40) ? ((val2*2 + 24)/2) : ((val2*2 - 254)/2);
	}
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : snd_soc_put_volsw_2r_aic3111
 * Purpose  : Callback to set the value of a double mixer control that spans
 *            two registers.
 *
 *----------------------------------------------------------------------------
 */
int snd_soc_put_volsw_2r_aic3111(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & AIC3111_8BITS_MASK;
	int reg2 = (kcontrol->private_value >> 24) & AIC3111_8BITS_MASK;
	int err;
	unsigned short val, val2, val_mask;

	DBG("snd_soc_put_volsw_2r_aic3111 (%s)\n", kcontrol->id.name);

	val = ucontrol->value.integer.value[0];
	val2 = ucontrol->value.integer.value[1];

	if (!strcmp(kcontrol->id.name, "PCM Playback Volume")) {
		val = (val >= 127) ? (val - 127) : (val + 129);
//		 (val <= 48) ? (val + 127) : (val - 129);
		val2 = (val2 >= 127) ? (val2 - 127) : (val2 + 129);
		val_mask = AIC3111_8BITS_MASK;	/* 8 bits */
	} else if (!strcmp(kcontrol->id.name, "HP Driver Gain")) {
		val = (val >= 0) ? (val - 0) : (val + 15);
		val2 = (val2 >= 0) ? (val2 - 0) : (val2 + 15);
		val_mask = 0xF;	/* 4 bits */
	} else if (!strcmp(kcontrol->id.name, "LO Driver Gain")) {
		val = (val/6 >= 1) ? ((val/6 +1)*6) : ((val/6 + 0)*6);
		val2 = (val2/6 >= 1) ? ((val2/6 +1)*6) : ((val2/6 + 0)*6);
		val_mask = 0x3;	/* 2 bits */
	} else if (!strcmp(kcontrol->id.name, "PGA Capture Volume")) {
		val = (val*2 >= 24) ? ((val*2 - 24)/2) : ((val*2 + 254)/2);
		val2 = (val2*2 >= 24) ? ((val2*2 - 24)/2) : ((val2*2 + 254)/2);
		val_mask = 0x7F;	/* 7 bits */
	} else {
		printk(KERN_ALERT "Invalid control name\n");
		return -1;
	}

	if ((err = snd_soc_update_bits(codec, reg, val_mask, val)) < 0) {
		printk(KERN_ALERT "Error while updating bits\n");
		return err;
	}

	err = snd_soc_update_bits(codec, reg2, val_mask, val2);
	return err;
}

/*
 *----------------------------------------------------------------------------
 * Function : __new_control_info
 * Purpose  : This function is to initialize data for new control required to 
 *            program the AIC3111 registers.
 *            
 *----------------------------------------------------------------------------
 */
static int __new_control_info(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_info *uinfo)
{

	DBG("+ new control info\n");

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 65535;

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : __new_control_get
 * Purpose  : This function is to read data of new control for 
 *            program the AIC3111 registers.
 *            
 *----------------------------------------------------------------------------
 */
static int __new_control_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u32 val;

	DBG("+ new control get (%d)\n", aic3111_reg_ctl);

	val = aic3111_read(codec, aic3111_reg_ctl);
	ucontrol->value.integer.value[0] = val;

	DBG("+ new control get val(%d)\n", val);

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : __new_control_put
 * Purpose  : new_control_put is called to pass data from user/application to
 *            the driver.
 * 
 *----------------------------------------------------------------------------
 */
static int __new_control_put(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct aic3111_priv *aic3111 = snd_soc_codec_get_drvdata(codec);

	u32 data_from_user = ucontrol->value.integer.value[0];
	u8 data[2];

	DBG("+ new control put (%s)\n", kcontrol->id.name);

	DBG("reg = %d val = %x\n", data[0], data[1]);

	aic3111_reg_ctl = data[0] = (u8) ((data_from_user & 0xFF00) >> 8);
	data[1] = (u8) ((data_from_user & 0x00FF));

	if (!data[0]) {
		aic3111->page_no = data[1];
	}

	DBG("reg = %d val = %x\n", data[0], data[1]);

	if (codec->hw_write(codec->control_data, data, 2) != 2) {
		printk(KERN_ALERT "Error in i2c write\n");
		return -EIO;
	}
	DBG("- new control put\n");

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_change_page
 * Purpose  : This function is to switch between page 0 and page 1.
 *            
 *----------------------------------------------------------------------------
 */
int aic3111_change_page(struct snd_soc_codec *codec, u8 new_page)
{
	struct aic3111_priv *aic3111 = snd_soc_codec_get_drvdata(codec);
	u8 data[2];

	data[0] = 0;
	data[1] = new_page;
	aic3111->page_no = new_page;
	DBG("aic3111_change_page => %d (w 30 %02x %02x)\n", new_page, data[0], data[1]);

//	DBG("w 30 %02x %02x\n", data[0], data[1]);

	if (codec->hw_write(codec->control_data, data, 2) != 2) {
		printk(KERN_ALERT "Error in changing page to %d\n", new_page);
		return -1;
	}
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_write_reg_cache
 * Purpose  : This function is to write aic3111 register cache
 *            
 *----------------------------------------------------------------------------
 */
static inline void aic3111_write_reg_cache(struct snd_soc_codec *codec,
					   u16 reg, u8 value)
{
	u8 *cache = codec->reg_cache;

	if (reg >= AIC3111_CACHEREGNUM) {
		return;
	}
	cache[reg] = value;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_write
 * Purpose  : This function is to write to the aic3111 register space.
 *            
 *----------------------------------------------------------------------------
 */
int aic3111_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	struct aic3111_priv *aic3111 = snd_soc_codec_get_drvdata(codec);
	u8 data[2];
	u8 page;

	page = reg / 128;
	data[AIC3111_REG_OFFSET_INDEX] = reg % 128;
//	DBG("# aic3111 write reg(%d) new_page(%d) old_page(%d) value(0x%02x)\n", reg, page, aic3111->page_no, value);


	if (aic3111->page_no != page) {
		aic3111_change_page(codec, page);
	}
	
	DBG("w 30 %02x %02x\n", data[AIC3111_REG_OFFSET_INDEX], value);
	

	/* data is
	 *   D15..D8 aic3111 register offset
	 *   D7...D0 register data
	 */
	data[AIC3111_REG_DATA_INDEX] = value & AIC3111_8BITS_MASK;
#if defined(EN_REG_CACHE)
	if ((page == 0) || (page == 1)) {
		aic3111_write_reg_cache(codec, reg, value);
	}
#endif
	if (!data[AIC3111_REG_OFFSET_INDEX]) {
		/* if the write is to reg0 update aic3111->page_no */
		aic3111->page_no = value;
	}

	if (codec->hw_write(codec->control_data, data, 2) != 2) {
		printk(KERN_ALERT "Error in i2c write\n");
		return -EIO;
	}
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_read
 * Purpose  : This function is to read the aic3111 register space.
 *            
 *----------------------------------------------------------------------------
 */
static unsigned int aic3111_read(struct snd_soc_codec *codec, unsigned int reg)
{
	struct aic3111_priv *aic3111 = snd_soc_codec_get_drvdata(codec);
	u8 value;
	u8 page = reg / 128;


	reg = reg % 128;

	DBG("r 30 %02x\n", reg);
	
	if (aic3111->page_no != page) {
		aic3111_change_page(codec, page);
	}

	i2c_master_send(codec->control_data, (char *)&reg, 1);
	i2c_master_recv(codec->control_data, &value, 1);
	return value;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_get_divs
 * Purpose  : This function is to get required divisor from the "aic3111_divs"
 *            table.
 *            
 *----------------------------------------------------------------------------
 */
static inline int aic3111_get_divs(int mclk, int rate)
{
	int i;

	DBG("+ aic3111_get_divs mclk(%d) rate(%d)\n", mclk, rate);

	for (i = 0; i < ARRAY_SIZE(aic3111_divs); i++) {
		if ((aic3111_divs[i].rate == rate)
		    && (aic3111_divs[i].mclk == mclk)) {
	DBG("%d %d %d %d %d %d %d %d %d %d\n",
	aic3111_divs[i].p_val,
	aic3111_divs[i].pll_j,
	aic3111_divs[i].pll_d,
	aic3111_divs[i].dosr,
	aic3111_divs[i].ndac,
	aic3111_divs[i].mdac,
	aic3111_divs[i].aosr,
	aic3111_divs[i].nadc,
	aic3111_divs[i].madc,
	aic3111_divs[i].blck_N);

			return i;
		}
	}
	printk(KERN_ALERT "Master clock and sample rate is not supported\n");
	return -EINVAL;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_add_widgets
 * Purpose  : This function is to add the dapm widgets 
 *            The following are the main widgets supported
 *                # Left DAC to Left Outputs
 *                # Right DAC to Right Outputs
 *		  # Left Inputs to Left ADC
 *		  # Right Inputs to Right ADC
 *
 *----------------------------------------------------------------------------
 */
static int aic3111_add_widgets(struct snd_soc_codec *codec)
{
	int i;

	DBG("+ aic3111_add_widgets num_widgets(%d) num_routes(%d)\n",
			ARRAY_SIZE(aic3111_dapm_widgets), AIC3111_DAPM_ROUTE_NUM);
	for (i = 0; i < ARRAY_SIZE(aic3111_dapm_widgets); i++) {
		snd_soc_dapm_new_control(codec->dapm, &aic3111_dapm_widgets[i]);
	}

	DBG("snd_soc_dapm_add_routes\n");

	/* set up audio path interconnects */
	snd_soc_dapm_add_routes(codec->dapm, &aic3111_dapm_routes[0],
				AIC3111_DAPM_ROUTE_NUM);

	DBG("- aic3111_add_widgets\n");
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_hw_params
 * Purpose  : This function is to set the hardware parameters for AIC3111.
 *            The functions set the sample rate and audio serial data word 
 *            length.
 *            
 *----------------------------------------------------------------------------
 */
static int aic3111_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct aic3111_priv *aic3111 = snd_soc_codec_get_drvdata(codec);
	
	int i, j;
	//u8 data,dac_reg;
	u8 data;
	DBG("+ SET aic3111_hw_params\n");
	DBG("aic3111_hw_params: substream->stream = %d\n" , substream->stream);
	
	if(audio_playback_status || audio_record_status)
	{
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		aic3111_capture_io_control(codec, 1);
		else if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		aic3111_playback_io_control(codec, 1);
		
		return 0;
	}	
		

	aic3111_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	i = aic3111_get_divs(aic3111->sysclk, params_rate(params));
	DBG("- Sampling rate: %d, %d\n", params_rate(params), i);


	if (i < 0) {
		printk(KERN_ALERT "sampling rate not supported\n");
		return i;
	}

	if (soc_static_freq_config) {

		/* We will fix R value to 1 and will make P & J=K.D as varialble */

		/* Setting P & R values */
		aic3111_write(codec, CLK_REG_2,
			      ((aic3111_divs[i].p_val << 4) | 0x01));

		/* J value */
		aic3111_write(codec, CLK_REG_3, aic3111_divs[i].pll_j);

		/* MSB & LSB for D value */
		aic3111_write(codec, CLK_REG_4, (aic3111_divs[i].pll_d >> 8));
		aic3111_write(codec, CLK_REG_5,
			      (aic3111_divs[i].pll_d & AIC3111_8BITS_MASK));

		/* NDAC divider value */
		aic3111_write(codec, NDAC_CLK_REG_6, aic3111_divs[i].ndac);

		/* MDAC divider value */
		aic3111_write(codec, MDAC_CLK_REG_7, aic3111_divs[i].mdac);

		/* DOSR MSB & LSB values */
		aic3111_write(codec, DAC_OSR_MSB, aic3111_divs[i].dosr >> 8);
		aic3111_write(codec, DAC_OSR_LSB,
			      aic3111_divs[i].dosr & AIC3111_8BITS_MASK);

		/* NADC divider value */
		aic3111_write(codec, NADC_CLK_REG_8, aic3111_divs[i].nadc);

		/* MADC divider value */
		aic3111_write(codec, MADC_CLK_REG_9, aic3111_divs[i].madc);

		/* AOSR value */
		aic3111_write(codec, ADC_OSR_REG, aic3111_divs[i].aosr);
	}
	/* BCLK N divider */
	aic3111_write(codec, CLK_REG_11, aic3111_divs[i].blck_N);

	aic3111_set_bias_level(codec, SND_SOC_BIAS_ON);

	data = aic3111_read(codec, INTERFACE_SET_REG_1);

	data = data & ~(3 << 4);

	DBG( "- Data length: %d\n", params_format(params));

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		data |= (AIC3111_WORD_LEN_20BITS << DAC_OSR_MSB_SHIFT);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		data |= (AIC3111_WORD_LEN_24BITS << DAC_OSR_MSB_SHIFT);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		data |= (AIC3111_WORD_LEN_32BITS << DAC_OSR_MSB_SHIFT);
		break;
	}

	aic3111_write(codec, INTERFACE_SET_REG_1, data);


	for (j = 0; j < NO_FEATURE_REGS; j++) {
		aic3111_write(codec,
			      aic3111_divs[i].codec_specific_regs[j].reg_offset,
			      aic3111_divs[i].codec_specific_regs[j].reg_val);
	}
//&*&*&*BC1_110704: fix pop noise issue
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
	aic3111_capture_io_control(codec, 1);
	else if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	aic3111_playback_io_control(codec, 1);
//&*&*&*BC2_110704: fix pop noise issue
	DBG("- SET aic3111_hw_params\n");

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_mute
 * Purpose  : This function is to mute or unmute the left and right DAC
 *            
 *----------------------------------------------------------------------------
 */
static int aic3111_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 dac_reg;

	DBG("+ aic3111_mute %d\n", mute);
/*
	if(mute)
	{

		dac_reg = aic3111_read(codec, DAC_MUTE_CTRL_REG) & ~MUTE_ON;
		aic3111_write(codec, DAC_MUTE_CTRL_REG, dac_reg | MUTE_ON);
//&*&*&*BC1_110407:implement fm routeing
		if(fm_route_path == 0)
		{
			aic3111_write(codec, HPHONE_DRIVERS ,0x4); // OFF
			aic3111_write(codec, CLASS_D_SPK, 0x06); // OFF
		}
//&*&*&*BC2_110407:implement fm routeing		
		aic3111_write(codec, DAC_CHN_REG, 0x16);

	}
	else
	{

		aic3111_write(codec, DAC_CHN_REG, 0xD6);

		mdelay(5);

		dac_reg = aic3111_read(codec, DAC_MUTE_CTRL_REG) & ~MUTE_ON;
		aic3111_write(codec, DAC_MUTE_CTRL_REG, dac_reg);
		
//&*&*&*BC1_110407:implement fm routeing		
		if(audio_headset_plugin && (fm_route_path != 2))
		{
			aic3111_write(codec, HPHONE_DRIVERS, 0xC4); // ON
			aic3111_write(codec, CLASS_D_SPK, 0x06); // OFF
		}
		else
		{
			aic3111_write(codec, HPHONE_DRIVERS ,0x4); // OFF
			aic3111_write(codec, CLASS_D_SPK ,0xC6 ); //ON
		}

//&*&*&*BC2_110407:implement fm routeing
	}
*/
	
/*
	dac_reg = aic3111_read(codec, DAC_MUTE_CTRL_REG) & ~MUTE_ON;
	if (mute)
		aic3111_write(codec, DAC_MUTE_CTRL_REG, dac_reg | MUTE_ON);
	else
		aic3111_write(codec, DAC_MUTE_CTRL_REG, dac_reg);
*/


	DBG("- aic3111_mute %d\n", mute);

#ifdef DEBUG
	DBG("++ aic3111_dump\n");
	debug_print_registers (codec);
	DBG("-- aic3111_dump\n");
#endif //DEBUG

	return 0;
}


static int aic3111_pcm_trigger(struct snd_pcm_substream *substream,
			      int cmd, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	
	printk("aic3111_pcm_trigger = %d, substream->stream = %d\n", cmd , substream->stream);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) 
	{

		if((cmd == SNDRV_PCM_TRIGGER_START) || (cmd == SNDRV_PCM_TRIGGER_STOP))
		{
	
			audio_playback_status = cmd;
/*	
			if(cmd)
			{ 		
//&*&*&*BC1_110407:implement fm routeing
				if(audio_headset_plugin && (fm_route_path != 2))
				{

					aic3111_headset_driver_switch(1);
				}
				else
				{
					aic3111_headset_driver_switch(0);
				}
//&*&*&*BC2_110407:implement fm routeing		
			}
			else
			{
				
				if(fm_route_path == 0)
				aic3111_headset_driver_switch(0);
			}
*/
		}
	}
	else if(substream->stream == SNDRV_PCM_STREAM_CAPTURE)
	{
		if((cmd == SNDRV_PCM_TRIGGER_START) || (cmd == SNDRV_PCM_TRIGGER_STOP))
		{
			audio_record_status = cmd;
		
		}
	}
	return 0;
}



static int aic3111_pcm_shutdown(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	
	printk("aic3111_pcm_shutdown substream->stream = %d\n" , substream->stream);
	//if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
	//aic3111_capture_io_control(codec, 0);
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	aic3111_playback_io_control(codec, 0);
	
	return 0;
}


/*
 *----------------------------------------------------------------------------
 * Function : aic3111_set_dai_sysclk
 * Purpose  : This function is to set the DAI system clock
 *            
 *----------------------------------------------------------------------------
 */
static int aic3111_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct aic3111_priv *aic3111 = snd_soc_dai_get_drvdata(codec_dai);

	DBG("aic3111_set_dai_sysclk clk_id(%d) (%d)\n", clk_id, freq);

	switch (freq) {
	case AIC3111_FREQ_12000000:
	case AIC3111_FREQ_24000000:
	case AIC3111_FREQ_13000000:
		aic3111->sysclk = freq;
		return 0;
	}
	printk(KERN_ALERT "Invalid frequency to set DAI system clock\n");
	return -EINVAL;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_set_dai_fmt
 * Purpose  : This function is to set the DAI format
 *            
 *----------------------------------------------------------------------------
 */
static int aic3111_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct aic3111_priv *aic3111 = snd_soc_dai_get_drvdata(codec_dai);
	u8 iface_reg;

	iface_reg = aic3111_read(codec, INTERFACE_SET_REG_1);
	iface_reg = iface_reg & ~(3 << 6 | 3 << 2);

	DBG("+ aic3111_set_dai_fmt (%x) \n", fmt);

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		aic3111->master = 1;
		iface_reg |= BIT_CLK_MASTER | WORD_CLK_MASTER;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		aic3111->master = 0;
		break;
	default:
		printk(KERN_ALERT "Invalid DAI master/slave interface\n");
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface_reg |= (AIC3111_DSP_MODE << CLK_REG_3_SHIFT);
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iface_reg |= (AIC3111_RIGHT_JUSTIFIED_MODE << CLK_REG_3_SHIFT);
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface_reg |= (AIC3111_LEFT_JUSTIFIED_MODE << CLK_REG_3_SHIFT);
		break;
	default:
		printk(KERN_ALERT "Invalid DAI interface format\n");
		return -EINVAL;
	}

	DBG("- aic3111_set_dai_fmt (%x) \n", iface_reg);
	aic3111_write(codec, INTERFACE_SET_REG_1, iface_reg);
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_set_bias_level
 * Purpose  : This function is to get triggered when dapm events occurs.
 *            
 *----------------------------------------------------------------------------
 */
static int aic3111_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	struct aic3111_priv *aic3111 = snd_soc_codec_get_drvdata(codec);
	u8 value;

	DBG("++ aic3111_set_bias_level\n");

	if (level == codec->dapm->bias_level)
		return 0;

	switch (level) {
		/* full On */
	case SND_SOC_BIAS_ON:
		DBG("aic3111_set_bias_level ON\n");
		/* all power is driven by DAPM system */
		if (aic3111->master) {
			/* Switch on PLL */
			value = aic3111_read(codec, CLK_REG_2);
			aic3111_write(codec, CLK_REG_2, (value | ENABLE_PLL));

			/* Switch on NDAC Divider */
			value = aic3111_read(codec, NDAC_CLK_REG_6);
			aic3111_write(codec, NDAC_CLK_REG_6,
				      value | ENABLE_NDAC);

			/* Switch on MDAC Divider */
			value = aic3111_read(codec, MDAC_CLK_REG_7);
			aic3111_write(codec, MDAC_CLK_REG_7,
				      value | ENABLE_MDAC);

			/* Switch on NADC Divider */
			value = aic3111_read(codec, NADC_CLK_REG_8);
			aic3111_write(codec, NADC_CLK_REG_8,
				      value | ENABLE_MDAC);

			/* Switch on MADC Divider */
			value = aic3111_read(codec, MADC_CLK_REG_9);
			aic3111_write(codec, MADC_CLK_REG_9,
				      value | ENABLE_MDAC);

			/* Switch on BCLK_N Divider */
			value = aic3111_read(codec, CLK_REG_11);
			aic3111_write(codec, CLK_REG_11, value | ENABLE_BCLK);
		}
		break;

		/* partial On */
	case SND_SOC_BIAS_PREPARE:
		break;

		/* Off, with power */
	case SND_SOC_BIAS_OFF:
	case SND_SOC_BIAS_STANDBY:
		/*
		 * all power is driven by DAPM system,
		 * so output power is safe if bypass was set
		 */
		 DBG("aic3111_set_bias_level %s\n",
			(level == SND_SOC_BIAS_OFF) ? "OFF" : "STANDBY");
//&*&*&*BC1_110725: fix the issue that the device has no sounds for some applications when the device resume		
		if(level == SND_SOC_BIAS_STANDBY)		
		{
			/* Perform the Device Soft Power UP */
			value = aic3111_read(codec, MICBIAS_CTRL);
			aic3111_write(codec, MICBIAS_CTRL, (value & ~0x80));			
		}		
//&*&*&*BC2_110725: fix the issue that the device has no sounds for some applications when the device resume		
		if (aic3111->master) {
			/* Switch off PLL */
			value = aic3111_read(codec, CLK_REG_2);
			aic3111_write(codec, CLK_REG_2, (value & ~ENABLE_PLL));

			/* Switch off NDAC Divider */
			value = aic3111_read(codec, NDAC_CLK_REG_6);
			aic3111_write(codec, NDAC_CLK_REG_6,
				      value & ~ENABLE_NDAC);

			/* Switch off MDAC Divider */
			value = aic3111_read(codec, MDAC_CLK_REG_7);
			aic3111_write(codec, MDAC_CLK_REG_7,
				      value & ~ENABLE_MDAC);

			/* Switch off NADC Divider */
			value = aic3111_read(codec, NADC_CLK_REG_8);
			aic3111_write(codec, NADC_CLK_REG_8,
				      value & ~ENABLE_NDAC);

			/* Switch off MADC Divider */
			value = aic3111_read(codec, MADC_CLK_REG_9);
			aic3111_write(codec, MADC_CLK_REG_9,
				      value & ~ENABLE_MDAC);
			value = aic3111_read(codec, CLK_REG_11);

			/* Switch off BCLK_N Divider */
			aic3111_write(codec, CLK_REG_11, value & ~ENABLE_BCLK);
		}
		break;
	}
	codec->dapm->bias_level = level;
	DBG("-- aic3111_set_bias_level\n");

	return 0;
}

static void aic3111_capture_io_control(struct snd_soc_codec *codec, int on)
{
  if(on)
  { 
    aic3111_write(codec, ADC_REG_1, 0x80);
    aic3111_write(codec, ADC_FGA, 0x00);
    //aic3111_write(codec, HP_MIC_DECT, 0x86);
    aic3111_write(codec, MICBIAS_CTRL, 0x09);
    //aic3111_write(codec, MICPGA_VOL_CTRL, 0x18);
   /*<--LHRDSW@Gavin,decrease the noise for record by filter*/
    aic3111_write(codec, ADC_PRO_BLOCKS_SELCT, 0x04);
    aic3111_write(codec, MSBS_N0_ADC_IIR, 0x22);
    aic3111_write(codec, LSBS_N0_ADC_IIR, 0xB5);
    aic3111_write(codec, MSBS_N1_ADC_IIR, 0x22);
    aic3111_write(codec, LSBS_N1_ADC_IIR, 0xB5);
    aic3111_write(codec, MSBS_D1_ADC_IIR, 0x3A);
    aic3111_write(codec, LSBS_D1_ADC_IIR, 0x94);
   /*LHRDSW@Gavin,decrease the noise for record by filter-->*/

   
  }
  else
  {
    aic3111_write(codec, MICBIAS_CTRL, 0x09);
    aic3111_write(codec, ADC_REG_1, 0x00);
    aic3111_write(codec, ADC_FGA, 0x80);
  }
}

static void aic3111_playback_io_control(struct snd_soc_codec *codec, int on)
{
	u8 dac_reg;

	if(on)
	{
//&*&*&*BC1_110725: fix the issue that the device has no sounds for some applications when the device resume			
		audio_playbackio_status = 1;
		
		//aic3111_write(codec, DAC_CHN_REG, 0xD6);
//&*&*&*BC2_110725: fix the issue that the device has no sounds for some applications when the device resume				
		dac_reg = aic3111_read(codec, DAC_MUTE_CTRL_REG) & ~MUTE_ON;
		aic3111_write(codec, DAC_MUTE_CTRL_REG, dac_reg);	
		
		msleep(10);

    if(audio_headset_plugin && (fm_route_path != 2))
    {
      //aic3111_write(codec, HPHONE_DRIVERS, 0xC4); // ON
	/*<--LHSWRD@Gavin,fix bug for volume chage high when plugin/out many times*/
		aic3111_write(codec, LDAC_VOL, 0xFA);
        aic3111_write(codec, RDAC_VOL, 0xFA);
	/*LHSWRD@Gavin,fix bug for volume chage high when plugin/out many times-->*/
      	aic3111_write(codec, CLASS_D_SPK, 0x06); // OFF
		aic3111_write(codec, DAC_CHN_REG, 0xD6);
    	aic3111_headset_driver_switch(1);
    }
    else
    {
      aic3111_headset_driver_switch(0);
	  aic3111_write(codec, DAC_CHN_REG, 0xFC); 
	  aic3111_write(codec, CLASS_D_SPK ,0xC6 ); //ON 
	  //aic3111_write(codec, HPHONE_DRIVERS ,0x4); // OFF
	  
    }

	}
	else
	{
//&*&*&*BC1_110725: fix the issue that the device has no sounds for some applications when the device resume			
		audio_playbackio_status = 0;
//&*&*&*BC2_110725: fix the issue that the device has no sounds for some applications when the device resume			
		if(fm_route_path == 0)
		{
		aic3111_headset_driver_switch(0);	
		//aic3111_write(codec, HPHONE_DRIVERS ,0x4); // OFF
		aic3111_write(codec, CLASS_D_SPK, 0x06); // OFF
		msleep(10);
		}	

		dac_reg = aic3111_read(codec, DAC_MUTE_CTRL_REG) & ~MUTE_ON;
		aic3111_write(codec, DAC_MUTE_CTRL_REG, dac_reg | MUTE_ON);

		//aic3111_write(codec, DAC_CHN_REG, 0x16);
	
	}

}

void aic3111_fm_path(struct snd_soc_codec *codec)
{

	if(audio_headset_plugin)
	{

		if(fm_route_path == 0)	
			aic3111_write(codec, DAC_MIXER_ROUTING, 0x44);	
		else if(fm_route_path == 1)
		{
			aic3111_write(codec, DAC_MIXER_ROUTING, 0x66);	
			//aic3111_write(codec, HPHONE_DRIVERS, 0xC4); 
			aic3111_write(codec, CLASS_D_SPK, 0x06);
			aic3111_headset_driver_switch(1);
		}
		else if(fm_route_path == 2)
		{
			aic3111_headset_driver_switch(0);
			aic3111_write(codec, DAC_MIXER_ROUTING, 0x66);	
			//aic3111_write(codec, HPHONE_DRIVERS, 0x04);
			aic3111_write(codec, CLASS_D_SPK, 0xC6);
		}

	}
	else
	{
		aic3111_write(codec, DAC_MIXER_ROUTING, 0x44);
	}

}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_suspend
 * Purpose  : This function is to suspend the AIC3111 driver.
 *            
 *----------------------------------------------------------------------------
 */
static int aic3111_suspend(struct snd_soc_codec *codec)
{
  u8 regvalue;

  DBG("+ aic3111_suspend\n");
/* <-- LH_SWRD_CL1_Henry@2011.7.24 fix bug 27825: Plug in/out earphone in sleeping process leads to playing video abnormally*/	  
  if (hph_detect_irq > 0)
	  disable_irq(hph_detect_irq);  
/* LH_SWRD_CL1_Henry@2011.7.24 fix bug 27825: Plug in/out earphone in sleeping process leads to playing video abnormally  -->*/	
  aic3111_capture_io_control(codec, 0);
  aic3111_headset_driver_switch(0);
  aic3111_write(codec, HPHONE_DRIVERS ,0x04); // OFF
  aic3111_write(codec, CLASS_D_SPK, 0x06); // OFF
  aic3111_write(codec, DAC_CHN_REG, 0x16);
  aic3111_set_bias_level(codec, SND_SOC_BIAS_OFF);
  DBG("-aic3111_suspend\n");

  /* Perform the Device Soft Power Down */
  regvalue = aic3111_read(codec, MICBIAS_CTRL);
  aic3111_write(codec, MICBIAS_CTRL, (regvalue | 0x80));
  return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic3111_resume
 * Purpose  : This function is to resume the AIC3111 driver
 *            
 *----------------------------------------------------------------------------
 */
static int aic3111_resume(struct snd_soc_codec *codec)
{
  u8 regvalue;
  DBG("+ aic3111_resume\n");
  aic3111_change_page(codec, 1);
  /* Perform the Device Soft Power UP */
  regvalue = aic3111_read(codec, MICBIAS_CTRL);
  aic3111_write(codec, MICBIAS_CTRL, (regvalue & ~0x80));
  aic3111_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

  aic3111_write(codec, DAC_CHN_REG, 0xD6);
  aic3111_write(codec, HPHONE_DRIVERS ,0xC4); // ON
  codec_work_var_glob = codec;
/* <-- LH_SWRD_CL1_Henry@2011.7.24 fix bug 27825: Plug in/out earphone in sleeping process leads to playing video abnormally*/	  
  if (hph_detect_irq > 0)
	  enable_irq(hph_detect_irq);  
/* LH_SWRD_CL1_Henry@2011.7.24 fix bug 27825: Plug in/out earphone in sleeping process leads to playing video abnormally  -->*/	
  schedule_work(&works);

  DBG("- aic3111_resume\n");
  return 0;
}

/* <-- LH_SWRD_CL1_Gavin@2011.10.10 fix bug : when stop the music,hear some electric nosie from headphone*/	  
static ssize_t aic3111_aic_show(struct device * dev, struct device_attribute *attr, char * buf)
{
	int state;
	state = gpio_get_value(AUDIO_CODEC_HEADSET_DRIVER_GPIO);
	
	state =(state ? 1 : 0);
	return sprintf(buf, "%d", state);
}

static ssize_t aic3111_aic_store(struct device * dev, struct device_attribute *attr, char * buf, size_t count)
{
	ulong val;
	int error;
	char data[2];

	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;
	  printk("aic3111_aic_store  buf =%d\n", buf);
 
	if (val) {
		gpio_set_value(AUDIO_CODEC_HEADSET_DRIVER_GPIO, 1);
	}
	else {
		gpio_set_value(AUDIO_CODEC_HEADSET_DRIVER_GPIO, 0);
	}

	return count;
}
static DEVICE_ATTR(aic, 0666, aic3111_aic_show, aic3111_aic_store);
static struct attribute * aic3111_attributes[] = {
	&dev_attr_aic.attr,
	NULL
};

static const struct attribute_group aic3111_attr_group = {
	.attrs = aic3111_attributes,
};

/* LH_SWRD_CL1_Gavin@2011.10.10 fix bug : when stop the music,hear some electric nosie from headphone-->*/	

//&*&*&BC1_101001:implement headphone driver control
static int aic3111_headset_driver_switch(int on)
{

	int gpio = AUDIO_CODEC_HEADSET_DRIVER_GPIO;

	printk(KERN_DEBUG "aic3111_headset_driver_switch  gavin %d\n", on);
#if defined (CONFIG_CL1_DVT_BOARD)
	
	if(on)
	{
		gpio_direction_output(gpio, 0);
		
	}
	else
	{
		gpio_direction_output(gpio, 1);	

	}

#else

	if(on)
        {
                gpio_direction_output(gpio, 1);

        }
        else
        {
                gpio_direction_output(gpio, 0);

        }
#endif

}
//&*&*&BC2_101001:implement headphone driver control


static void AudioPathSwitch( struct snd_soc_codec *codec )
{	
//&*&*&*BC1_110725: fix the issue that the device has no sounds for some applications when the device resume			
	if(audio_playback_status || audio_playbackio_status)
	{
//&*&*&*BC1_110407:implement fm routeing
		if(audio_headset_plugin && (fm_route_path != 2))
		{

		//aic3111_write(codec, HPHONE_DRIVERS, 0xC4); // ON
		aic3111_write(codec, CLASS_D_SPK, 0x06); // OFF
		aic3111_headset_driver_switch(1);
		
		}
		else
		{

		//aic3111_headset_driver_switch(0);
		//aic3111_write(codec, HPHONE_DRIVERS ,0x4); // OFF
		aic3111_write(codec, CLASS_D_SPK ,0xC6 ); //ON
		
		}
//&*&*&*BC2_110407:implement fm routeing
	}		

}
/*<-- LH_SWRD_CL1_Gavin@2011.06.24 ,add for microphone Path switch about the microphone in the board and headphone*/	
static void MicPathSwitch( struct snd_soc_codec *codec )
{
  unsigned int reg=0;
 
//&*&*&*BC1_110407:implement mic routeing
    //printk(KERN_ERR "audio_headset_plugin_mic\n");
   
  //  aic3111_write(codec,PAGE_SELECT,0x00);
  //  aic3111_write(codec,HP_MIC_DECT,0x00);
	aic3111_write(codec,PAGE_SELECT,0x01);
    aic3111_write(codec,MICBIAS_CTRL,0x09);//turn on micbias only if microphone is detected
        //aic3111_write(codec,PAGE_SELECT,0x03);//page 3
        //aic3111_write(codec,TIMER_CLOCK_MCLK_DRIVER,0x01);//use internal oscillator

    if(audio_headset_plugin)
    {           
        //aic3111_write(codec,RESET,0x01);//software reset
       
        aic3111_write(codec,PAGE_SELECT,0x00);
        aic3111_write(codec,MICBIAS_CTRL,0x0A);//turn on micbias only if microphone is detected
       // aic3111_write(codec,PAGE_SELECT,0x03);//page 3
        //aic3111_write(codec,TIMER_CLOCK_MCLK_DRIVER,0x01);//use internal oscillator
        
        aic3111_write(codec,PAGE_SELECT,0x00);
        aic3111_write(codec,HP_MIC_DECT,0x00);
   
        //aic3111_write(codec,PAGE_SELECT,0x01);
        aic3111_write(codec,HP_MIC_DECT,0x92);
        msleep(50);
        //aic3111_write(codec,MICBIAS_CTRL,0x02);//turn on micbias only if microphone is detected

				reg=aic3111_read(codec,HP_MIC_DECT);
				msleep(100);
				reg=aic3111_read(codec,HP_MIC_DECT);
				msleep(100);
				printk(KERN_ERR "HP_MIC_DECT1\n");
		aic3111_write(codec,MICPGA_VOL_CTRL,0x5a);	//<--LHRDSW@gavin_0921,debug for too small gain for  headset record	
				if((reg & (1 << 5))&& (reg & (1<<6)))
					{
						
        			    audio_record_status=1;
						//aic3111_write(codec,MICPGA_VOL_CTRL,0x64);
						aic3111_write(codec,MICPGA_PIN_CFG,0x60);//OFF
						printk(KERN_ERR "Headset with microphone\n");
					}
				else
					{
						audio_record_status=0;
						aic3111_write(codec,MICPGA_PIN_CFG,0x08);	//ON
						printk(KERN_ERR "Headset without microphone\n");
					}
		//aic3111_write(codec, HPHONE_DRIVERS, 0xC4); // ON
		aic3111_write(codec, CLASS_D_SPK, 0x06); // OFF
		//aic3111_headset_driver_switch(1);
		
		}
		else
		{
		audio_record_status=0;
		//aic3111_headset_driver_switch(0);
		//aic3111_write(codec, HPHONE_DRIVERS ,0x4); // OFF
		aic3111_write(codec,MICPGA_PIN_CFG,0x08);	//ON
		aic3111_write(codec, CLASS_D_SPK ,0xC6 ); //ON
		
		}
//&*&*&*BC2_110407:implement mic routeing


}
/*LH_SWRD_CL1_Gavin@2011.06.24 ,add for microphone Path switch about the microphone in the board and headphone*-->*/	

static int aic3111_headset_detect(struct snd_soc_codec *codec)
{
  int headset_detect_gpio = AUDIO_CODEC_HPH_DETECT_GPIO;
  int headset_present = 0;
  int   iTmp = 0;
  int   iCntr = 0;

	//headset_present = gpio_get_value(headset_detect_gpio);

	/* check 500us to make sure the hotplug is stable */
	while (1) {
		msleep(10);

		iTmp = gpio_get_value(headset_detect_gpio);

		if (iTmp!=headset_present)	{
			headset_present = iTmp;
			iCntr = 0;
		}
		else {
			if (iCntr++>15) break;
		}
	}
	

  if(headset_present) {
    audio_headset_plugin = 1;
    switch_set_state(&sdev, FOX_HEADSET);
	/*<--LHRDSW@gavin,debug for change of power of headset*/
	 aic3111_write(codec, LDAC_VOL, 0xFA);
	 aic3111_write(codec, RDAC_VOL, 0xFA);
	  aic3111_write(codec, DAC_CHN_REG, 0xD6);
	  aic3111_write(codec, MINIDSP_DAC_CONTROL	 ,0x04 ); //enable adaptive mode
	   /*400Hz high pass filter*/
	  aic3111_write(codec, MSB_N0_LDAC_A ,0x7F );
	  aic3111_write(codec, LSB_N0_LDAC_A ,0xFF );
	  aic3111_write(codec, MSB_N1_LDAC_A ,0x00 );
	  aic3111_write(codec, LSB_N1_LDAC_A ,0x00  );
	  aic3111_write(codec, MSB_N2_LDAC_A ,0x00 );
	  aic3111_write(codec, LSB_N2_LDAC_A ,0x00 );
	  aic3111_write(codec, MSB_D1_LDAC_A ,0x00);
	  aic3111_write(codec, LSB_D1_LDAC_A ,0x00);
	  aic3111_write(codec, MSB_D2_LDAC_A ,0x00 );
	  aic3111_write(codec, LSB_D2_LDAC_A ,0x00 );
	  aic3111_write(codec, MINIDSP_DAC_CONTROL, 0x05);//adaptive mode and make a switch to make the filters to take effect
	  aic3111_write(codec, MSB_N0_LDAC_A ,0x7F );
	  aic3111_write(codec, LSB_N0_LDAC_A ,0xFF );
	  aic3111_write(codec, MSB_N1_LDAC_A ,0x00 );
	  aic3111_write(codec, LSB_N1_LDAC_A ,0x00  );
	  aic3111_write(codec, MSB_N2_LDAC_A ,0x00 );
	  aic3111_write(codec, LSB_N2_LDAC_A ,0x00 );
	  aic3111_write(codec, MSB_D1_LDAC_A ,0x00);
	  aic3111_write(codec, LSB_D1_LDAC_A ,0x00);
	  aic3111_write(codec, MSB_D2_LDAC_A ,0x00 );
	  aic3111_write(codec, LSB_D2_LDAC_A ,0x00 );
	  aic3111_write(codec, DRC_CTRL1, 0x01);
	//aic3111_write(codec, DAC_INSTRUCTION_SET, 0x01);
	/*LHRDSW@gavin,debug for change of power of headset-->*/
	aic3111_write(codec, MINIDSP_DAC_CONTROL, 0); // OFF
  } else {
    audio_headset_plugin = 0;
    switch_set_state(&sdev, NO_DEVICE);
	/*<--LH_RDSW@gavin_0922,debug for POP volume when plugin out headphone,before the music stop*/
	  aic3111_write(codec, LDAC_VOL, 0x81);
	  aic3111_write(codec, RDAC_VOL, 0x81);
	  msleep(250);
	 /*LH_RDSW@gavin_0922,debug for POP volume when plugin out headphone,before the music stop-->*/
	  /*<--LH_RDSW@gavin_0903,debug for change volume of plugin/out headphone*/
	  aic3111_write(codec, LDAC_VOL, 0x1F);
	  aic3111_write(codec, RDAC_VOL, 0x1F);
	/*LH_RDSW@gavin_0903,debug for change volume of plugin/out headphone-->*/
	  aic3111_write(codec, DAC_CHN_REG, 0xFC); 
	  aic3111_write(codec, MINIDSP_DAC_CONTROL	 ,0x04 ); //enable adaptive mode
	 // aic3111_write(codec, DAC_INSTRUCTION_SET, 0x02);//disable for speaker no sound if headset plugout sometime.
	  /*400Hz high pass filter*/
	  aic3111_write(codec, MSB_N0_LDAC_A ,0x7A );
	  aic3111_write(codec, LSB_N0_LDAC_A ,0xF0 );
	  aic3111_write(codec, MSB_N1_LDAC_A ,0x85 );
	  aic3111_write(codec, LSB_N1_LDAC_A ,0x10 );
	  aic3111_write(codec, MSB_N2_LDAC_A ,0x7A );
	  aic3111_write(codec, LSB_N2_LDAC_A ,0xF0 );
	  aic3111_write(codec, MSB_D1_LDAC_A ,0x7A );
	  aic3111_write(codec, LSB_D1_LDAC_A ,0xD7 );
	  aic3111_write(codec, MSB_D2_LDAC_A ,0x89 );
	  aic3111_write(codec, LSB_D2_LDAC_A ,0xEB );
	  /*DRC HPF, it must be set together with the 400Hz DAC filter*/
	  /*
	  aic3111_write(codec, MSB_N0_HDRC ,0x7E );
	  aic3111_write(codec, LSB_N0_HDRC ,0x32 );
	  aic3111_write(codec, MSB_N1_HDRC ,0x81 );
	  aic3111_write(codec, LSB_N1_HDRC ,0xCE );
	  aic3111_write(codec, MSB_D1_HDRC ,0x7C );
	  aic3111_write(codec, LSB_D1_HDRC ,0x66 );
	  */
	  /*DRC LPF, it mu be set together with the 400Hz DAC filte*/
	  aic3111_write(codec, MSB_N0_LDRC ,0x05 );
	  aic3111_write(codec, LSB_N0_LDRC ,0x3F );
	  aic3111_write(codec, MSB_N1_LDRC ,0x05 );
	  aic3111_write(codec, LSB_N1_LDRC ,0x3F );
	  aic3111_write(codec, MSB_D1_LDRC ,0x75 );
	  aic3111_write(codec, LSB_D1_LDRC ,0x7F );
	  aic3111_write(codec, MINIDSP_DAC_CONTROL	,0x05 );
	  aic3111_write(codec, MSB_N0_LDAC_A ,0x7A );
	  aic3111_write(codec, LSB_N0_LDAC_A ,0xF0 );
	  aic3111_write(codec, MSB_N1_LDAC_A ,0x85 );
	  aic3111_write(codec, LSB_N1_LDAC_A ,0x10 );
	  aic3111_write(codec, MSB_N2_LDAC_A ,0x7A );
	  aic3111_write(codec, LSB_N2_LDAC_A ,0xF0 );
	  aic3111_write(codec, MSB_D1_LDAC_A ,0x7A );
	  aic3111_write(codec, LSB_D1_LDAC_A ,0xD7 );
	  aic3111_write(codec, MSB_D2_LDAC_A ,0x89 );
	  aic3111_write(codec, LSB_D2_LDAC_A ,0xEB );
	  /*DRC HPF, it must be set together with the 400Hz DAC filter*/
	  /*
	  aic3111_write(codec, MSB_N0_HDRC ,0x7E );
	  aic3111_write(codec, LSB_N0_HDRC ,0x32 );
	  aic3111_write(codec, MSB_N1_HDRC ,0x81 );
	  aic3111_write(codec, LSB_N1_HDRC ,0xCE );
	  aic3111_write(codec, MSB_D1_HDRC ,0x7C );
	  aic3111_write(codec, LSB_D1_HDRC ,0x66 );
	  */
	  /*DRC LPF, it mu be set together with the 400Hz DAC filte*/
	  aic3111_write(codec, MSB_N0_LDRC ,0x05 );
	  aic3111_write(codec, LSB_N0_LDRC ,0x3F );
	  aic3111_write(codec, MSB_N1_LDRC ,0x05 );
	  aic3111_write(codec, LSB_N1_LDRC ,0x3F );
	  aic3111_write(codec, MSB_D1_LDRC ,0x75 );
	  aic3111_write(codec, LSB_D1_LDRC ,0x7F );
	  aic3111_write(codec, DRC_CTRL1 ,0x61 );
	  /*LHRDSW@gavin,debug for change of power of headset-->*/
	aic3111_write(codec, MINIDSP_DAC_CONTROL, 0x04); // ON
  }
  //printk(KERN_DEBUG "AudioPathSwitch\n");
  AudioPathSwitch(codec);
  aic3111_fm_path(codec);
 
  //printk(KERN_DEBUG "MicPathSwitch\n");
  MicPathSwitch(codec);
  printk("aic3111_headset_detect %d\n", headset_present);
 
  return 0;

}


/*
 * This interrupt is called when HEADSET is insert or remove from conector.
   On this interup sound will be rouote to HEadset or speaker path.
 */
static irqreturn_t aic3111_irq_handler(int irq, void *par)
{
	struct snd_soc_codec *codec = par;

	DBG("interrupt of headset found\n");
	//disable_irq(irq);
	codec_work_var_glob = codec;
	schedule_work(&works);
	//enable_irq(irq);
	return IRQ_HANDLED;
}

static void i2c_aic3111_headset_access_work(struct work_struct *work)
{
	aic3111_headset_detect(codec_work_var_glob);
}

 #define TRITON_AUDIO_IF_PAGE 	0x1
static int tlv320aic3111_init(struct snd_soc_codec *codec)
{
	struct aic3111_priv *aic3111 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;
	int i = 0;	
//	int gpio = AUDIO_CODEC_PWR_ON_GPIO;
	int hph_detect_gpio = AUDIO_CODEC_HPH_DETECT_GPIO;
/* <-- LH_SWRD_CL1_Henry@2011.7.24 fix bug 27825: Plug in/out earphone in sleeping process leads to playing video abnormally*/	  
//	int hph_detect_irq = 0;
/* LH_SWRD_CL1_Henry@2011.7.24 fix bug 27825: Plug in/out earphone in sleeping process leads to playing video abnormally  -->*/	

	printk(KERN_ALERT "+tlv320aic3111_init\n");
	
	/*
	int gpio = AUDIO_CODEC_PWR_ON_GPIO;
	ret = gpio_request(gpio, AUDIO_CODEC_PWR_ON_GPIO_NAME);
	if(ret != 0)
	printk(KERN_ERR "aic3111: gpio_request fail for gpio %d\n", gpio);	

	gpio_direction_output(gpio, 1);
	//gpio_set_value(gpio, 1);
	*/
	
	int gpio=AUDIO_CODEC_RESET_GPIO;
	ret=gpio_request(gpio,AUDIO_CODEC_RESET_GPIO_NAME);
	if(ret!=0)
	printk(KERN_ERR "aic3111: gpio_request fail for gpio %d\n", gpio);
	gpio_direction_output(gpio, 1);
	gpio_set_value(gpio, 1);
//&*&*&BC1_101001:implement headphone driver control

	gpio = AUDIO_CODEC_HEADSET_DRIVER_GPIO;
	ret = gpio_request(gpio, AUDIO_CODEC_HEADSET_DRIVER_GPIO_NAME);
	if(ret != 0)
	printk(KERN_ERR "aic3111: gpio_request fail for gpio %d\n", gpio);

	gpio_direction_output(gpio, 1);//*LH_SWRD_Gavin@0907,fix bug for headphone noise  when boot-strap
	
//&*&*&BC2_101001:implement headphone driver control
	
	gpio = AUDIO_CODEC_SPK_AMP_GPIO;
	ret = gpio_request(gpio, AUDIO_CODEC_SPK_AMP_GPIO_NAME);
	if(ret != 0)
	printk(KERN_ERR "aic3111: gpio_request fail for gpio %d\n", gpio);

	gpio_direction_output(gpio, 1);
	
	sdev.name = "h2w";
	sdev.print_name = headset_h2w_print_name;

	ret = switch_dev_register(&sdev);
	
	switch_set_state(&sdev, NO_DEVICE);

	ret = gpio_request(hph_detect_gpio, "AIC3111-headset");
	if (ret < 0) {
		goto err1;
	}
	gpio_direction_input(hph_detect_gpio);
	//omap_set_gpio_debounce(hph_detect_gpio,1);
	//omap_set_gpio_debounce_time(hph_detect_gpio,0xFF);
	hph_detect_irq = OMAP_GPIO_IRQ(hph_detect_gpio);
	
	aic3111->page_no = 0;

	printk(KERN_ALERT "*** Configuring AIC3111 registers ***\n");
	for (i = 0; i < sizeof(aic3111_reg_init) / sizeof(struct aic3111_configs); i++) {
		aic3111_write(codec, aic3111_reg_init[i].reg_offset, aic3111_reg_init[i].reg_val);
	}
	//aic3111_headset_detect(codec);
	DBG( "*** Done Configuring AIC3111 registers ***\n");
	
	ret = request_irq(hph_detect_irq, aic3111_irq_handler,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_DISABLED | IRQF_SHARED , "aic3111", codec);

	/* off, with power on */
	aic3111_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

//&*&*&*BC1_110722: fix the pop noise when system shutdown
	codec_work_var_glob = codec;
	
	ret = register_reboot_notifier(&aic3111_shutdown_notifier);
	if (ret)
		printk(KERN_ERR "AIC3111 Failed to register reboot notifier\n");
//&*&*&*BC2_110722: fix the pop noise when system shutdown


err1:
	//free_irq(hph_detect_irq, codec);

	return ret;
}


static int aic3111_probe(struct snd_soc_codec *codec)
{
	struct aic3111_priv *aic3111 = snd_soc_codec_get_drvdata(codec);

	codec->hw_write = (hw_write_t) i2c_master_send;
	codec->hw_read = NULL; //FIXME
	codec->control_data = aic3111->control_data;

	INIT_WORK(&works, i2c_aic3111_headset_access_work);
	

	tlv320aic3111_init(codec);

	snd_soc_add_controls(codec, aic3111_snd_controls,
			     ARRAY_SIZE(aic3111_snd_controls));

	//aic3111_add_widgets(codec);

#ifdef CONFIG_MINI_DSP
	/* Program MINI DSP for ADC and DAC */
	aic3111_minidsp_program(codec);
	aic3111_change_page(codec, 0x0);
#endif	

	aic3111_headset_detect(codec);

  return 0;
}



/*
 *----------------------------------------------------------------------------
 * Function : aic3111_remove
 * Purpose  : to remove aic3111 soc device 
 *            
 *----------------------------------------------------------------------------
 */

static int aic3111_remove(struct snd_soc_codec *codec)
{
	aic3111_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}


/*
 *----------------------------------------------------------------------------
 * @struct  snd_soc_codec_device |
 *          This structure is soc audio codec device sturecute which pointer
 *          to basic functions aic3111_probe(), aic3111_remove(),  
 *          aic3111_suspend() and aic3111_resume()
 *
 */
 
static struct snd_soc_codec_driver soc_codec_dev_aic3111 = {
	.read = aic3111_read,
	.write = aic3111_write,
	.reg_cache_size = sizeof(aic3111_reg),
	.reg_word_size = sizeof(u8),
	.probe = aic3111_probe,
	.remove = aic3111_remove,
	.suspend = aic3111_suspend,
	.resume = aic3111_resume,
}; 

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)

/*
 * If the i2c layer weren't so broken, we could pass this kind of data
 * around
 */
static int tlv320aic3111_i2c_probe(struct i2c_client *i2c,
			   const struct i2c_device_id *id)
{

	struct aic3111_priv *aic3111;
	int ret;

	DBG("tlv320aic3111_i2c_probe ++\n");
	//ret = sysfs_create_group(&i2c->dev.kobj, &aic3111_attr_group);// add the node for aic
	aic3111 = kzalloc(sizeof(struct aic3111_priv), GFP_KERNEL);
	if (aic3111 == NULL) {
		dev_err(&i2c->dev, "failed to create private data\n");
		return -ENOMEM;
	}

	i2c_set_clientdata(i2c, aic3111);
	aic3111->control_data = i2c;
	aic3111->control_type = SND_SOC_I2C;

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_aic3111, &tlv320aic3111_dai, 1);
	if (ret < 0)
		kfree(aic3111);
		
	DBG("tlv320aic3111_i2c_probe %d --\n", ret);
	return ret;

}


/*
 *----------------------------------------------------------------------------
 * Function : tlv320aic3007_i2c_remove
 * Purpose  : This function removes the i2c client and uninitializes 
 *                              AIC3007 CODEC.
 *            NOTE:
 *            This function is called from i2c core 
 *            If the i2c layer weren't so broken, we could pass this kind of 
 *            data around
 *            
 *----------------------------------------------------------------------------
 */

static int __exit tlv320aic3111_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	//sysfs_remove_group(&client->dev.kobj, &aic3111_attr_group); //remove the node
    return 0;
}

static const struct i2c_device_id tlv320aic3111_id[] = {
        {"tlv320aic3111", 0},
        {}
};

MODULE_DEVICE_TABLE(i2c, tlv320aic3111_id);

static struct i2c_driver tlv320aic3111_i2c_driver = {
	.driver = {
		.name = "tlv320aic3111-codec",
		.owner = THIS_MODULE,
	},
	.probe = tlv320aic3111_i2c_probe,
	.remove = __exit_p(tlv320aic3111_i2c_remove),
	.id_table = tlv320aic3111_id,
};

#endif //#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)

 
static int __init tlv320aic3111_modinit(void)
{
	int ret = 0;
	printk("tlv320aic3111_modinit ++\n");	
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	ret = i2c_add_driver(&tlv320aic3111_i2c_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register tlv320aic3111 I2C driver: %d\n",
		       ret);
	}
#endif
	printk("tlv320aic3111_modinit --\n");	
	return ret;
}

module_init(tlv320aic3111_modinit);

static void __exit tlv320aic3111_exit(void)
{
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_del_driver(&tlv320aic3111_i2c_driver);
#endif
}

module_exit(tlv320aic3111_exit);

MODULE_DESCRIPTION("ASoC TLV320AIC3111 codec driver");
MODULE_AUTHOR("sandeepsp@mistralsolutions.com");
MODULE_LICENSE("GPL");
