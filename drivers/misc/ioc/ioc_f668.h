/*
 * Copyright (c) 2011 Foxconn Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef _IOC_F668_H_
#define _IOC_F668_H_

#define IOC_MAJOR				245		 
#define IOC_DEVICE_NAME	"ioc_node"

#define IOC_GPIO_INPUT	1
#define IOC_GPIO_OUTPUT	0

struct init_ioc_gpios {
	u8 gpio_num;
	u8 gpio_dir;
	u8 gpio_val;
	char *gpio_name;
};

#define IOCF668_I2C_DEVICE_NAME     "iocf668"
#define IOCF668_DEVICE_NAME         "iocf668"

#define IOC_POWER_GPIO  						16
#define IOC_RST_GPIO								150
#define IOC_I2C_REQ_GPIO						115	
#define	UP_IOC_DATA_GPIO						11 //(EVT2)
//#define	UP_IOC_DATA_GPIO						14	//(EVT1)
#define UP_IOC_CLK_GPIO							31
#define UP_IOC_LEVEL_GPIO           60
#define IOC_USB1_ID									41

#endif /* _IOC_F668_H_ */
