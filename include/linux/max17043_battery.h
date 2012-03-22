/*
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAX17043_BATTERY_H_
#define __MAX17043_BATTERY_H_

struct max17043_irq_info {
    int		irq;
	unsigned int     gpio;
	int     irq_flags;
};

struct max17043_platform_data {
	struct max17043_irq_info *irq_info;
	int (*battery_is_online)(void);
	int (*charger_is_online)(void);
	int (*battery_is_charging)(void);
	int (*charger_enable)(void);
};

#ifdef CONFIG_BATTERY_MAX17043_TEMPERATURE_PROTECTION
struct max17043_temp_access {
	u16 V_bat_ntc; 	 /* max17043 NTC voltage value */
	u16 R_center;	 /* max17043 R value */
	int T_temp_ntc;	 /* max17043 NTC temperature value */
};
#endif

#endif
