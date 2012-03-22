/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include "vibrator.h"

struct vibrator_dev *vibrator;

static enum hrtimer_restart timer_func(struct hrtimer *timer)
{
	gpio_direction_output(vibrator->gpio,  0 );
        vibrator->current_state = 0;
	return HRTIMER_NORESTART;
}

static ssize_t show_vibrator_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", vibrator->current_state);
}

static ssize_t store_vibrator_status(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	val = simple_strtoul(buf, NULL, 10);
	if(val > 0)
	{
		vibrator->current_state = 1;
	}
	else
	{
		vibrator->current_state = 0;
		gpio_direction_output(vibrator->gpio,  0 );
		hrtimer_cancel(&vibrator->timer);
	}
	
	return count;
}


static ssize_t store_vibrator_timer(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int value;
	unsigned long	flags;
	if (sscanf(buf, "%d", &value) != 1)
		return -EINVAL;
	
	spin_lock_irqsave(&vibrator->lock, flags);

	/* cancel previous timer and set GPIO according to value */
	hrtimer_cancel(&vibrator->timer);

	if (value > 0 && vibrator->current_state == 1) {
	    
        gpio_direction_output(vibrator->gpio,  1 );	
		hrtimer_start(&vibrator->timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}
	
	spin_unlock_irqrestore(&vibrator->lock, flags);
	
	return 0;
}
static DEVICE_ATTR(status, 0666, show_vibrator_status, store_vibrator_status);
static DEVICE_ATTR(timer, 0666, NULL, store_vibrator_timer);


/**vibrator driver probe*/
static int __devinit vibrator_probe(struct platform_device *pdev)
{
	 
	 int ret=0,err=0;
	 vibrator=(struct vibrator_dev *)kzalloc(sizeof(struct vibrator_dev),GFP_KERNEL);
	 vibrator->gpio=CL1_VIBRATOR_GPIO_EN;
	 hrtimer_init(&vibrator->timer, CLOCK_MONOTONIC,
				HRTIMER_MODE_REL);
				
	vibrator->timer.function = timer_func;
	spin_lock_init(&vibrator->lock);
	err=device_create_file(&pdev->dev ,&dev_attr_status);
	if(err<0)
	{
	   printk("create file err\n");
	   return err;	  
	 }
	err=device_create_file(&pdev->dev ,&dev_attr_timer);
	if(err<0)
	{
	   printk("create file err\n");
	   return err;	  
	 } 
	ret=gpio_request(vibrator->gpio,"vibrator");
	if(ret<0){
	   printk("gpio request err\n");
	   return ret;
	}	
	 
	return 0;
}

static int __devexit vibrator_remove(struct platform_device *pdev)
{
    int ret=0;
	
	gpio_free(vibrator->gpio);
	kfree(vibrator);
	
	return ret;
}

//&*&*&HC1_20110428, disable keypad wakeup source
#ifdef CONFIG_PM

static int vibrator_suspend(struct platform_device *pdev,
				 pm_message_t state)
{
	int ret = 0;
	

	return ret;
}

static int vibrator_resume(struct platform_device *pdev)
{
	
	int ret = 0;

	return ret;
}
#else	/* !CONFIG_PM */
#define vibrator_suspend	NULL
#define vibrator_resume	NULL
#endif	/* !CONFIG_PM */


static struct platform_driver vibrator_driver = {
	.probe		= vibrator_probe,
	.remove		= __devexit_p(vibrator_remove),
	.driver		= {
		.name	= "vibrator",
		.owner	= THIS_MODULE,
	},
	.suspend		= vibrator_suspend,
	.resume		= vibrator_resume,
};

static int __init vibrator_init(void)
{
	return platform_driver_register(&vibrator_driver);
}
module_init(vibrator_init);

static void __exit vibrator_exit(void)
{
	platform_driver_unregister(&vibrator_driver);
}
module_exit(vibrator_exit);

MODULE_AUTHOR("Andy cheng");
MODULE_DESCRIPTION("vibrator Driver");
MODULE_LICENSE("GPL");

