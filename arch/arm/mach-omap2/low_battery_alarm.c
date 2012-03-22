/*
 * low battery alarm & android alarm must be judge
 *
 * Copyright (C) 2007 MontaVista Software, Inc
 * Author: Alexandre Rusev <source@mvista.com>
 *
 * Based on original TI driver twl4030-rtc.c
 *   Copyright (C) 2006 Texas Instruments, Inc.
 *
 * Based on rtc-omap.c
 *   Copyright (C) 2003 MontaVista Software, Inc.
 *   Author: George G. Davis <gdavis@mvista.com> or <source@mvista.com>
 *   Copyright (C) 2006 David Brownell
 *
 */
 
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <linux/low_battery.h>
#include <linux/i2c/twl.h>

#define ALL_TIME_REGS		6
/* RTC_INTERRUPTS_REG bitfields */
#define BIT_RTC_INTERRUPTS_REG_EVERY_M           0x03
#define BIT_RTC_INTERRUPTS_REG_IT_TIMER_M        0x04
#define BIT_RTC_INTERRUPTS_REG_IT_ALARM_M        0x08
#define BIT_RTC_CTRL_REG_GET_TIME_M              0x40

#define REG_SECONDS_REG					0x1C
#define REG_ALARM_SECONDS_REG 	0x23
#define REG_RTC_INTERRUPTS_REG 	0x2B
#define REG_RTC_CTRL_REG				0x29

/*
 * Cache the value for timer/alarm interrupts register; this is
 * only changed by callers holding rtc ops lock (or resume).
 */
static unsigned char rtc_irq_bits = 0;
unsigned long    test_rtc_current_time;

/*
 * Enable 1/second update and/or alarm interrupts.
 */
int set_irq_bit(unsigned char bit)
{
	char val;
	int ret;

	val = rtc_irq_bits | bit;
	val &= ~BIT_RTC_INTERRUPTS_REG_EVERY_M;
	ret = i2c_write(0x4b,REG_RTC_INTERRUPTS_REG,1,&val,1);
	if (ret)
		printk("rtc_read_alarm error %d\n", ret);

	return ret;
}

/*
 * Disable update and/or alarm interrupts.
 */
int mask_irq_bit(unsigned char bit)
{
	char val;
	int ret;

	val = rtc_irq_bits & ~bit;
	ret = i2c_write(0x4b,REG_RTC_INTERRUPTS_REG,1,&val,1);
	if (ret)
		printk("rtc_read_alarm error %d\n", ret);

	return ret;
}

static int twl_alarm_irq_enable(unsigned enabled)
{
	int ret;

	if (enabled)
		ret = set_irq_bit(BIT_RTC_INTERRUPTS_REG_IT_ALARM_M);
	else
		ret = mask_irq_bit(BIT_RTC_INTERRUPTS_REG_IT_ALARM_M);

	return ret;
}

/*
 * Gets current TWL RTC time and date parameters.
 *
 * The RTC's time/alarm representation is not what gmtime(3) requires
 * Linux to use:
 *
 *  - Months are 1..12 vs Linux 0-11
 *  - Years are 0..99 vs Linux 1900..N (we assume 21st century)
 */
int twl_read_rtc(struct rtc_time *tm)
{
	unsigned char rtc_data[ALL_TIME_REGS + 1];
	int ret;
	u8 save_control;

	ret = i2c_read(0x4b,REG_RTC_CTRL_REG,1,&save_control,1);
	if (ret)
		return ret;

	save_control |= BIT_RTC_CTRL_REG_GET_TIME_M;

	ret = i2c_write(0x4b,REG_RTC_CTRL_REG,1,&save_control,1);
	if (ret)
		return ret;

	ret = i2c_read(0x4b,REG_SECONDS_REG,1,rtc_data,ALL_TIME_REGS);
	if (ret ) {
		printk("rtc_read_time error %d\n", ret);
		return ret;
	}

	tm->tm_sec = bcd2bin(rtc_data[0]);
	tm->tm_min = bcd2bin(rtc_data[1]);
	tm->tm_hour = bcd2bin(rtc_data[2]);
	tm->tm_mday = bcd2bin(rtc_data[3]);
	tm->tm_mon = bcd2bin(rtc_data[4]) - 1;
	tm->tm_year = bcd2bin(rtc_data[5]) + 100;

	return ret;
}

/*
 * Gets current TWL RTC alarm time.
 */
int twl_read_alarm(struct rtc_wkalrm *alm)
{
	unsigned char rtc_data[ALL_TIME_REGS + 1];
	int ret;
	
	ret = i2c_read(0x4b,REG_ALARM_SECONDS_REG,1,rtc_data,ALL_TIME_REGS);

	if (ret) {
		printk("rtc_read_alarm error %d\n", ret);
		return ret;
	}
printk("read-alarm-time:%d-%d-%d,%d:%d:%d\n",rtc_data[5],rtc_data[4],rtc_data[3],rtc_data[2],rtc_data[1],rtc_data[0]);
	/* some of these fields may be wildcard/"match all" */
	alm->time.tm_sec = bcd2bin(rtc_data[0]);
	alm->time.tm_min = bcd2bin(rtc_data[1]);
	alm->time.tm_hour = bcd2bin(rtc_data[2]);
	alm->time.tm_mday = bcd2bin(rtc_data[3]);
	alm->time.tm_mon = bcd2bin(rtc_data[4]) - 1;
	alm->time.tm_year = bcd2bin(rtc_data[5]) + 100;

	return ret;
}

int twl_set_alarm(struct rtc_wkalrm *alm)
{
	char alarm_data[ALL_TIME_REGS + 1];
	int ret;

	ret = twl_alarm_irq_enable(0);
	if (ret)
		goto out;

	alarm_data[0] = bin2bcd(alm->time.tm_sec);
	alarm_data[1] = bin2bcd(alm->time.tm_min);
	alarm_data[2] = bin2bcd(alm->time.tm_hour);
	alarm_data[3] = bin2bcd(alm->time.tm_mday);
	alarm_data[4] = bin2bcd(alm->time.tm_mon + 1);
	alarm_data[5] = bin2bcd(alm->time.tm_year - 100);
	
	printk("set-alarm-time:%d-%d-%d,%d:%d:%d\n",alarm_data[5],alarm_data[4],alarm_data[3],alarm_data[2],alarm_data[1],alarm_data[0]);

	/* update all the alarm registers in one shot */
		
 	ret =	i2c_write(0x4b,REG_ALARM_SECONDS_REG,1,alarm_data,ALL_TIME_REGS);
	if (ret) {
		printk("rtc_set_alarm error %d\n", ret);
		goto out;
	}
	if (alm->enabled)
		ret = twl_alarm_irq_enable(1);
out:
	return ret;
}

int compare_android_alarm_to_myself_alarm(void)
{
	struct rtc_time     rtc_current_rtc_time;
	struct rtc_wkalrm 	rtc_alarm;
	unsigned long       rtc_current_time;
	unsigned long       rtc_alarm_time;
	unsigned char rtc_data;
	unsigned char alarm_status;
	
	i2c_read(0x4b,0x2b,1,&rtc_data,1);
	printk("=========rtc reg 0x2b = 0x%x\n",rtc_data);
	
	i2c_read(0x4b,0x2a,1,&alarm_status,1);
	printk("=========alarm_status reg 0x2a = 0x%x\n",alarm_status);
	alarm_status = alarm_status | (0x1 << 6);
	i2c_write(0x4b,0x2a,1,&alarm_status,1);
	
	
	twl_read_rtc(&rtc_current_rtc_time);
	
	rtc_tm_to_time(&rtc_current_rtc_time,&rtc_current_time);
	
//	test_rtc_current_time = rtc_current_time;
	
	rtc_alarm_time = get_alarm_time_for_low_battery();
	
	printk("============rtc_current_time = %ld\n",rtc_current_time);
	printk("============android rtc_alarm_time = %ld\n",rtc_alarm_time);
	
				if(((rtc_current_time + 3600) >= rtc_alarm_time) && (rtc_alarm_time >= 0))
					{
						printk("=======set android alarm\n");
						rtc_time_to_tm(rtc_alarm_time,&rtc_alarm.time);
						rtc_alarm.enabled = 1;
						twl_set_alarm(&rtc_alarm);
						
						if (rtc_current_time + 1 >= rtc_alarm_time) {
						printk("alarm about to go off\n");
						memset(&rtc_alarm, 0, sizeof(rtc_alarm));
						rtc_alarm.enabled = 0;
						twl_set_alarm(&rtc_alarm);
					 }
						set_polling_check_low_battery_flag_android(1);
						set_polling_check_low_battery_flag_myself(0);
					}
				else
					{
							rtc_current_time += 3600;
							rtc_time_to_tm(rtc_current_time, &rtc_alarm.time);
							
							rtc_tm_to_time(&rtc_alarm.time,&rtc_alarm_time);
							printk("============write alarm is %ld\n",rtc_alarm_time);
							rtc_alarm.enabled = 1;
							twl_set_alarm(&rtc_alarm);
							
							twl_read_alarm(&rtc_alarm);
							rtc_tm_to_time(&rtc_alarm.time,&rtc_alarm_time);
							printk("============write alarm---2 is %ld\n",rtc_alarm_time);
							i2c_read(0x4b,0x2b,1,&rtc_data,1);
							i2c_read(0x4b,0x2a,1,&alarm_status,1);
				/*	
							if(!(alarm_status & (0x1 << 6)) && (rtc_data & (0x1 << 3)))
							{
							gpio_direction_output(60,1);
				    	mdelay(200);
				    	gpio_direction_output(60,0);
				    	mdelay(200);
					    gpio_direction_output(60,1);
				    	mdelay(200);
				    	gpio_direction_output(60,0);
				    	mdelay(200);
						}
				*/	
					}
				
	return 0;
}
