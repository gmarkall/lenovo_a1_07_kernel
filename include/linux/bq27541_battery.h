/*
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
 
#define T_POLL_MS			2000

#define BQ27541_REG_CTRL    0x00
#define BQ27541_REG_AR			0x02
#define BQ27541_REG_ARTTE		0x04

//#define BQ27541_REG_TEMP		0x06
#define BQ27541_REG_TEMP_LOW		0x06
#define BQ27541_REG_TEMP_HIGH		0x07

#define BQ27541_REG_VOLT_LOW		0x08
#define BQ27541_REG_VOLT_HIGH		0x09

#define BQ27541_REG_FLAGS		0x0A

#define BQ27541_REG_NAC_LOW			0x0C
#define BQ27541_REG_NAC_HIGH			0x0D
#define BQ27541_REG_FAC_LOW			0x0E
#define BQ27541_REG_FAC_HIGH			0x0F
#define BQ27541_REG_RM_LOW			0x10
#define BQ27541_REG_RM_HIGH			0x11

//#define BQ27541_REG_FCC			0x12

#define BQ27541_REG_FCC_LOW			0x12
#define BQ27541_REG_FCC_HIGH		0x13

#define BQ27541_REG_AI_LOW			0x14
#define BQ27541_REG_AI_HIGH			0x15
#define BQ27541_REG_TTE			0x16
#define BQ27541_REG_TTF			0x18

//#define BQ27541_REG_SI			0x1a
#define BQ27541_REG_SI_LOW			0x1a
#define BQ27541_REG_SI_HIGH			0x1b

#define BQ27541_REG_STTE		0x1C
#define	BQ27541_REG_MLI			0x1E
#define BQ27541_REG_MLTTE		0x20
#define BQ27541_REG_AE			0x22
#define BQ27541_REG_AP			0x24
#define BQ27541_REG_TTECP		0x26
#define	BQ27541_REG_RSVD		0x28
#define BQ27541_REG_CC			0x2A

#define BQ27541_REG_RSOC_LOW			0x2C /* Relative State-of-Charge */
#define BQ27541_REG_RSOC_HIGH			0x2D /* Relative State-of-Charge */

#define OFFSET_KELVIN_CELSIUS_DECI	2731
#define CURRENT_OVF_THRESHOLD		((1 << 15) - 1)

#define FLAG_BIT_DSG			0
#define FLAG_BIT_SOCF			1
#define FLAG_BIT_SOC1			2
#define FLAG_BIT_BAT_DET		3
#define FLAG_BIT_WAIT_ID		4
#define FLAG_BIT_OCV_GD			5
#define FLAG_BIT_CHG			8
#define FLAG_BIT_FC			9
#define FLAG_BIT_XCHG			10
#define FLAG_BIT_CHG_INH		11
#define FLAG_BIT_OTD			14
#define FLAG_BIT_OTC			15


struct bq27541_irq_info {
    int		irq;
	unsigned int     gpio;
	int     irq_flags;
};

struct bq27541_platform_data {
	struct bq27541_irq_info *irq_info;
	int (*battery_is_online)(void);
	int (*charger_is_online)(void);
	int (*battery_is_charging)(void);
	int (*charger_enable)(void);
};
