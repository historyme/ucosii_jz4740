/*
 * jz4730.c
 *
 * JZ4730 common routines
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

#define CPU_CLK (CFG_CPU_SPEED)
#define MEM_CLK (CFG_CPU_SPEED / 3)

/*
 * Init SDRAM memory.
 */
void sdram_init(void)
{
	register unsigned int dmcr, sdmode, tmp, ns;
	volatile unsigned int *p;

	unsigned int cas_latency_sdmr[2] = {
		EMC_SDMR_CAS_2,
		EMC_SDMR_CAS_3,
	};

	unsigned int cas_latency_dmcr[2] = {
		1 << EMC_DMCR_TCL_BIT,	/* CAS latency is 2 */
		2 << EMC_DMCR_TCL_BIT	/* CAS latency is 3 */
	};

	REG_EMC_BCR = EMC_BCR_BRE;	/* Enable SPLIT */
	REG_EMC_RTCSR = EMC_RTCSR_CKS_DISABLE;
	REG_EMC_RTCOR = 0;
	REG_EMC_RTCNT = 0;

	/* Basic DMCR register value. */
	dmcr = ((CFG_SDRAM_ROW-11)<<EMC_DMCR_RA_BIT) |
		((CFG_SDRAM_COL-8)<<EMC_DMCR_CA_BIT) |
		(CFG_SDRAM_BANK4<<EMC_DMCR_BA_BIT) |
		(CFG_SDRAM_BW16<<EMC_DMCR_BW_BIT) |
		EMC_DMCR_EPIN |
		cas_latency_dmcr[((CFG_SDRAM_CASL == 3) ? 1 : 0)];

	/* SDRAM timimg parameters */
	ns = 1000000000 / MEM_CLK;

	tmp = CFG_SDRAM_TRAS/ns;
	if (tmp < 4)
		tmp = 4;
	if (tmp > 11)
		tmp = 11;
	dmcr |= ((tmp-4) << EMC_DMCR_TRAS_BIT);
	tmp = CFG_SDRAM_RCD/ns;
	if (tmp > 3)
		tmp = 3;
	dmcr |= (tmp << EMC_DMCR_RCD_BIT);
	tmp = CFG_SDRAM_TPC/ns;
	if (tmp > 7)
		tmp = 7;
	dmcr |= (tmp << EMC_DMCR_TPC_BIT);
	tmp = CFG_SDRAM_TRWL/ns;
	if (tmp > 3)
		tmp = 3;
	dmcr |= (tmp << EMC_DMCR_TRWL_BIT);
	tmp = (CFG_SDRAM_TRAS + CFG_SDRAM_TPC)/ns;
	if (tmp > 14)
		tmp = 14;
	dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

	/* SDRAM mode values */
	sdmode = EMC_SDMR_BT_SEQ | 
		 EMC_SDMR_OM_NORMAL |
		 EMC_SDMR_BL_4 | 
		 cas_latency_sdmr[((CFG_SDRAM_CASL == 3) ? 1 : 0)];

	if (CFG_SDRAM_BW16)
		sdmode <<= 1;
	else
		sdmode <<= 2;

	/* First, precharge phase */
	REG_EMC_DMCR = dmcr;

	/* Set refresh registers */
	tmp = CFG_SDRAM_TREF/ns;
	tmp = tmp/64 + 1;
	if (tmp > 0xff)
		tmp = 0xff;

	REG_EMC_RTCOR = tmp;
	REG_EMC_RTCSR = EMC_RTCSR_CKS_64;	/* Divisor is 64, CKO/64 */

	/* precharge all chip-selects */
	REG8(EMC_SDMR0|sdmode) = 0;
	REG8(EMC_SDMR1|sdmode) = 0;

	/* wait for precharge, > 200us */
	tmp = (CPU_CLK / 1000000) * 200;
	while (tmp--);

	/* enable refresh and set SDRAM mode */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET;

	/* write sdram mode register for each chip-select */
	REG8(EMC_SDMR0|sdmode) = 0;
	REG8(EMC_SDMR1|sdmode) = 0;

	/* everything is ok now */
}

#if  (CFG_SDRAM_ROW == 12)
#define CFG_SDRAM_ROW0 13

void sdram_init0(void)
{
	register unsigned int dmcr, sdmode, tmp, ns;
	volatile unsigned int *p;

	unsigned int cas_latency_sdmr[2] = {
		EMC_SDMR_CAS_2,
		EMC_SDMR_CAS_3,
	};

	unsigned int cas_latency_dmcr[2] = {
		1 << EMC_DMCR_TCL_BIT,	/* CAS latency is 2 */
		2 << EMC_DMCR_TCL_BIT	/* CAS latency is 3 */
	};

	REG_EMC_BCR = EMC_BCR_BRE;	/* Enable SPLIT */
	REG_EMC_RTCSR = EMC_RTCSR_CKS_DISABLE;
	REG_EMC_RTCOR = 0;
	REG_EMC_RTCNT = 0;

	/* Basic DMCR register value. */
	dmcr = ((CFG_SDRAM_ROW0-11)<<EMC_DMCR_RA_BIT) |
		((CFG_SDRAM_COL-8)<<EMC_DMCR_CA_BIT) |
		(CFG_SDRAM_BANK4<<EMC_DMCR_BA_BIT) |
		(CFG_SDRAM_BW16<<EMC_DMCR_BW_BIT) |
		EMC_DMCR_EPIN |
		cas_latency_dmcr[((CFG_SDRAM_CASL == 3) ? 1 : 0)];

	/* SDRAM timimg parameters */
	ns = 1000000000 / MEM_CLK;

	tmp = CFG_SDRAM_TRAS/ns;
	if (tmp < 4)
		tmp = 4;
	if (tmp > 11)
		tmp = 11;
	dmcr |= ((tmp-4) << EMC_DMCR_TRAS_BIT);
	tmp = CFG_SDRAM_RCD/ns;
	if (tmp > 3)
		tmp = 3;
	dmcr |= (tmp << EMC_DMCR_RCD_BIT);
	tmp = CFG_SDRAM_TPC/ns;
	if (tmp > 7)
		tmp = 7;
	dmcr |= (tmp << EMC_DMCR_TPC_BIT);
	tmp = CFG_SDRAM_TRWL/ns;
	if (tmp > 3)
		tmp = 3;
	dmcr |= (tmp << EMC_DMCR_TRWL_BIT);
	tmp = (CFG_SDRAM_TRAS + CFG_SDRAM_TPC)/ns;
	if (tmp > 14)
		tmp = 14;
	dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

	/* SDRAM mode values */
	sdmode = EMC_SDMR_BT_SEQ | 
		 EMC_SDMR_OM_NORMAL |
		 EMC_SDMR_BL_4 | 
		 cas_latency_sdmr[((CFG_SDRAM_CASL == 3) ? 1 : 0)];

	if (CFG_SDRAM_BW16)
		sdmode <<= 1;
	else
		sdmode <<= 2;

	/* First, precharge phase */
	REG_EMC_DMCR = dmcr;

	/* Set refresh registers */
	tmp = CFG_SDRAM_TREF/ns;
	tmp = tmp/64 + 1;
	if (tmp > 0xff)
		tmp = 0xff;

	REG_EMC_RTCOR = tmp;
	REG_EMC_RTCSR = EMC_RTCSR_CKS_64;	/* Divisor is 64, CKO/64 */

	/* precharge all chip-selects */
	REG8(EMC_SDMR0|sdmode) = 0;
	REG8(EMC_SDMR1|sdmode) = 0;

	/* wait for precharge, > 200us */
	tmp = (CPU_CLK / 1000000) * 200;
	while (tmp--);

	/* enable refresh and set SDRAM mode */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET;

	/* write sdram mode register for each chip-select */
	REG8(EMC_SDMR0|sdmode) = 0;
	REG8(EMC_SDMR1|sdmode) = 0;

	/* everything is ok now */
}
#endif
/*
 * Cache Operations
 */
#define Index_Invalidate_I      0x00
#define Index_Writeback_Inv_D   0x01
#define Index_Invalidate_SI     0x02
#define Index_Writeback_Inv_SD  0x03
#define Index_Load_Tag_I	0x04
#define Index_Load_Tag_D	0x05
#define Index_Load_Tag_SI	0x06
#define Index_Load_Tag_SD	0x07
#define Index_Store_Tag_I	0x08
#define Index_Store_Tag_D	0x09
#define Index_Store_Tag_SI	0x0A
#define Index_Store_Tag_SD	0x0B
#define Create_Dirty_Excl_D	0x0d
#define Create_Dirty_Excl_SD	0x0f
#define Hit_Invalidate_I	0x10
#define Hit_Invalidate_D	0x11
#define Hit_Invalidate_SI	0x12
#define Hit_Invalidate_SD	0x13
#define Fill			0x14
#define Hit_Writeback_Inv_D	0x15
					/* 0x16 is unused */
#define Hit_Writeback_Inv_SD	0x17
#define Hit_Writeback_I		0x18
#define Hit_Writeback_D		0x19
					/* 0x1a is unused */
#define Hit_Writeback_SD	0x1b
					/* 0x1c is unused */
					/* 0x1e is unused */
#define Hit_Set_Virtual_SI	0x1e
#define Hit_Set_Virtual_SD	0x1f

#define K0BASE			0x80000000

#define CFG_DCACHE_SIZE		16384
#define CFG_ICACHE_SIZE		16384
#define CFG_CACHELINE_SIZE	32

void flush_icache_all(void)
{
	u32 addr, t = 0;

	asm volatile ("mtc0 $0, $28"); /* Clear Taglo */
	asm volatile ("mtc0 $0, $29"); /* Clear TagHi */

	for (addr = K0BASE; addr < K0BASE + CFG_ICACHE_SIZE;
	     addr += CFG_CACHELINE_SIZE) {
		asm volatile (
			".set mips3\n\t"
			" cache %0, 0(%1)\n\t"
			".set mips2\n\t"
			:
			: "I" (Index_Store_Tag_I), "r"(addr));
	}

	/* invalicate btb */
	asm volatile (
		".set mips32\n\t"
		"mfc0 %0, $16, 7\n\t"
		"nop\n\t"
		"ori %0,2\n\t"
		"mtc0 %0, $16, 7\n\t"
		".set mips2\n\t"
		:
		: "r" (t));
}

void flush_dcache_all(void)
{
	u32 addr;

	for (addr = K0BASE; addr < K0BASE + CFG_DCACHE_SIZE; 
	     addr += CFG_CACHELINE_SIZE) {
		asm volatile (
			".set mips3\n\t"
			" cache %0, 0(%1)\n\t"
			".set mips2\n\t"
			:
			: "I" (Index_Writeback_Inv_D), "r"(addr));
	}

	asm volatile ("sync");
}

void flush_cache_all(void)
{
	flush_dcache_all();
	flush_icache_all();
}
