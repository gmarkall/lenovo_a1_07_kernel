/*
 * omap_wdt.c
 *
 * Watchdog driver for the TI OMAP 16xx & 24xx/34xx 32KHz (non-secure) watchdog
 *
 * Author: MontaVista Software, Inc.
 *	 <gdavis@mvista.com> or <source@mvista.com>
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * History:
 *
 * 20030527: George G. Davis <gdavis@mvista.com>
 *	Initially based on linux-2.4.19-rmk7-pxa1/drivers/char/sa1100_wdt.c
 *	(c) Copyright 2000 Oleg Drokin <green@crimea.edu>
 *	Based on SoftDog driver by Alan Cox <alan@lxorguk.ukuu.org.uk>
 *
 * Copyright (c) 2004 Texas Instruments.
 *	1. Modified to support OMAP1610 32-KHz watchdog timer
 *	2. Ported to 2.6 kernel
 *
 * Copyright (c) 2005 David Brownell
 *	Use the driver model and standard identifiers; handle bigger timeouts.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/moduleparam.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#else
#include <linux/clk.h>
#endif
#ifdef CONFIG_OMAP_WATCHDOG_AUTOPET
#include <linux/timer.h>
#endif
#include <mach/hardware.h>
#include <plat/prcm.h>
#include <plat/omap_device.h>

#include "omap_wdt.h"

static struct platform_device *omap_wdt_dev;

/* <-- LH_SWRD_CL1_Henry@2011.8.20 Setup default timer_margin=20s (from 60s)  */	
static unsigned timer_margin=20;
module_param(timer_margin, uint, 0);
MODULE_PARM_DESC(timer_margin, "initial watchdog timeout (in seconds)");

static unsigned int wdt_trgr_pattern = 0x1234;
static spinlock_t wdt_lock;
/* <-- LH_SWRD_CL1_andy@2011.9.08*/
#define MAX_RSTCIRCLE 40
int run_circle=0;
int boot=0;
EXPORT_SYMBOL_GPL(boot);

struct omap_wdt_dev {
	void __iomem    *base;          /* physical */
	struct device   *dev;
	int             omap_wdt_users;
	struct resource *mem;
	struct miscdevice omap_wdt_miscdev;
#ifdef CONFIG_OMAP_WATCHDOG_AUTOPET
	struct timer_list autopet_timer;
	unsigned long  jiffies_start;
	unsigned long  jiffies_exp;
#endif

};

static void omap_wdt_ping(struct omap_wdt_dev *wdev)
{
	void __iomem    *base = wdev->base;

	/* wait for posted write to complete */
	while ((__raw_readl(base + OMAP_WATCHDOG_WPS)) & 0x08)
		cpu_relax();

	wdt_trgr_pattern = ~wdt_trgr_pattern;
	__raw_writel(wdt_trgr_pattern, (base + OMAP_WATCHDOG_TGR));

	/* wait for posted write to complete */
	while ((__raw_readl(base + OMAP_WATCHDOG_WPS)) & 0x08)
		cpu_relax();
	/* reloaded WCRR from WLDR */
}

static void omap_wdt_enable(struct omap_wdt_dev *wdev)
{
	void __iomem *base = wdev->base;

	/* Sequence to enable the watchdog */
	__raw_writel(0xBBBB, base + OMAP_WATCHDOG_SPR);
	while ((__raw_readl(base + OMAP_WATCHDOG_WPS)) & 0x10)
		cpu_relax();

	__raw_writel(0x4444, base + OMAP_WATCHDOG_SPR);
	while ((__raw_readl(base + OMAP_WATCHDOG_WPS)) & 0x10)
		cpu_relax();
}

static void omap_wdt_disable(struct omap_wdt_dev *wdev)
{
	void __iomem *base = wdev->base;

	/* sequence required to disable watchdog */
	__raw_writel(0xAAAA, base + OMAP_WATCHDOG_SPR);	/* TIMER_MODE */
	while (__raw_readl(base + OMAP_WATCHDOG_WPS) & 0x10)
		cpu_relax();

	__raw_writel(0x5555, base + OMAP_WATCHDOG_SPR);	/* TIMER_MODE */
	while (__raw_readl(base + OMAP_WATCHDOG_WPS) & 0x10)
		cpu_relax();
}

static void omap_wdt_adjust_timeout(unsigned new_timeout)
{
	if (new_timeout < TIMER_MARGIN_MIN)
		new_timeout = TIMER_MARGIN_DEFAULT;
	if (new_timeout > TIMER_MARGIN_MAX)
		new_timeout = TIMER_MARGIN_MAX;
	timer_margin = new_timeout;
}

static void omap_wdt_set_timeout(struct omap_wdt_dev *wdev)
{
	u32 pre_margin = GET_WLDR_VAL(timer_margin);
	void __iomem *base = wdev->base;

	/* just count up at 32 KHz */
	while (__raw_readl(base + OMAP_WATCHDOG_WPS) & 0x04)
		cpu_relax();

	__raw_writel(pre_margin, base + OMAP_WATCHDOG_LDR);
	while (__raw_readl(base + OMAP_WATCHDOG_WPS) & 0x04)
		cpu_relax();
}

static void omap_wdt_startclocks(struct omap_wdt_dev *wdev)
{
	void __iomem *base = wdev->base;

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_get_sync(wdev->dev);
#else
	if (!cpu_is_omap44xx())
		clk_enable(wdev->ick);    /* Enable the interface clock */
	clk_enable(wdev->fck);    /* Enable the functional clock */
#endif


	/* initialize prescaler */
	while (__raw_readl(base + OMAP_WATCHDOG_WPS) & 0x01)
		cpu_relax();

	__raw_writel((1 << 5) | (PTV << 2), base + OMAP_WATCHDOG_CNTRL);
	while (__raw_readl(base + OMAP_WATCHDOG_WPS) & 0x01)
		cpu_relax();
}

/*
 *	Allow only one task to hold it open
 */
static int omap_wdt_open(struct inode *inode, struct file *file)
{
	struct omap_wdt_dev *wdev = platform_get_drvdata(omap_wdt_dev);
	void __iomem *base = wdev->base;

printk("\n\n\n\n\n*************andy in the function %s\n\n\n",__FUNCTION__);
	if (test_and_set_bit(1, (unsigned long *)&(wdev->omap_wdt_users)))
		return -EBUSY;
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_get_sync(wdev->dev);
#else
	if (!cpu_is_omap44xx())
		clk_enable(wdev->ick);    /* Enable the interface clock */
	clk_enable(wdev->fck);    /* Enable the functional clock */
#endif

	/* initialize prescaler */
	while (__raw_readl(base + OMAP_WATCHDOG_WPS) & 0x01)
		cpu_relax();

	__raw_writel((1 << 5) | (PTV << 2), base + OMAP_WATCHDOG_CNTRL);
	while (__raw_readl(base + OMAP_WATCHDOG_WPS) & 0x01)
		cpu_relax();

	file->private_data = (void *) wdev;

	omap_wdt_set_timeout(wdev);
	omap_wdt_ping(wdev); /* trigger loading of new timeout value */
	omap_wdt_enable(wdev);

	return nonseekable_open(inode, file);
}

static int omap_wdt_release(struct inode *inode, struct file *file)
{
	struct omap_wdt_dev *wdev = file->private_data;

	/*
	 *      Shut off the timer unless NOWAYOUT is defined.
	 */
#ifndef CONFIG_WATCHDOG_NOWAYOUT
	omap_wdt_disable(wdev);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_put_sync(wdev->dev);
#else
	if (!cpu_is_omap44xx())
		clk_disable(wdev->ick);
	clk_disable(wdev->fck);
#endif

#else
	printk(KERN_CRIT "omap_wdt: Unexpected close, not stopping!\n");
#endif
	wdev->omap_wdt_users = 0;

	return 0;
}

static ssize_t omap_wdt_write(struct file *file, const char __user *data,
		size_t len, loff_t *ppos)
{
	struct omap_wdt_dev *wdev = file->private_data;

	/* Refresh LOAD_TIME. */
	if (len) {
		spin_lock(&wdt_lock);
		omap_wdt_ping(wdev);
		spin_unlock(&wdt_lock);
	}
	return len;
}

static long omap_wdt_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg)
{
	struct omap_wdt_dev *wdev;
	int new_margin;
	static const struct watchdog_info ident = {
		.identity = "OMAP Watchdog",
		.options = WDIOF_SETTIMEOUT,
		.firmware_version = 0,
	};

	wdev = file->private_data;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		return copy_to_user((struct watchdog_info __user *)arg, &ident,
				sizeof(ident));
	case WDIOC_GETSTATUS:
		return put_user(0, (int __user *)arg);
	case WDIOC_GETBOOTSTATUS:
		if (cpu_is_omap16xx())
			return put_user(__raw_readw(ARM_SYSST),
					(int __user *)arg);
		if (cpu_is_omap24xx())
			return put_user(omap_prcm_get_reset_sources(),
					(int __user *)arg);
	case WDIOC_KEEPALIVE:
		spin_lock(&wdt_lock);
		omap_wdt_ping(wdev);
		spin_unlock(&wdt_lock);
		return 0;
	case WDIOC_SETTIMEOUT:
		if (get_user(new_margin, (int __user *)arg))
			return -EFAULT;
		omap_wdt_adjust_timeout(new_margin);

		spin_lock(&wdt_lock);
		omap_wdt_disable(wdev);
		omap_wdt_set_timeout(wdev);
		omap_wdt_enable(wdev);

		omap_wdt_ping(wdev);
		spin_unlock(&wdt_lock);
		/* Fall */
	case WDIOC_GETTIMEOUT:
		return put_user(timer_margin, (int __user *)arg);
	default:
		return -ENOTTY;
	}
}

static const struct file_operations omap_wdt_fops = {
	.owner = THIS_MODULE,
	.write = omap_wdt_write,
	.unlocked_ioctl = omap_wdt_ioctl,
	.open = omap_wdt_open,
	.release = omap_wdt_release,
};
static ssize_t stage_show(struct device *dev, struct device_attribute *attr, char *buf)
{		
	return sprintf(buf, " remain_circle=%d\n", MAX_RSTCIRCLE-run_circle);
}
static ssize_t normal_boot(struct device *dev, struct device_attribute *attr, const char *buf,
		size_t count)
{
	int val = simple_strtol(buf, NULL, 10);
	if(val == 1)
	{
	  boot=1;
	}
	return count;
}
static DEVICE_ATTR(watchdog_reset, S_IRUGO|S_IWUSR|S_IWGRP, stage_show, normal_boot);

static struct attribute *watchdog_attributes[] = {
      &dev_attr_watchdog_reset.attr,
      NULL
};

static const struct attribute_group watchdog_attr_group = {
	.attrs = watchdog_attributes,
};

#ifdef CONFIG_OMAP_WATCHDOG_AUTOPET
static void autopet_handler(unsigned long data)
{
	unsigned long flags;
	struct omap_wdt_dev *wdev = (struct omap_wdt_dev *) data;	
	if( boot == 0 )
        run_circle++;
	//printk("****run_circle=%d\n",run_circle);
	spin_lock_irqsave(&wdt_lock, flags);
	omap_wdt_ping(wdev);
	spin_unlock_irqrestore(&wdt_lock, flags);
	wdev->jiffies_start = jiffies;
	wdev->jiffies_exp = (HZ * TIMER_AUTOPET_FREQ);
	if( run_circle < MAX_RSTCIRCLE )
	   mod_timer(&(wdev->autopet_timer), jiffies + wdev->jiffies_exp);
}
#endif

static int __devinit omap_wdt_probe(struct platform_device *pdev)
{
	struct resource *res, *mem;
	struct omap_wdt_dev *wdev;
	int ret;

	/* reserve static register mappings */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENOENT;
		goto err_get_resource;
	}

	if (omap_wdt_dev) {
		ret = -EBUSY;
		goto err_busy;
	}

	mem = request_mem_region(res->start, resource_size(res), pdev->name);
	if (!mem) {
		ret = -EBUSY;
		goto err_busy;
	}

	wdev = kzalloc(sizeof(struct omap_wdt_dev), GFP_KERNEL);
	if (!wdev) {
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	wdev->omap_wdt_users = 0;
	wdev->mem = mem;
	wdev->dev = &pdev->dev;

	wdev->base = ioremap(res->start, resource_size(res));
	if (!wdev->base) {
		ret = -ENOMEM;
		goto err_ioremap;
	}

	platform_set_drvdata(pdev, wdev);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_enable(wdev->dev);
	pm_runtime_get_sync(wdev->dev);
#else
	if (!cpu_is_omap44xx())
		clk_enable(wdev->ick);    /* Enable the interface clock */
	clk_enable(wdev->fck);    /* Enable the functional clock */
#endif
	omap_wdt_disable(wdev);
	omap_wdt_adjust_timeout(timer_margin);

	wdev->omap_wdt_miscdev.parent = &pdev->dev;
	wdev->omap_wdt_miscdev.minor = WATCHDOG_MINOR;
	wdev->omap_wdt_miscdev.name = "watchdog";
	wdev->omap_wdt_miscdev.fops = &omap_wdt_fops;

	ret = misc_register(&(wdev->omap_wdt_miscdev));
	if (ret)
		goto err_misc;

	pr_info("OMAP Watchdog Timer Rev 0x%02x: initial timeout %d sec\n",
		__raw_readl(wdev->base + OMAP_WATCHDOG_REV) & 0xFF,
		timer_margin);
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_put_sync(wdev->dev);
#else
	if (!cpu_is_omap44xx())
		clk_disable(wdev->ick);
	clk_disable(wdev->fck);

#endif

	omap_wdt_dev = pdev;
	
   ret = sysfs_create_group(&omap_wdt_dev->dev.kobj, &watchdog_attr_group);
	if (ret)
		printk(" watchdog failed to create sysfs files\n",__FUNCTION__);
		
#ifdef CONFIG_OMAP_WATCHDOG_AUTOPET
	set_bit(1, (unsigned long *)&(wdev->omap_wdt_users));
	setup_timer(&wdev->autopet_timer, autopet_handler,
		    (unsigned long) wdev);
	omap_wdt_startclocks(wdev);
	omap_wdt_set_timeout(wdev);
	wdev->jiffies_start = jiffies;
	wdev->jiffies_exp = (HZ * TIMER_AUTOPET_FREQ);
	mod_timer(&wdev->autopet_timer, jiffies + wdev->jiffies_exp);
	omap_wdt_enable(wdev);
	pr_info("Watchdog auto-pet enabled at %d sec intervals\n",
		TIMER_AUTOPET_FREQ);
	omap_wdt_ping(wdev);

#endif
	return 0;

err_misc:
	platform_set_drvdata(pdev, NULL);
	iounmap(wdev->base);

err_ioremap:
	wdev->base = NULL;
	kfree(wdev);

err_kzalloc:
	release_mem_region(res->start, resource_size(res));

err_busy:
err_get_resource:

	return ret;
}

static void omap_wdt_shutdown(struct platform_device *pdev)
{
	struct omap_wdt_dev *wdev = platform_get_drvdata(pdev);

	if (wdev->omap_wdt_users)
		omap_wdt_disable(wdev);
}

static int __devexit omap_wdt_remove(struct platform_device *pdev)
{
	struct omap_wdt_dev *wdev = platform_get_drvdata(pdev);
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!res)
		return -ENOENT;

	misc_deregister(&(wdev->omap_wdt_miscdev));
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_put_sync(wdev->dev);
	pm_runtime_disable(wdev->dev);
#else
	if (!cpu_is_omap44xx())
		clk_put(wdev->ick);
	clk_put(wdev->fck);
#endif

	iounmap(wdev->base);

	kfree(wdev);
	omap_wdt_dev = NULL;

	return 0;
}

#ifdef	CONFIG_PM

/* REVISIT ... not clear this is the best way to handle system suspend; and
 * it's very inappropriate for selective device suspend (e.g. suspending this
 * through sysfs rather than by stopping the watchdog daemon).  Also, this
 * may not play well enough with NOWAYOUT...
 */

static int omap_wdt_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct omap_wdt_dev *wdev = platform_get_drvdata(pdev);

	if (wdev->omap_wdt_users)
		omap_wdt_disable(wdev);
#ifdef CONFIG_OMAP_WATCHDOG_AUTOPET
	wdev->jiffies_exp = wdev->jiffies_exp - (jiffies - wdev->jiffies_start);
	del_timer(&wdev->autopet_timer);
#endif

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_put_sync(wdev->dev);
#else
	if (!cpu_is_omap44xx())
		clk_put(wdev->ick);
	clk_put(wdev->fck);
#endif
	return 0;
}

static int omap_wdt_resume(struct platform_device *pdev)
{
	struct omap_wdt_dev *wdev = platform_get_drvdata(pdev);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_get_sync(wdev->dev);
#else
	if (!cpu_is_omap44xx())
		clk_get(wdev->ick);
	clk_get(wdev->fck);
#endif

#ifdef CONFIG_OMAP_WATCHDOG_AUTOPET
	mod_timer(&wdev->autopet_timer, jiffies + wdev->jiffies_exp);
#endif

	if (wdev->omap_wdt_users) {
		omap_wdt_enable(wdev);
		omap_wdt_ping(wdev);
	}

	return 0;
}

#else
#define	omap_wdt_suspend	NULL
#define	omap_wdt_resume		NULL
#endif

static struct platform_driver omap_wdt_driver = {
	.probe		= omap_wdt_probe,
	.remove		= __devexit_p(omap_wdt_remove),
	.shutdown	= omap_wdt_shutdown,
	.suspend	= omap_wdt_suspend,
	.resume		= omap_wdt_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "omap_wdt",
	},
};

static int __init omap_wdt_init(void)
{
	spin_lock_init(&wdt_lock);
	return platform_driver_register(&omap_wdt_driver);
}

static void __exit omap_wdt_exit(void)
{
	platform_driver_unregister(&omap_wdt_driver);
}

module_init(omap_wdt_init);
module_exit(omap_wdt_exit);

MODULE_AUTHOR("George G. Davis");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:omap_wdt");
