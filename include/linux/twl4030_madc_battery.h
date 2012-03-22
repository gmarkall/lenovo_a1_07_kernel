/*
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __TWL4030_MADC_BATTERY_H_
#define __TWL4030_MADC_BATTERY_H_

struct twl4030_madc_battery_platform_data {
	int (*battery_is_online)(void);
	int (*charger_is_online)(void);
	int (*battery_is_charging)(void);
	int (*charger_enable)(void);
};

#endif
