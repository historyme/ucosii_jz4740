/*
 * linux/arch/mips/jz4740/common/pm.c
 * 
 * JZ4740 Power Management Routines
 * 
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

#include <bsp.h>
#include <jz4740.h>
#include <bsp.h>
#include <jz4740.h>
#include <mipsregs.h>

static int jz_pm_do_hibernate(void)
{
	printf("Put CPU into hibernate mode.\n");
    serial_waitfinish();
	__rtc_clear_hib_stat_all();
	 __rtc_set_scratch_pattern(0x12345678);
	__rtc_enable_alarm_wakeup();
	__rtc_set_hrcr_val(0xfe0);
	__rtc_set_hwfcr_val((100 << 4));
   	__rtc_power_down();
	
	while(1);
        
	return 0;
}

static int jz_pm_do_sleep(void)
{
	//unsigned long imr = REG_INTC_IMR;

	/* Preserve current time */

	/* Mask all interrupts */
//	REG_INTC_IMSR = 0xffffffff;

	/* Just allow next interrupts to wakeup the system.
	 * Note: modify this according to your system.
	 */

	/* Enter SLEEP mode */
	REG_CPM_LCR &= ~CPM_LCR_LPM_MASK;
	REG_CPM_LCR |= CPM_LCR_LPM_SLEEP;

	//REG_CPM_LCR &= ~CPM_LCR_LPM_MASK;
	//REG_CPM_LCR |= CPM_LCR_LPM_IDLE;

	__asm__(".set\tmips3\n\t"
		"wait\n\t"
		".set\tmips0");

	/* Restore to IDLE mode */
	REG_CPM_LCR &= ~CPM_LCR_LPM_MASK;
	REG_CPM_LCR |= CPM_LCR_LPM_IDLE;

	/* Restore interrupts */
	//REG_INTC_IMR = imr;

	/* Restore current time */

	return 0;
}

static int jz_pm_do_pllconvert(unsigned int pllin,int div)
{
	unsigned int cfcr, pllout,sdram_freq;
	unsigned int pll_cfcr,pll_plcr1;
	unsigned int t;
	int div_preq[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};


       if(pllin < 25000000 || pllin > 420000000)
	{
		printf("pll should > 25000000 and < 420000000 ! \n");
		return -1;
	}
    pll_convert(pllin,div,&pll_cfcr,&pll_plcr1);



		sdram_convert(pllin / div_preq[div],&sdram_freq);
	t = read_c0_status();
	write_c0_status(t & (~1));
//	pll_bypass();
#if (DM==1)
            dm_pre_convert();
#endif
//	udelay(1000);

		REG_CPM_CPCCR &= ~CPM_CPCCR_CE;
		REG_CPM_CPCCR = pll_cfcr;
		REG_CPM_CPPCR = pll_plcr1;
		//	while (!(REG_CPM_CPPCR & CPM_CPPCR_PLLS));
		//      REG_CPM_CPPCR &= ~CPM_CPPCR_PLLBP;
              
		REG_EMC_RTCOR = sdram_freq;
		REG_EMC_RTCNT = sdram_freq;
		REG_CPM_CPCCR |= CPM_CPCCR_CE;
		udelay(1000);
		write_c0_status(t);
#if (DM==1)
        dm_all_convert();
#endif

        pllout = (__cpm_get_pllm() + 2)* EXTAL_CLK / (__cpm_get_plln() + 2);
//        printf("pll out new: %d %08x %08x %08x %08x\n",pllout,pll_cfcr,REG_CPM_CPCCR, pll_plcr1,REG_CPM_CPPCR); 
	return 0;
}

static void pll_bypass(void)
{
	unsigned int freq;
         /* sdram convert */
        
        sdram_convert(12000000,&freq);
        REG_EMC_RTCOR = freq;
	REG_EMC_RTCNT = freq;

        REG_CPM_CPPCR |= CPM_CPPCR_PLLBP;
        REG_CPM_CPCCR  = ((REG_CPM_CPCCR & (~0xffff)) | CPM_CPCCR_CE);
	
       	REG_CPM_CPCCR &= ~CPM_CPCCR_CE;

}
static inline int sdram_convert(unsigned int pllin,unsigned int *sdram_freq)
{
	  register unsigned int ns, dmcr,tmp;
 
	ns = 1000000000 / pllin;
	tmp = SDRAM_TRAS/ns;
	if (tmp < 4) tmp = 4;
	if (tmp > 11) tmp = 11;
	dmcr |= ((tmp-4) << EMC_DMCR_TRAS_BIT);

	tmp = SDRAM_RCD/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_RCD_BIT);

	tmp = SDRAM_TPC/ns;
	if (tmp > 7) tmp = 7;
	dmcr |= (tmp << EMC_DMCR_TPC_BIT);

	tmp = SDRAM_TRWL/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_TRWL_BIT);

	tmp = (SDRAM_TRAS + SDRAM_TPC)/ns;
	if (tmp > 14) tmp = 14;
	dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

	/* Set refresh registers */
	tmp = SDRAM_TREF/ns;
	tmp = tmp/64 + 1;
	if (tmp > 0xff) tmp = 0xff;
        *sdram_freq = tmp; 
	//REG_EMC_RTCOR = tmp;
	//REG_EMC_RTCNT = tmp;

	return 0;

}

void pll_convert(unsigned int pllin,int divx,unsigned int *pll_cfcr,unsigned int *pll_plcr1)
{
	register unsigned int cfcr, plcr1;
	int n2FR[33] = {
		0, 0, 1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0,
		7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,
		9
	};
	int div[5] = {0, 3, 3, 3, 3}; /* divisors of I:S:P:L:M */
	div[1] = divx - 1;
	div[2] = divx - 1;
	div[3] = divx - 1;
	div[4] = divx - 1;
//	printf("divx = %d\r\n",divx);
	//int div[5] = {1, 3, 3, 3, 3}; /* divisors of I:S:P:L:M */
	int nf, pllout2;

	cfcr = CPM_CPCCR_CLKOEN |
		(div[0] << CPM_CPCCR_CDIV_BIT) | 
		(div[1] << CPM_CPCCR_HDIV_BIT) | 
		(div[2] << CPM_CPCCR_PDIV_BIT) |
		(div[3] << CPM_CPCCR_MDIV_BIT) |
		(div[4] << CPM_CPCCR_LDIV_BIT);

	pllout2 = (cfcr & CPM_CPCCR_PCS) ? pllin : (pllin / 2);
	/* Init UHC clock */
	//REG_CPM_UHCCDR = pllout2 / 48000000 - 1;

	nf = pllin * 2 / CFG_EXTAL;
	plcr1 = ((nf - 2) << CPM_CPPCR_PLLM_BIT) | /* FD */
		(0 << CPM_CPPCR_PLLN_BIT) |	/* RD=0, NR=2 */
		(0 << CPM_CPPCR_PLLOD_BIT) |    /* OD=0, NO=1 */
		(0xb << CPM_CPPCR_PLLST_BIT) | /* PLL stable time */
		//CPM_CPPCR_PLLEN | CPM_CPPCR_PLLBP;                /* enable PLL */
                CPM_CPPCR_PLLEN;
	/* init PLL */
	*pll_cfcr= cfcr;
	*pll_plcr1=plcr1;
#if 0
//        SetPLL(cfcr,plcr1); 
       	REG_CPM_CPCCR = cfcr;
//	REG_CPM_CPCCR |= CPM_CPCCR_CE;
	REG_CPM_CPPCR = plcr1;
	while (!(REG_CPM_CPPCR & CPM_CPPCR_PLLS));
#endif        
		
}
void SetPLL(unsigned int div,unsigned int pll)
{

       u32 prog_entry = ((u32)SetPLL / 32 - 1) * 32 ;
       u32 prog_size = 1024;
       u32 i;

       for( i = (prog_entry);i < prog_entry + prog_size; i += 32)
		__asm__ __volatile__(
			"cache 0x1c, 0x00(%0)\n\t"
            :
			: "r" (i));
	REG_CPM_CPCCR = div;
//	REG_CPM_CPCCR |= CPM_CPCCR_CE;
	REG_CPM_CPPCR = pll;
	while (!(REG_CPM_CPPCR & CPM_CPPCR_PLLS));

}
/* convert pll while program is running */
int jz_pm_pllconvert(unsigned int pllin,int div)
{
	return jz_pm_do_pllconvert(pllin,div);
}

/* Put CPU to HIBERNATE mode */
int jz_pm_hibernate(void)
{
	return jz_pm_do_hibernate();
}

/* Put CPU to SLEEP mode */
int jz_pm_sleep(void)
{
	return jz_pm_do_sleep();
}

/* Put CPU to IDLE mode */
void jz_pm_idle(void)
{
		//cpu_wait();
}
