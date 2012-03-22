/*
 * omap3edp-aic3111.c - SoC audio for OMAP3530 EVM.
 * 
 * Copyright (C) 2009 Mistral Solutions
 *
 * Author: Sandeep S Prabhu	,sandeepsp@mistralsolutions.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Modified from n810.c  --  SoC audio for Nokia N810
 *
 * Revision History
 *
 * Inital code : May 7, 2009 :	Sandeep S Prabhu 
 * 					<sandeepsp@mistralsolutions.com>
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/soundcard.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/i2c/twl.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <plat/mux.h>
#include <mach/io.h>
#include <asm/io.h>


#include "../codecs/tlv320aic3111.h"

#include "omap-pcm.h"
#include "omap-mcbsp.h"

static struct clk *sys_clkout2;
static struct clk *clkout2_src_ck;
static struct clk *sys_ck;

/*
 * We need this superfluous clock to keep CM_SYS_CLK from being gated.
 * See table 3-48, page 343, section 3.5.3.7.1 from the OMAP36xx TRM.
 *
 * Parent must be set to cm_sys_clk so that the gating logic can work.
 */
//&*&*&*BC1_110407:disable unused GPT11 clock 
//static struct clk *gpt11_fclk;
//&*&*&*BC2_110407:disable unused GPT11 clock
#define CODEC_SYSCLK_FREQ	13000000lu


#define MCBSP1_ID	0
#define MCBSP2_ID	1
#define MCBSP3_ID	2
#define MCBSP4_ID	3


static int omap3edp_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int err;
	
	/* Set codec DAI configuration */
	err = snd_soc_dai_set_fmt(codec_dai,
					 SND_SOC_DAIFMT_I2S |
					 SND_SOC_DAIFMT_NB_NF |
					 SND_SOC_DAIFMT_CBM_CFM);

	if (err < 0)
		return err;
	
	/* Set cpu DAI configuration */
	err = snd_soc_dai_set_fmt(cpu_dai,
				       SND_SOC_DAIFMT_I2S |
				       SND_SOC_DAIFMT_NB_NF |
				       SND_SOC_DAIFMT_CBM_CFM);

	if (err < 0)
		return err;

	/* Set the codec system clock for DAC and ADC */
	err = snd_soc_dai_set_sysclk(codec_dai, 0, CODEC_SYSCLK_FREQ,
					    SND_SOC_CLOCK_IN);

	/* Use CLKX input for mcBSP2 */
	err = snd_soc_dai_set_sysclk(cpu_dai, OMAP_MCBSP_SYSCLK_CLKX_EXT,
			0, SND_SOC_CLOCK_IN);

	return err;
}

static int omap3edp_startup(struct snd_pcm_substream *substream)
{
//&*&*&*BC1_110407:disable unused GPT11 clock
	//clk_enable(gpt11_fclk);
//&*&*&*BC2_110407:disable unused GPT11 clock
	return clk_enable(sys_clkout2);
}
static void omap3edp_shutdown(struct snd_pcm_substream *substream)
{
	clk_disable(sys_clkout2);
//&*&*&*BC1_110407:disable unused GPT11 clock
	//clk_disable(gpt11_fclk);
//&*&*&*BC2_110407:disable unused GPT11 clock
}

static struct snd_soc_ops omap3edp_ops = {
	.startup = omap3edp_startup,
	.hw_params = omap3edp_hw_params,
	.shutdown = omap3edp_shutdown,
};

static const struct snd_soc_dapm_widget aic3111_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
	SND_SOC_DAPM_LINE("Ext Spk", NULL),
	SND_SOC_DAPM_MIC("Mic", NULL),
};

static int omap3edp_aic3111_init(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_dai_link omap3edp_dai = {
	.name = "TLV320AIC3111 I2S",
	.stream_name = "AIC3111 Audio",
	.codec_dai_name = "TLV320AIC3111-hifi",
	.cpu_dai_name = "omap-mcbsp-dai.1",
	.platform_name = "omap-pcm-audio",
	.init = omap3edp_aic3111_init,
	.codec_name = "tlv320aic3111-codec.2-0018",
	.ops = &omap3edp_ops,
};

static struct snd_soc_card snd_soc_card_omap3edp = {
    .name = "OMAP3 EDP",
    .dai_link = &omap3edp_dai,
    .num_links = 1,
	.long_name = "OMAP3 EDP(tlv320aic3111)",
   
    
};

static struct platform_device *omap3edp_snd_device;

static int __init omap3edp_init(void)
{
	int ret = 0;
	struct device *dev;

	printk("omap3epd-sound: Audio SoC init +\n");
	omap3edp_snd_device = platform_device_alloc("soc-audio", -1);

	if (!omap3edp_snd_device)
		return -ENOMEM;

	platform_set_drvdata(omap3edp_snd_device, &snd_soc_card_omap3edp);
	ret = platform_device_add(omap3edp_snd_device);
	if (ret)
		goto err1;
#if 1
	dev = &omap3edp_snd_device->dev;

	/* Set McBSP2 as audio McBSP */
	//*(unsigned int *)omap3edp_dai.cpu_dai->private_data = MCBSP2_ID; /* McBSP2 */

	clkout2_src_ck = clk_get(dev, "clkout2_src_ck");
	if (IS_ERR(clkout2_src_ck)) {
		printk(dev, "Could not get clkout2_src_ck\n");
		ret = PTR_ERR(clkout2_src_ck);
		goto err2;
	}

	sys_clkout2 = clk_get(dev, "sys_clkout2");
	if (IS_ERR(sys_clkout2)) {
		printk(dev, "Could not get sys_clkout2\n");
		ret = PTR_ERR(sys_clkout2);
		goto err3;
	}

	sys_ck = clk_get(dev, "sys_ck");
	if (IS_ERR(sys_ck)) {
		printk(dev, "Could not get sys_ck\n");
		ret = PTR_ERR(sys_ck);
		goto err4;
	}
//&*&*&*BC1_110407:disable unused GPT11 clock
/*
	gpt11_fclk = clk_get(dev, "gpt11_fck");
	if (IS_ERR(gpt11_fclk)) {
		dev_err(dev, "Could not get gpt11_fclk\n");
		ret = PTR_ERR(gpt11_fclk);
		goto err5;
	}
*/
//&*&*&*BC2_110407:disable unused GPT11 clock
	ret = clk_set_parent(clkout2_src_ck, sys_ck);
	if (ret) {
		printk(dev, "Could not set clkout2_src_ck's parent to sys_ck\n");
		goto err6;
	}
//&*&*&*BC1_110407:disable unused GPT11 clock
/*
	ret = clk_set_parent(gpt11_fclk, sys_ck);
	if (ret) {
		dev_err(dev, "Could not set gpt11_fclk's parent to sys_ck\n");
		goto err6;
	}
*/
//&*&*&*BC2_110407:disable unused GPT11 clock
	ret = clk_set_parent(sys_clkout2, clkout2_src_ck);
	if (ret) {
		printk(dev, "Could not set sys_clkout2's parent to clkout2_src_ck\n");
		goto err6;
	}

	ret = clk_set_rate(sys_clkout2, CODEC_SYSCLK_FREQ);
	if (ret) {
		printk(dev, "Could not set sys_clkout2 rate to %lu\n",
							CODEC_SYSCLK_FREQ);
		goto err6;
	}

	printk("sys_ck = %lu\n", clk_get_rate(sys_ck));
	printk("clkout2_src_ck = %lu\n", clk_get_rate(clkout2_src_ck));
	printk("sys_clkout2 = %lu\n", clk_get_rate(sys_clkout2));
#endif	

	printk("omap3epd-sound: Audio SoC init -\n");
	return 0;

err6:
//&*&*&*BC1_110407:disable unused GPT11 clock
//	clk_put(gpt11_fclk);
//&*&*&*BC2_110407:disable unused GPT11 clock
err5:
	clk_put(sys_ck);
err4:
	clk_put(sys_clkout2);
err3:
	clk_put(clkout2_src_ck);
err2:
	platform_device_del(omap3edp_snd_device);
	

	
err1:
	platform_device_put(omap3edp_snd_device);

	return ret;
}

static void __exit omap3edp_exit(void)
{
	clk_put(clkout2_src_ck);
	clk_put(sys_clkout2);
	clk_put(sys_ck);
//&*&*&*BC1_110407:disable unused GPT11 clock
//	clk_put(gpt11_fclk);
//&*&*&*BC2_110407:disable unused GPT11 clock
	platform_device_unregister(omap3edp_snd_device);
}

module_init(omap3edp_init);
module_exit(omap3edp_exit);

MODULE_AUTHOR("Sandeep S Prabhu<sandeepsp@mistralsolutions.com>");
MODULE_DESCRIPTION("ALSA SoC OMAP3530 EVM");
MODULE_LICENSE("GPL");
