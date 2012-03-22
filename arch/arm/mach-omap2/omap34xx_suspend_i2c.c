/*
 * Basic I2C functions
 *
 * Copyright (c) 2004 Texas Instruments
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Jian Zhang jzhang@ti.com, Texas Instruments
 *
 * Copyright (c) 2003 Wolfgang Denk, wd@denx.de
 * Rewritten to fit into the current U-Boot framework
 *
 * Adapted for OMAP2420 I2C, r-woodruff2@ti.com
 *
 */
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/i2c/twl.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <linux/low_battery.h>


static u32 i2c_base = I2C_DEFAULT_BASE;
static u32 i2c_speed = CFG_I2C_SPEED;

#define inbl(a) __raw_readb(OMAP2_L4_IO_ADDRESS(i2c_base + (a)))
#define outbl(v,a) __raw_writeb((v), OMAP2_L4_IO_ADDRESS((i2c_base + (a))))
#define inwl(a) __raw_readw(OMAP2_L4_IO_ADDRESS(i2c_base +(a)))
#define outwl(v,a) __raw_writew((v), OMAP2_L4_IO_ADDRESS((i2c_base + (a))))


static void wait_for_bb(void);
static u16 wait_for_pin(void);
static void flush_fifo(void);


#define I2C_NUM_IF 3
void i2c_init(int speed, int slaveadd);

int select_bus(int bus, int speed)
{
	if ((bus < 0) || (bus >= I2C_NUM_IF)) {
		printk("Bad bus ID-%d\n", bus);
		return -1;
	}

//#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
#if 1
	/* Check speed */
	if ((speed != OMAP_I2C_STANDARD) && (speed != OMAP_I2C_FAST_MODE)
	    && (speed != OMAP_I2C_HIGH_SPEED)) {
		printk("Invalid Speed for i2c init-%d\n", speed);
		return -1;
	}
#else
	if ((speed != OMAP_I2C_STANDARD) && (speed != OMAP_I2C_FAST_MODE)) {
		printk("Invalid Speed for i2c init-%d\n", speed);
		return -1;
	}
#endif

	if (bus == 2)
		i2c_base = I2C_BASE3;
	else if (bus == 1)
		i2c_base = I2C_BASE2;
	else
		i2c_base = I2C_BASE1;

	i2c_init(speed, CFG_I2C_SLAVE);
	return 0;
}

void i2c_init(int speed, int slaveadd)
{
	int psc, fsscll, fssclh;
	int hsscll = 0, hssclh = 0;
	u32 scll, sclh, scl;
	int reset_timeout = 10;
	unsigned long internal_clk;

	/* Only handle standard, fast and high speeds */
	if ((speed != OMAP_I2C_STANDARD) &&
	    (speed != OMAP_I2C_FAST_MODE) &&
	    (speed != OMAP_I2C_HIGH_SPEED)) {
		printk("Error : I2C unsupported speed %d\n", speed);
		return;
	}
#if 1
	/*
	 * Set the internal sampling clock to what the
	 * board requires if it is defined.  Else use
	 * the values in the v2.6.31 kernel.
	 */

	/* standard */
	internal_clk = 4000;
	if (speed == OMAP_I2C_HIGH_SPEED)
		internal_clk = 19200;
	else if (speed == OMAP_I2C_FAST_MODE)
		internal_clk = 9600;
	else /* standard */
		internal_clk = 4000;


	psc = I2C_IP_CLK / internal_clk;
	psc -= 1;
	if (psc < I2C_PSC_MIN) {
		printk("Error : I2C unsupported prescalar %d\n", psc);
		return;
	}


	if (speed == OMAP_I2C_HIGH_SPEED) {
		/* High speed */

		/* For first phase of HS mode */
		scl = internal_clk / 400;
		fsscll = scl - (scl / 3) - I2C_HIGHSPEED_PHASE_ONE_SCLL_TRIM;
		fssclh = (scl / 3) - I2C_HIGHSPEED_PHASE_ONE_SCLH_TRIM;

		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printk("Error : I2C initializing clock\n");
			return;
		}

		/* For second phase of HS mode */
		scl = I2C_IP_CLK / speed;
		hsscll = scl - (scl / 3) - I2C_HIGHSPEED_PHASE_TWO_SCLL_TRIM;
		hssclh = (scl / 3) - I2C_HIGHSPEED_PHASE_TWO_SCLH_TRIM;

		if (((hsscll < 0) || (hssclh < 0)) ||
		    ((hsscll > 255) || (hssclh > 255))) {
			printk("Error : I2C initializing second phase clock\n");
			return;
		}

		scll = (unsigned int)hsscll << 8 | (unsigned int)fsscll;
		sclh = (unsigned int)hssclh << 8 | (unsigned int)fssclh;

	} else if (speed == OMAP_I2C_FAST_MODE) {
		/* Standard speed */
		scl = internal_clk / speed;
		fsscll = scl - (scl / 3) - I2C_FASTSPEED_SCLL_TRIM;
		fssclh = (scl / 3) - I2C_FASTSPEED_SCLH_TRIM;

		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printk("Error : I2C initializing clock\n");
			return;
		}

		scll = (unsigned int)fsscll;
		sclh = (unsigned int)fssclh;

	} else {
		/* Standard speed */
		fsscll = fssclh = internal_clk / (2 * speed);

		fsscll -= I2C_FASTSPEED_SCLL_TRIM;
		fssclh -= I2C_FASTSPEED_SCLH_TRIM;

		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printk("Error : I2C initializing clock\n");
			return;
		}

		scll = (unsigned int)fsscll;
		sclh = (unsigned int)fssclh;
	}
	
#endif
#if 0
		    gpio_direction_output(59,1);
	    	mdelay(100);
	    	gpio_direction_output(59,0);
	    	mdelay(100);
	    	gpio_direction_output(59,1);
	    	mdelay(100);
	    	gpio_direction_output(59,0);
	    	mdelay(300);
#endif
	/* Execute Soft-reset sequence for I2C controller */
	reset_timeout = 100;
	while ((inwl(I2C_CON) & I2C_CON_EN) && reset_timeout--) {

		/* Ensure that the module is disabled */
		outwl(0, I2C_CON);
	}
			#if 0
		    gpio_direction_output(59,1);
	    	mdelay(100);
	    	gpio_direction_output(59,0);
	    	mdelay(100);
	    	gpio_direction_output(59,1);
	    	mdelay(100);
	    	gpio_direction_output(59,0);
	    	mdelay(300);
		#endif
	if (reset_timeout <= 0)
		{
		printk("ERROR: Timeout to Disable the Module\n");
	}

	outwl(I2C_SYSC_SRST, I2C_SYSC);  /* Set the I2Ci.I2C_SYSC[1] SRST bit to 1 */
	mdelay(1);
	outwl(I2C_CON_EN, I2C_CON);  /* Enable the module */

	reset_timeout = 100;
	while (!(inwl(I2C_SYSS) & I2C_SYSS_RDONE) && reset_timeout--) {
		if (reset_timeout <= 0)
			printk("ERROR: Timeout while waiting for soft-reset to complete\n");
	}

	outwl(0, I2C_CON);  /* Disable I2C controller before writing
                                        to PSC and SCL registers */
	outwl(psc, I2C_PSC);
	outwl(scll, I2C_SCLL);
	outwl(sclh, I2C_SCLH);

	/* own address */
	outwl(slaveadd, I2C_OA);
	outwl(I2C_CON_EN, I2C_CON);

	/* have to enable intrrupts or OMAP i2c module doesn't work */
	outwl(I2C_IE_XRDY_IE | I2C_IE_RRDY_IE | I2C_IE_ARDY_IE |
	     I2C_IE_NACK_IE | I2C_IE_AL_IE, I2C_IE);
	//udelay(1000);
	mdelay(1);
#if 0
				mdelay(300);
			  gpio_direction_output(60,1);
	    	mdelay(100);
	    	gpio_direction_output(60,0);
	    	mdelay(100);
	    	gpio_direction_output(60,1);
	    	mdelay(100);
	    	gpio_direction_output(60,0);
#endif
	flush_fifo();
	outwl(0xFFFF, I2C_STAT);
	outwl(0, I2C_CNT);
	
}

static int i2c_read_byte(u8 devaddr, u8 regoffset, u8 * value)
{
	int err;
	int i2c_error = 0;
	u16 status;

	/* wait until bus not busy */
	wait_for_bb();

	/* one byte only */
	outwl(1, I2C_CNT);
	/* set slave address */
	outwl(devaddr, I2C_SA);
	/* no stop bit needed here */
	outwl(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX, I2C_CON);
	    

	status = wait_for_pin();

	if (status & I2C_STAT_XRDY) {
		/* Important: have to use byte access */
		outbl(regoffset, I2C_DATA);

		/* Important: wait for ARDY bit to set */
		err = 2000;
		while (!(inwl(I2C_STAT) & I2C_STAT_ARDY) && err--)
			;
		if (err <= 0)
			i2c_error = 1;

		if (inwl(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}

	if (!i2c_error) {
		err = 2000;
		outwl(I2C_CON_EN, I2C_CON);
		while (inwl(I2C_STAT) || (inwl(I2C_CON) & I2C_CON_MST)) {
			/* Have to clear pending interrupt to clear I2C_STAT */
			outwl(0xFFFF, I2C_STAT);
			if (!err--) {
				break;
			}
		}

		/* set slave address */
		outwl(devaddr, I2C_SA);
		/* read one byte from slave */
		outwl(1, I2C_CNT);
		/* need stop bit here */
		outwl(I2C_CON_EN |
		     ((i2c_speed ==
		       OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) | I2C_CON_MST |
		     I2C_CON_STT | I2C_CON_STP, I2C_CON);

		status = wait_for_pin();
		if (status & I2C_STAT_RRDY) {
#if 1
			*value = inbl(I2C_DATA);
#else
			*value = inwl(I2C_DATA);
#endif
		/* Important: wait for ARDY bit to set */
		err = 20000;
		while (!(inwl(I2C_STAT) & I2C_STAT_ARDY) && err--)
			;
		if (err <= 0){
printk("i2c_read_byte -- I2C_STAT_ARDY error\n");
			i2c_error = 1;
		}
		} else {
			i2c_error = 1;
		}

		if (!i2c_error) {
			int err = 1000;
			outwl(I2C_CON_EN, I2C_CON);
			while (inwl(I2C_STAT)
			       || (inwl(I2C_CON) & I2C_CON_MST)) {
				outwl(0xFFFF, I2C_STAT);
				if (!err--) {
					break;
				}
			}
		}
	}
	flush_fifo();
	outwl(0xFFFF, I2C_STAT);
	outwl(0, I2C_CNT);
	return i2c_error;
}

static int i2c_write_byte(u8 devaddr, u8 regoffset, u8 value)
{
	int eout;
	int i2c_error = 0;
	u16 status, stat;

	/* wait until bus not busy */
	wait_for_bb();

	/* two bytes */
	outwl(2, I2C_CNT);
	/* set slave address */
	outwl(devaddr, I2C_SA);
	/* stop bit needed here */
	outwl(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX | I2C_CON_STP, I2C_CON);

	/* wait until state change */
	status = wait_for_pin();

	if (status & I2C_STAT_XRDY) {
#if 1
		/* send out 1 byte */
		outbl(regoffset, I2C_DATA);
		outwl(I2C_STAT_XRDY, I2C_STAT);
		status = wait_for_pin();
		if ((status & I2C_STAT_XRDY)) {
			/* send out next 1 byte */
			outbl(value, I2C_DATA);
			outwl(I2C_STAT_XRDY, I2C_STAT);
		} else {
			i2c_error = 1;
		}
#else
		/* send out 2 bytes */
		outwl((value << 8) | regoffset, I2C_DATA);
#endif
		/* must have enough delay to allow BB bit to go low */
		eout= 20000;
		while (!(inwl(I2C_STAT) & I2C_STAT_ARDY) && eout--)
			;
		if (eout <= 0)
			printk("timed out in i2c_write_byte: I2C_STAT=%x\n",
			       inwl(I2C_STAT));

		if (inwl(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}
	if (!i2c_error) {
		eout = 2000;

		outwl(I2C_CON_EN, I2C_CON);
		while ((stat = inwl(I2C_STAT)) || (inwl(I2C_CON) & I2C_CON_MST)) {
			/* have to read to clear intrrupt */
			outwl(0xFFFF, I2C_STAT);
			if (--eout == 0)	/* better leave with error than hang */
				break;
		}
	}
	flush_fifo();
	outwl(0xFFFF, I2C_STAT);
	outwl(0, I2C_CNT);
	return i2c_error;
}

static void flush_fifo(void)
{
	u16 stat;

	/* note: if you try and read data when its not there or ready
	 * you get a bus error
	 */
	while (1) {
		stat = inwl(I2C_STAT);
		if (stat == I2C_STAT_RRDY) {
#if 1
			inbl(I2C_DATA);
#else
			inwl(I2C_DATA);
#endif
			outwl(I2C_STAT_RRDY, I2C_STAT);
		} else
			break;
	}
}

int i2c_probe(char chip)
{
	int res = 1;		/* default = fail */

	if (chip == inwl(I2C_OA)) {
		return res;
	}

	/* wait until bus not busy */
	wait_for_bb();

	/* try to read one byte */
	outwl(1, I2C_CNT);
	/* set slave address */
	outwl(chip, I2C_SA);
	/* stop bit needed here */
	outwl(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_STP, I2C_CON);
	/* enough delay for the NACK bit set */
	//udelay(50000);
		mdelay(50);

	if (!(inwl(I2C_STAT) & I2C_STAT_NACK)) {
		res = 0;	/* success case */
		flush_fifo();
		outwl(0xFFFF, I2C_STAT);
	} else {
		outwl(0xFFFF, I2C_STAT);	/* failue, clear sources */
		outwl(inwl(I2C_CON) | I2C_CON_STP, I2C_CON);	/* finish up xfer */
	//	udelay(20000);
		mdelay(20);
		wait_for_bb();
	}
	flush_fifo();
	outwl(0, I2C_CNT);	/* don't allow any more data in...we don't want it. */
	outwl(0xFFFF, I2C_STAT);
	return res;
}

int i2c_read(char chip, uint addr, int alen, char * buffer, int len)
{
	int i;

	if (alen > 1) {
		printk("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printk("I2C read: address out of range\n");
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (i2c_read_byte(chip, addr + i, &buffer[i])) {
			printk("I2C read: I/O error\n");
			i2c_init(i2c_speed, CFG_I2C_SLAVE);
			return 1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(i2c_read);

int i2c_write(char chip, uint addr, int alen, char * buffer, int len)
{
	int i;

	if (alen > 1) {
		printk("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printk("I2C read: address out of range\n");
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (i2c_write_byte(chip, addr + i, buffer[i])) {
			printk("I2C read: I/O error\n");
			i2c_init(i2c_speed, CFG_I2C_SLAVE);
			return 1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(i2c_write);

static void wait_for_bb(void)
{
	int timeout = 5000;
	u16 stat;

	outwl(0xFFFF, I2C_STAT);	/* clear current interruts... */
	while ((stat = inwl(I2C_STAT) & I2C_STAT_BB) && timeout--) {
		outwl(stat, I2C_STAT);
	}

	if (timeout <= 0) {
		printk("timed out in wait_for_bb: I2C_STAT=%x\n",
		       inwl(I2C_STAT));
	}
	outwl(0xFFFF, I2C_STAT);	/* clear delayed stuff */
}

static u16 wait_for_pin(void)
{
	u16 status;
	int timeout = 9000;

	do {
		status = inwl(I2C_STAT);
	} while (!(status &
		   (I2C_STAT_ROVR | I2C_STAT_XUDF | I2C_STAT_XRDY |
		    I2C_STAT_RRDY | I2C_STAT_ARDY | I2C_STAT_NACK |
		    I2C_STAT_AL)) && timeout--);

	if (timeout <= 0) {
		printk("timed out in wait_for_pin: I2C_STAT=%x\n",
		       inwl(I2C_STAT));
		outwl(0xFFFF, I2C_STAT);
	}
	return status;
}

