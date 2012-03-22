/*
 * pca9532.h - platform data structure for pca9532 led controller
 *
 * Copyright (C) 2008 Riku Voipio <riku.voipio@movial.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * Datasheet: http://www.nxp.com/acrobat/datasheets/PCA9532_3.pdf
 *
 */

#ifndef __LINUX_ep10_LEDS_H
#define __LINUX_ep10_LEDS_H

#include <linux/leds.h>

struct ep10_led_platdata {
	unsigned int  gpio_charge;
	unsigned int  gpio_charge_full;
	unsigned int  gpio;
	unsigned int  flags;
	const char	* name;
	const char	* def_trigger;
};

#endif /* __LINUX_PCA9532_H */

