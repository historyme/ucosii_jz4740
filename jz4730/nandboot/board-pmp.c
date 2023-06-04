/*
 * board-pmp.c
 *
 * JZ4730-based PMP board routines.
 *
 * Copyright (c) 2005-2007  Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "config.h"
#include "jz4730.h"

#ifdef CFG_JZ_PMP

void gpio_init(void)
{
	__harb_usb0_uhc();
	__gpio_as_emc();
	__gpio_as_uart0();
	__gpio_as_uart3();
}

void pll_init(void)
{
	unsigned int nf, plcr1;

	nf = CFG_CPU_SPEED * 2 / CFG_EXTAL;
	plcr1 = ((nf-2) << CPM_PLCR1_PLL1FD_BIT) |
		(0 << CPM_PLCR1_PLL1RD_BIT) |	/* RD=0, NR=2, 1.8432 = 3.6864/2 */
		(0 << CPM_PLCR1_PLL1OD_BIT) |   /* OD=0, NO=1 */
		(0x20 << CPM_PLCR1_PLL1ST_BIT) | /* PLL stable time */
		CPM_PLCR1_PLL1EN;                /* enable PLL */          

	/* Clock divisors.
	 * 
	 * CFCR values: when CPM_CFCR_UCS(bit 28) is set, select external USB clock.
	 *
	 * 0x10411110 -> 1:2:2:2:2
	 * 0x10422220 -> 1:3:3:3:3
	 * 0x10433330 -> 1:4:4:4:4
	 * 0x10444440 -> 1:6:6:6:6
	 * 0x10455550 -> 1:8:8:8:8
	 * 0x10466660 -> 1:12:12:12:12
	 */
	REG_CPM_CFCR = 0x00422220 | (((CFG_CPU_SPEED/48000000) - 1) << 25);

	/* PLL out frequency */
	REG_CPM_PLCR1 = plcr1;
}

#endif /* CFG_JZ_PMP */
