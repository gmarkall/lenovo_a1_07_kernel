/*
 * pm.c - Common OMAP2+ power management-related code
 *
 * Copyright (C) 2010 Texas Instruments, Inc.
 * Copyright (C) 2010 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/err.h>

#include <plat/omap-pm.h>
#include <plat/omap_device.h>
#include <plat/common.h>
#include <plat/opp.h>
#include <plat/voltage.h>

#include "omap3-opp.h"
#include "opp44xx.h"

static struct omap_device_pm_latency *pm_lats;

static struct device *mpu_dev;
static struct device *iva_dev;
static struct device *l3_dev;
static struct device *dsp_dev;

struct device *omap2_get_mpuss_device(void)
{
	WARN_ON_ONCE(!mpu_dev);
	return mpu_dev;
}
EXPORT_SYMBOL(omap2_get_mpuss_device);

struct device *omap2_get_iva_device(void)
{
	WARN_ON_ONCE(!iva_dev);
	return iva_dev;
}
EXPORT_SYMBOL(omap2_get_iva_device);

struct device *omap2_get_l3_device(void)
{
	WARN_ON_ONCE(!l3_dev);
	return l3_dev;
}
EXPORT_SYMBOL(omap2_get_l3_device);

struct device *omap4_get_dsp_device(void)
{
	WARN_ON_ONCE(!dsp_dev);
	return dsp_dev;
}
EXPORT_SYMBOL(omap4_get_dsp_device);

/* Overclock vdd sysfs interface */
static ssize_t overclock_vdd_show(struct kobject *, struct kobj_attribute *,
              char *);
static ssize_t overclock_vdd_store(struct kobject *k, struct kobj_attribute *,
const char *buf, size_t n);


static struct kobj_attribute overclock_vdd_opp1_attr =
    __ATTR(overclock_vdd_opp1, 0644, overclock_vdd_show, overclock_vdd_store);
static struct kobj_attribute overclock_vdd_opp2_attr =
    __ATTR(overclock_vdd_opp2, 0644, overclock_vdd_show, overclock_vdd_store);
static struct kobj_attribute overclock_vdd_opp3_attr =
    __ATTR(overclock_vdd_opp3, 0644, overclock_vdd_show, overclock_vdd_store);
static struct kobj_attribute overclock_vdd_opp4_attr =
    __ATTR(overclock_vdd_opp4, 0644, overclock_vdd_show, overclock_vdd_store);

static ssize_t overclock_vdd_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
	unsigned int target_opp;
	unsigned long *vdd = -1;
	unsigned long *temp_vdd = -1;
	char *voltdm_name = "mpu";
	struct device *mpu_dev = omap2_get_mpuss_device();
	struct cpufreq_frequency_table *mpu_freq_table = *omap_pm_cpu_get_freq_table();
	struct omap_opp *temp_opp;
	struct voltagedomain *mpu_voltdm;
	struct omap_volt_data *mpu_voltdata;

	if(!mpu_dev || !mpu_freq_table)
	    return -EINVAL;

	if ( attr == &overclock_vdd_opp1_attr) {
	    target_opp = 0;
	}
	if ( attr == &overclock_vdd_opp2_attr) {
	    target_opp = 1;
	}
	if ( attr == &overclock_vdd_opp3_attr) {
	    target_opp = 2;
	}
	if ( attr == &overclock_vdd_opp4_attr) {
	    target_opp = 3;
	}

	temp_opp = opp_find_freq_exact(mpu_dev, mpu_freq_table[target_opp].frequency*1000, true);
	if(IS_ERR(temp_opp))
	    return -EINVAL;

	temp_vdd = opp_get_voltage(temp_opp);
	mpu_voltdm = omap_voltage_domain_get(voltdm_name);
	mpu_voltdata = omap_voltage_get_voltdata(mpu_voltdm, temp_vdd);
	vdd = mpu_voltdata->volt_nominal;

	return sprintf(buf, "%lu\n", vdd);
}

static ssize_t overclock_vdd_store(struct kobject *k,
        struct kobj_attribute *attr, const char *buf, size_t n)
{
	// this is read-only atm
	return -EINVAL;
}

/* static int _init_omap_device(struct omap_hwmod *oh, void *user) */
static int _init_omap_device(char *name, struct device **new_dev)
{
	struct omap_hwmod *oh;
	struct omap_device *od;

	oh = omap_hwmod_lookup(name);
	if (WARN(!oh, "%s: could not find omap_hwmod for %s\n",
		 __func__, name))
		return -ENODEV;
	od = omap_device_build(oh->name, 0, oh, NULL, 0, pm_lats, 0, false);
	if (WARN(IS_ERR(od), "%s: could not build omap_device for %s\n",
		 __func__, name))
		return -ENODEV;

	*new_dev = &od->pdev.dev;

	return 0;
}

/*
 * Build omap_devices for processors and bus.
 */
static void omap2_init_processor_devices(void)
{
	struct omap_hwmod *oh;

	_init_omap_device("mpu", &mpu_dev);

	if (cpu_is_omap34xx())
		_init_omap_device("iva", &iva_dev);
	oh = omap_hwmod_lookup("iva");
	if (oh && oh->od)
		iva_dev = &oh->od->pdev.dev;

	oh = omap_hwmod_lookup("dsp");
	if (oh && oh->od)
		dsp_dev = &oh->od->pdev.dev;

	if (cpu_is_omap44xx())
		_init_omap_device("l3_main_1", &l3_dev);
	else
		_init_omap_device("l3_main", &l3_dev);
}

static int __init omap2_common_pm_init(void)
{
	omap2_init_processor_devices();
	if (cpu_is_omap34xx())
		omap3_pm_init_opp_table();
	else if (cpu_is_omap44xx())
		omap4_pm_init_opp_table();

	omap_pm_if_init();

	int error = -EINVAL;
	error = sysfs_create_file(power_kobj, &overclock_vdd_opp1_attr.attr);
	if (error) {
	    printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
	    return error;
	}
	error = sysfs_create_file(power_kobj, &overclock_vdd_opp2_attr.attr);
	if (error) {
	    printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
	    return error;
	}
	error = sysfs_create_file(power_kobj, &overclock_vdd_opp3_attr.attr);
	if (error) {
	    printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
	    return error;
	}
	error = sysfs_create_file(power_kobj, &overclock_vdd_opp4_attr.attr);
	if (error) {
	    printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
	    return error;
	}

	return 0;
}

device_initcall(omap2_common_pm_init);
