/*
 * linux/drivers/i2c/chips/twl4030-poweroff.c
 *
 * Power off device
 *
 * Copyright (C) 2008 Nokia Corporation
 *
 * Written by Peter De Schrijver <peter.de-schrijver@nokia.com>
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/pm.h>
#include <linux/i2c/twl.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <linux/reboot.h>

#define PWR_P1_SW_EVENTS	0x10
#define PWR_DEVOFF	(1<<0)
#define PWR_STOPON_POWERON (1<<6)

#define CFG_P123_TRANSITION	0x03
#define SEQ_OFFSYNC	(1<<0)

#define REG_RTC_INTERRUPTS_REG 0x0F
#define REG_RTC_STATUS_REG 		 0x0E

void set_reboot_flag(int value)
{
  u32 *reg_mem = ioremap(0x9ff00000,0x20); 
  
  __raw_writel(value,reg_mem);
  
   iounmap(reg_mem);
}
EXPORT_SYMBOL_GPL(set_reboot_flag);

void twl4030_poweroff(void)
{
	u8 uninitialized_var(val);
	int err;
	u8 rtc_val;
	

//&*&*&HC1_20110413, modify for auto-reboot function
	// if cable is attached, disconnect the VBUS for auto-reboot
	// check the DETECT_DEVICE (gpio_51) pin status
	if (!gpio_get_value(51))
	 {
    		// set CHG_OFFMODE (gpio_34) as output high
    		set_reboot_flag(0x12345678);
    		kernel_restart(NULL);     
    
    //&*&*&HC1_20110531, extend the delay time to prevent some failure case 
    		//mdelay(40);
    		mdelay(100);
    //&*&*&HC2_20110531, extend the delay time to prevent some failure case
	 }
	else
		{
     //&*&*&HC2_20110413, modify for auto-reboot function
     
      twl_i2c_read_u8(TWL_MODULE_RTC, &rtc_val, REG_RTC_STATUS_REG);
     	rtc_val |= (0x1 << 6);
     	twl_i2c_write_u8(TWL_MODULE_RTC, rtc_val, REG_RTC_STATUS_REG);	
     
     //&*&*&HC1_20110629, disable RTC alarm interrupt to prevent unexpected power on
     	twl_i2c_read_u8(TWL_MODULE_RTC, &rtc_val, REG_RTC_INTERRUPTS_REG);
     	rtc_val &= ~(0x08);
     	twl_i2c_write_u8(TWL_MODULE_RTC, rtc_val, REG_RTC_INTERRUPTS_REG);		
     //&*&*&HC2_20110629, disable RTC alarm interrupt to prevent unexpected power on
     
     
     	/* Make sure SEQ_OFFSYNC is set so that all the res goes to wait-on */
     	err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
     				   CFG_P123_TRANSITION);
     	if (err) {
     		pr_warning("I2C error %d while reading TWL4030 PM_MASTER CFG_P123_TRANSITION\n",
     			err);
     		return;
     	}
     
     	val |= SEQ_OFFSYNC;
     	err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
     				    CFG_P123_TRANSITION);
     	if (err) {
     		pr_warning("I2C error %d while writing TWL4030 PM_MASTER CFG_P123_TRANSITION\n",
     			err);
     		return;
     	}
     
     	err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
     				  PWR_P1_SW_EVENTS);
     	if (err) {
     		pr_warning("I2C error %d while reading TWL4030 PM_MASTER P1_SW_EVENTS\n",
     			err);
     		return;
     	}
     
     	val |= PWR_STOPON_POWERON | PWR_DEVOFF;
     
     	err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
     				   PWR_P1_SW_EVENTS);
     
     	if (err) {
     		pr_warning("I2C error %d while writing TWL4030 PM_MASTER P1_SW_EVENTS\n",
     			err);
     		return;
     	}
}

	return;
}

static int __init twl4030_poweroff_init(void)
{
	pm_power_off = twl4030_poweroff;

	return 0;
}

static void __exit twl4030_poweroff_exit(void)
{
	pm_power_off = NULL;
}

module_init(twl4030_poweroff_init);
module_exit(twl4030_poweroff_exit);

MODULE_DESCRIPTION("Triton2 device power off");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter De Schrijver");
