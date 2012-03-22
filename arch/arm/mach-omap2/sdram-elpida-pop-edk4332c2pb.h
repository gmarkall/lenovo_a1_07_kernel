/*
 * SDRC register values for the ELPIDA EDK4332C2PB (4G bits DDR Mobile RAM PoP)
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARCH_ARM_MACH_OMAP2_LPDDR1_ELPIDA_EDK4332C2PB
#define ARCH_ARM_MACH_OMAP2_LPDDR1_ELPIDA_EDK4332C2PB

#include <plat/sdrc.h>

/* ELPIDA EDK4332C2PB */
/* XXX Using ARE = 0x1 (no autorefresh burst) -- can this be changed? */
static struct omap_sdrc_params edk4332C2pb_sdrc_params[] = {
	[0] = {
		.rate        = 200000000,
		.actim_ctrla = 0xa2e1b4c6,
		.actim_ctrlb = 0x00022118,
		.rfr_ctrl    = 0x0005e601,    
		.mr          = 0x00000032,
	},
	[1] = {
		.rate        = 166000000,
		.actim_ctrla = 0x829db4c6,
		.actim_ctrlb = 0x00012114,
		.rfr_ctrl    = 0x0004e201,
		.mr          = 0x00000032,
	},
	[2] = {
		.rate        = 100000000,
		.actim_ctrla = 0x51912283,
		.actim_ctrlb = 0x0002210c,
		.rfr_ctrl    = 0x0002da01,
		.mr          = 0x00000022,
	},
	[3] = {
		.rate        = 83000000,
		.actim_ctrla = 0x51512283,
		.actim_ctrlb = 0x0001210a,
		.rfr_ctrl    = 0x00025801,
		.mr          = 0x00000022,
	},
	[4] = {
		.rate        = 0
	},
};

#endif
