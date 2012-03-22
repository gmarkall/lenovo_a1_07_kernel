/*
 * Support for TI max8903 (bqTINY-II) Dual Input (USB/AC Adpater)
 * 1-Cell Li-Ion Charger connected via GPIOs.
 *
 * Copyright (c) 2008 Philipp Zabel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

 struct regulator_init_data;

/**
 * max8903_mach_info - platform data for max8903
 * @gpio_nce: GPIO line connected to the nCE pin, used to enable / disable
 * charging
 * @gpio_iset2: GPIO line connected to the ISET2 pin, used to limit charging
 * current to 100 mA / 500 mA
 */
#define MAX8903_IRQ_NUM   3
	
enum max8903_pin_define {
	MAX8903_PIN_FLAUTn = 0,
	MAX8903_PIN_CHARHEn,
	MAX8903_PIN_DCOKn,
};

struct max8903_irq_info {
    int		irq;
	unsigned int     gpio;
	int     gpio_state;
};

struct max8903_mach_info {
	int gpio_cen;
	int gpio_dcm;
	int gpio_iusb;
	int gpio_cen_state;
	int gpio_dcm_state;
	int gpio_iusb_state;
	struct max8903_irq_info irq_info[MAX8903_IRQ_NUM];
	struct regulator_init_data *init_data;
};
