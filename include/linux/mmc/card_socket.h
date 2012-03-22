/*
 *  linux/include/linux/mmc/card_socket.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef LINUX_MMC_CARD_SOCKET_H
#define LINUX_MMC_CARD_SOCKET_H

#include <linux/switch.h>

#define TWL4030_GPIO0_IRQ	384
#define TWL4030_GPIO1_IRQ	385
#define TWL4030_GPIO1     193

struct sim_detect_dev {
	struct delayed_work	sim_detect_delayed_work;
	struct switch_dev sdev;
};

extern int g_sim_carddetect_status;
extern struct sim_detect_dev	*the_sim_detect;
extern void update_sim_card_status(struct work_struct *work);
extern void sim_card_detect_change(struct mmc_host *host, unsigned long delay);
extern void update_sim_card_switch_state(void);

#endif
