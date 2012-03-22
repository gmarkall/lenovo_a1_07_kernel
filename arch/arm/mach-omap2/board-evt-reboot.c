/*
 * Encore machine emergency restart routine called on kernel panic.
 *
 * Copyright (C) 2011 Barnes & Noble 
 *
 * Written by David Bolcsfoldi <dbolcsfoldi@intrinsyc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>

#include <plat/io.h>
#include <plat/prcm.h>
#include <plat/control.h>

#include "prcm-common.h"
#include "clock.h"

// This code is yanked from arch/arm/mach-omap2/prcm.c
void machine_emergency_restart(void)
{
	s16 prcm_offs;
	u32 l;

	// delay here to allow eMMC to finish any internal housekeeping before reset
	// even if mdelay fails to work correctly, 8 second button press should work 
	// this used to be an msleep but scheduler is gone here and calling msleep
	// will cause a panic
	mdelay(1600);

	//omap3_clk_prepare_for_reboot();

	prcm_offs = OMAP3430_GR_MOD;
	l = ('B' << 24) | ('M' << 16) | 'h';
	/* Reserve the first word in scratchpad for communicating
	* with the boot ROM. A pointer to a data structure
	* describing the boot process can be stored there,
	* cf. OMAP34xx TRM, Initialization / Software Booting
	* Configuration. */
	omap_writel(l, OMAP343X_SCRATCHPAD + 4);
}

