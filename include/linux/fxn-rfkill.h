/*
 * Bluetooth TI wl127x rfkill power control via GPIO
 *
 * Copyright (C) 2009 Motorola, Inc.
 * Copyright (C) 2008 Texas Instruments
 * Initial code: Pavan Savoy <pavan.savoy@gmail.com> (wl127x_power.c)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _LINUX_FXN_RFKILL_H
#define _LINUX_FXN_RFKILL_H

enum bcm4329_devices {
	FXN_WIFI = 0,
	FXN_BT,
	FXN_GPS_RESET,
	FXN_GPS_STANDBY,
	FXN_MAX_DEV,
};
	
#define FXN_GPIO_WL_BT_POW_EN   22 
#define FXN_GPIO_BT_HOST_WAKEUP 39 
#define FXN_GPIO_BT_WAKEUP      40 
#define FXN_GPIO_GPS_PWR_EN     159
#define FXN_GPIO_GPS_ON_OFF     109
#define FXN_GPIO_GPS_WAKEUP     62 
#define FXN_GPIO_WL_RST_N       158
#define FXN_GPIO_BT_RST_N       102
#define FXN_GPIO_GPS_RST        101
#define FXN_GPIO_UART1_SW       110
	
struct fxn_rfkill_platform_data {
    int wifi_bt_pwr_en_gpio;
    int wifi_reset_gpio;
    int gps_pwr_en_gpio;
    int gps_on_off_gpio;
    int gps_reset_gpio;
    int bt_reset_gpio;
    struct rfkill *rfkill[FXN_MAX_DEV];  /* for driver only */
};

#endif
