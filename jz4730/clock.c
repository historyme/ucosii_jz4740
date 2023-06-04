/*
 * clock.c
 *
 * Detect PLL out clk frequency and implement a udelay routine.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <regs.h>
#include <ops.h>

#define EXTAL_CLK      12000000

const static int FR2n[] = {
	1, 2, 3, 4, 6, 8, 12, 16, 24, 32
};

static unsigned int iclk;
 
void detect_clock(void)
{
	unsigned int cfcr, plcr1, pllout;

	cfcr = REG_CPM_CFCR;
	plcr1 = REG_CPM_PLCR1;

	pllout = ((((plcr1 & CPM_PLCR1_PLL1FD_MASK) >> CPM_PLCR1_PLL1FD_BIT) + 2) * EXTAL_CLK) / (((plcr1 & CPM_PLCR1_PLL1RD_MASK) >> CPM_PLCR1_PLL1RD_BIT) + 2);
	iclk = pllout / FR2n[(cfcr & CPM_CFCR_IFR_MASK) >> CPM_CFCR_IFR_BIT];
}

void udelay(unsigned int usec)
{
	unsigned int i = usec * (iclk / 2000000);
	__asm__ __volatile__ (
		"\t.set noreorder\n"
		"1:\n\t"
		"bne\t%0, $0, 1b\n\t"
		"addi\t%0, %0, -1\n\t"
		".set reorder\n"
		: "=r" (i)
		: "0" (i)
	);
}
