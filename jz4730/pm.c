/*
 * pm.c
 *
 * This file implements the Jz47xx CPU clock change and suspend/resume.
 *
 * Author: Wei Jianli <jlwei@ingenic.cn>
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc. All rights reserved.
 */

#include <regs.h>
#include <ops.h>
#include <clock.h>

//=======================================================================
//
// CPU clock change local variables
//
//=======================================================================

static unsigned long boot_rtcor_val;
static unsigned long boot_mclk_val;

#define PLL_WAIT_500NS (500*(1000000000/__cpm_get_iclk()))

//==========================================================================
//
// CPU suspend / resume local routines
//
//==========================================================================

#define SAVE(x,s)	sleep_save[SLEEP_SAVE_##x] = REG##s(x)
#define RESTORE(x,s)	REG##s(x) = sleep_save[SLEEP_SAVE_##x]

/*
 * List of global jz47xx peripheral registers to preserve.
 * More ones like core register and general purpose register values 
 * are preserved with the stack pointer in sleep.S.
 */
enum {	SLEEP_SAVE_START = 0,

	/* OST */
	SLEEP_SAVE_OST_TER,
	SLEEP_SAVE_OST_TCSR0, SLEEP_SAVE_OST_TCSR1, SLEEP_SAVE_OST_TCSR2,
	SLEEP_SAVE_OST_TRDR0, SLEEP_SAVE_OST_TRDR1, SLEEP_SAVE_OST_TRDR2,
	SLEEP_SAVE_OST_TCNT0, SLEEP_SAVE_OST_TCNT1, SLEEP_SAVE_OST_TCNT2,

	/* HARB */
	SLEEP_SAVE_HARB_HAPOR, SLEEP_SAVE_HARB_HMCTR, SLEEP_SAVE_HARB_HMLTR,

	/* GPIO */
	SLEEP_SAVE_GPIO_GPDR0, SLEEP_SAVE_GPIO_GPDR1, SLEEP_SAVE_GPIO_GPDR2, SLEEP_SAVE_GPIO_GPDR3,
	SLEEP_SAVE_GPIO_GPDIR0, SLEEP_SAVE_GPIO_GPDIR1, SLEEP_SAVE_GPIO_GPDIR2,	SLEEP_SAVE_GPIO_GPDIR3,
	SLEEP_SAVE_GPIO_GPODR0, SLEEP_SAVE_GPIO_GPODR1, SLEEP_SAVE_GPIO_GPODR2,	SLEEP_SAVE_GPIO_GPODR3,
	SLEEP_SAVE_GPIO_GPPUR0, SLEEP_SAVE_GPIO_GPPUR1, SLEEP_SAVE_GPIO_GPPUR2, SLEEP_SAVE_GPIO_GPPUR3,
	SLEEP_SAVE_GPIO_GPALR0, SLEEP_SAVE_GPIO_GPALR1, SLEEP_SAVE_GPIO_GPALR2,	SLEEP_SAVE_GPIO_GPALR3,
	SLEEP_SAVE_GPIO_GPAUR0, SLEEP_SAVE_GPIO_GPAUR1, SLEEP_SAVE_GPIO_GPAUR2,	SLEEP_SAVE_GPIO_GPAUR3,
	SLEEP_SAVE_GPIO_GPIDLR0, SLEEP_SAVE_GPIO_GPIDLR1, SLEEP_SAVE_GPIO_GPIDLR2, SLEEP_SAVE_GPIO_GPIDLR3,
	SLEEP_SAVE_GPIO_GPIDUR0, SLEEP_SAVE_GPIO_GPIDUR1, SLEEP_SAVE_GPIO_GPIDUR2, SLEEP_SAVE_GPIO_GPIDUR3,
	SLEEP_SAVE_GPIO_GPIER0, SLEEP_SAVE_GPIO_GPIER1, SLEEP_SAVE_GPIO_GPIER2,	SLEEP_SAVE_GPIO_GPIER3,
	SLEEP_SAVE_GPIO_GPIMR0, SLEEP_SAVE_GPIO_GPIMR1, SLEEP_SAVE_GPIO_GPIMR2, SLEEP_SAVE_GPIO_GPIMR3,
	SLEEP_SAVE_GPIO_GPFR0, SLEEP_SAVE_GPIO_GPFR1, SLEEP_SAVE_GPIO_GPFR2, SLEEP_SAVE_GPIO_GPFR3,

	/* UART(0-3) */
	SLEEP_SAVE_UART0_IER, SLEEP_SAVE_UART0_LCR, SLEEP_SAVE_UART0_MCR, SLEEP_SAVE_UART0_SPR, SLEEP_SAVE_UART0_DLLR, SLEEP_SAVE_UART0_DLHR,
	SLEEP_SAVE_UART1_IER, SLEEP_SAVE_UART1_LCR, SLEEP_SAVE_UART1_MCR, SLEEP_SAVE_UART1_SPR, SLEEP_SAVE_UART1_DLLR, SLEEP_SAVE_UART1_DLHR,
	SLEEP_SAVE_UART2_IER, SLEEP_SAVE_UART2_LCR, SLEEP_SAVE_UART2_MCR, SLEEP_SAVE_UART2_SPR, SLEEP_SAVE_UART2_DLLR, SLEEP_SAVE_UART2_DLHR,
	SLEEP_SAVE_UART3_IER, SLEEP_SAVE_UART3_LCR, SLEEP_SAVE_UART3_MCR, SLEEP_SAVE_UART3_SPR, SLEEP_SAVE_UART3_DLLR, SLEEP_SAVE_UART3_DLHR,

	/* DMAC */
	SLEEP_SAVE_DMAC_DMACR,
	SLEEP_SAVE_DMAC_DSAR0, SLEEP_SAVE_DMAC_DSAR1, SLEEP_SAVE_DMAC_DSAR2, SLEEP_SAVE_DMAC_DSAR3, SLEEP_SAVE_DMAC_DSAR4, SLEEP_SAVE_DMAC_DSAR5, SLEEP_SAVE_DMAC_DSAR6, SLEEP_SAVE_DMAC_DSAR7,
	SLEEP_SAVE_DMAC_DDAR0, SLEEP_SAVE_DMAC_DDAR1, SLEEP_SAVE_DMAC_DDAR2, SLEEP_SAVE_DMAC_DDAR3, SLEEP_SAVE_DMAC_DDAR4, SLEEP_SAVE_DMAC_DDAR5, SLEEP_SAVE_DMAC_DDAR6, SLEEP_SAVE_DMAC_DDAR7,
	SLEEP_SAVE_DMAC_DTCR0, SLEEP_SAVE_DMAC_DTCR1, SLEEP_SAVE_DMAC_DTCR2, SLEEP_SAVE_DMAC_DTCR3, SLEEP_SAVE_DMAC_DTCR4, SLEEP_SAVE_DMAC_DTCR5, SLEEP_SAVE_DMAC_DTCR6, SLEEP_SAVE_DMAC_DTCR7,
	SLEEP_SAVE_DMAC_DRSR0, SLEEP_SAVE_DMAC_DRSR1, SLEEP_SAVE_DMAC_DRSR2, SLEEP_SAVE_DMAC_DRSR3, SLEEP_SAVE_DMAC_DRSR4, SLEEP_SAVE_DMAC_DRSR5, SLEEP_SAVE_DMAC_DRSR6, SLEEP_SAVE_DMAC_DRSR7,
	SLEEP_SAVE_DMAC_DCCSR0, SLEEP_SAVE_DMAC_DCCSR1, SLEEP_SAVE_DMAC_DCCSR2, SLEEP_SAVE_DMAC_DCCSR3, SLEEP_SAVE_DMAC_DCCSR4, SLEEP_SAVE_DMAC_DCCSR5, SLEEP_SAVE_DMAC_DCCSR6, SLEEP_SAVE_DMAC_DCCSR7,

	/* INTC */
	SLEEP_SAVE_INTC_IMR, 

	/* Checksum */
	SLEEP_SAVE_CKSUM,

	SLEEP_SAVE_SIZE
};

/***********************************************************************
 * void pm_wakeup_setup(void)
 *
 * This is called to setup the wakeup sources (such as key input etc.) 
 * before suspending the CPU.
 *
 ***********************************************************************/
static void pm_wakeup_setup(void)
{
	REG_CPM_WER = 0;  /* Clear all wakeup sources first */

	/* Set the GPIO/CS/PCMCIA pins state in hibernate mode */

	/* Put next pins to there proper states during HIBERNATE
	 *
	 * GP66(PW_O) = high 
	 * GP94(PWM0) = low
	 */
	__cpm_set_pin(66);
	__cpm_clear_pin(94);

	/* Enable GP97(PW_I) wakeup function  */
	__gpio_as_input(97);
	REG_CPM_WER |= 1 << 1;
	REG_CPM_WRER |= 1 << 1;
	REG_CPM_WFER |= 1 << 1;
	__intc_unmask_irq(IRQ_GPIO3); /* enable INTC irq */
}

/***********************************************************************
 * void pm_cpu_suspend_enter(void)
 *
 * This is called to suspend all other devices before suspending the CPU.
 *
 ***********************************************************************/
#define MMC_POWER_PIN 91   /* Pin to enable/disable card power */

#define MMC_POWER_OFF()				\
do {						\
      	__gpio_set_pin(MMC_POWER_PIN);		\
} while (0)

static void pm_cpu_suspend_enter(void)
{
	MMC_POWER_OFF();                 /* Disable power to MMC/SD card */
}

/***********************************************************************
 * void pm_cpu_suspend_exit(void)
 *
 * This is called to resume all other devices after resuming.
 *
 ***********************************************************************/
static void pm_cpu_suspend_exit(void)
{
	REG_EMC_NFCSR |= EMC_NFCSR_NFE;  /* Enable NAND flash */
#if MMC
	MMC_Initialize();                /* ReInitialize the MMC/SD card */
#endif 
#if CODECTYPE > 0
	pcm_init();   /* ReInitialize the audio system */
#endif
}

/***********************************************************************
 * void do_pm_cpu_suspend(void)
 *
 * This is called internally to do the CPU suspend.
 *
 ***********************************************************************/

extern void jz_cpu_suspend(void);
extern void jz_cpu_resume(void);

#define PHYS(addr) 	((unsigned long)(addr) & ~0xE0000000)

static void do_pm_cpu_suspend(void)
{
	unsigned long sleep_save[SLEEP_SAVE_SIZE];
	unsigned long delta;
	unsigned long checksum = 0;
	int i;

	REG_CPM_OCR |= CPM_OCR_SUSPEND_PHY0; /* suspend USB PHY 0 */
	REG_CPM_OCR |= CPM_OCR_SUSPEND_PHY1; /* suspend USB PHY 1 */
	REG_CPM_OCR |= CPM_OCR_EXT_RTC_CLK;  /* select the external RTC clock (32.768KHz) */

	/*
	 * Temporary solution.  This won't be necessary once
	 * we move jz support into the serial driver.
	 * Save the on-chip UART
	 */
	SAVE(UART0_LCR, 8); SAVE(UART0_MCR, 8); SAVE(UART0_SPR, 8);
	REG8(UART0_LCR) |= UARTLCR_DLAB; /* Access to DLLR/DLHR */
	SAVE(UART0_DLLR, 8); SAVE(UART0_DLHR, 8);
	REG8(UART0_LCR) &= ~UARTLCR_DLAB; /* Access to IER */
	SAVE(UART0_IER, 8);

	SAVE(UART1_LCR, 8); SAVE(UART1_MCR, 8); SAVE(UART1_SPR, 8);
	REG8(UART1_LCR) |= UARTLCR_DLAB; /* Access to DLLR/DLHR */
	SAVE(UART1_DLLR, 8); SAVE(UART1_DLHR, 8);
	REG8(UART1_LCR) &= ~UARTLCR_DLAB; /* Access to IER */
	SAVE(UART1_IER, 8);

	SAVE(UART2_LCR, 8); SAVE(UART2_MCR, 8); SAVE(UART2_SPR, 8);
	REG8(UART2_LCR) |= UARTLCR_DLAB; /* Access to DLLR/DLHR */
	SAVE(UART2_DLLR, 8); SAVE(UART2_DLHR, 8);
	REG8(UART2_LCR) &= ~UARTLCR_DLAB; /* Access to IER */
	SAVE(UART2_IER, 8);

	SAVE(UART3_LCR, 8); SAVE(UART3_MCR, 8); SAVE(UART3_SPR, 8);
	REG8(UART3_LCR) |= UARTLCR_DLAB; /* Access to DLLR/DLHR */
	SAVE(UART3_DLLR, 8); SAVE(UART3_DLHR, 8);
	REG8(UART3_LCR) &= ~UARTLCR_DLAB; /* Access to IER */
	SAVE(UART3_IER, 8);

	/* Save vital registers */

	SAVE(OST_TER, 8);
	SAVE(OST_TCSR0, 16); SAVE(OST_TCSR1, 16); SAVE(OST_TCSR2, 16);
	SAVE(OST_TRDR0, 32); SAVE(OST_TRDR1, 32); SAVE(OST_TRDR2, 32);
	SAVE(OST_TCNT0, 32); SAVE(OST_TCNT1, 32); SAVE(OST_TCNT2, 32);

	SAVE(HARB_HAPOR, 32); SAVE(HARB_HMCTR, 32); SAVE(HARB_HMLTR, 32);

	SAVE(GPIO_GPDR0, 32); SAVE(GPIO_GPDR1, 32); SAVE(GPIO_GPDR2, 32); 
	SAVE(GPIO_GPDR3, 32);
	SAVE(GPIO_GPDIR0, 32); SAVE(GPIO_GPDIR1, 32); SAVE(GPIO_GPDIR2, 32); 
	SAVE(GPIO_GPDIR3, 32);
	SAVE(GPIO_GPODR0, 32); SAVE(GPIO_GPODR1, 32); SAVE(GPIO_GPODR2, 32); 
	SAVE(GPIO_GPODR3, 32);
	SAVE(GPIO_GPPUR0, 32); SAVE(GPIO_GPPUR1, 32); SAVE(GPIO_GPPUR2, 32); 
	SAVE(GPIO_GPPUR3, 32);
	SAVE(GPIO_GPALR0, 32); SAVE(GPIO_GPALR1, 32); SAVE(GPIO_GPALR2, 32); 
	SAVE(GPIO_GPALR3, 32);
	SAVE(GPIO_GPAUR0, 32); SAVE(GPIO_GPAUR1, 32); SAVE(GPIO_GPAUR2, 32); 
	SAVE(GPIO_GPAUR3, 32);
	SAVE(GPIO_GPIDLR0, 32); SAVE(GPIO_GPIDLR1, 32);	SAVE(GPIO_GPIDLR2, 32); 
	SAVE(GPIO_GPIDLR3, 32);
	SAVE(GPIO_GPIDUR0, 32);	SAVE(GPIO_GPIDUR1, 32);	SAVE(GPIO_GPIDUR2, 32);	
	SAVE(GPIO_GPIDUR3, 32);
	SAVE(GPIO_GPIER0, 32); SAVE(GPIO_GPIER1, 32); SAVE(GPIO_GPIER2, 32); 
	SAVE(GPIO_GPIER3, 32);
	SAVE(GPIO_GPIMR0, 32); SAVE(GPIO_GPIMR1, 32); SAVE(GPIO_GPIMR2, 32); 
	SAVE(GPIO_GPIMR3, 32);
	SAVE(GPIO_GPFR0, 32); SAVE(GPIO_GPFR1, 32); SAVE(GPIO_GPFR2, 32); 
	SAVE(GPIO_GPFR3, 32);

	SAVE(DMAC_DMACR, 32);
	SAVE(DMAC_DSAR0, 32); SAVE(DMAC_DSAR1, 32); SAVE(DMAC_DSAR2, 32); SAVE(DMAC_DSAR3, 32); SAVE(DMAC_DSAR4, 32); SAVE(DMAC_DSAR5, 32); SAVE(DMAC_DSAR6, 32); SAVE(DMAC_DSAR7, 32); 
	SAVE(DMAC_DDAR0, 32); SAVE(DMAC_DDAR1, 32); SAVE(DMAC_DDAR2, 32); SAVE(DMAC_DDAR3, 32); SAVE(DMAC_DDAR4, 32); SAVE(DMAC_DDAR5, 32); SAVE(DMAC_DDAR6, 32); SAVE(DMAC_DDAR7, 32); 
	SAVE(DMAC_DTCR0, 32); SAVE(DMAC_DTCR1, 32); SAVE(DMAC_DTCR2, 32); SAVE(DMAC_DTCR3, 32); SAVE(DMAC_DTCR4, 32); SAVE(DMAC_DTCR5, 32); SAVE(DMAC_DTCR6, 32); SAVE(DMAC_DTCR7, 32); 
	SAVE(DMAC_DRSR0, 32); SAVE(DMAC_DRSR1, 32); SAVE(DMAC_DRSR2, 32); SAVE(DMAC_DRSR3, 32); SAVE(DMAC_DRSR4, 32); SAVE(DMAC_DRSR5, 32); SAVE(DMAC_DRSR6, 32); SAVE(DMAC_DRSR7, 32); 
	SAVE(DMAC_DCCSR0, 32); SAVE(DMAC_DCCSR1, 32); SAVE(DMAC_DCCSR2, 32); SAVE(DMAC_DCCSR3, 32); SAVE(DMAC_DCCSR4, 32); SAVE(DMAC_DCCSR5, 32); SAVE(DMAC_DCCSR6, 32); SAVE(DMAC_DCCSR7, 32);

	SAVE(INTC_IMR, 32);
	REG32(INTC_IMSR) = 0xffffffff; /* Mask all intrs */

	pm_wakeup_setup();             /* setup wakeup sources */

	/* Clear previous reset status */
	REG_CPM_RSTR &= ~(CPM_RSTR_HR | CPM_RSTR_WR | CPM_RSTR_SR);

	/* Set resume return address */
	REG_CPM_SPR = PHYS(jz_cpu_resume);

	/* Before sleeping, calculate and save a checksum */
	for (i = 0; i < SLEEP_SAVE_SIZE - 1; i++)
		checksum += sleep_save[i];
	sleep_save[SLEEP_SAVE_CKSUM] = checksum;

	/* *** go zzz *** */
	jz_cpu_suspend();

	/* after sleeping, validate the checksum */
	checksum = 0;
	for (i = 0; i < SLEEP_SAVE_SIZE - 1; i++)
		checksum += sleep_save[i];

	/* if invalid, display message and wait for a hardware reset */
	if (checksum != sleep_save[SLEEP_SAVE_CKSUM]) {
		/** Add platform-specific message display codes here **/
		while (1);
	}

	/* Ensure not to come back here if it wasn't intended */
	REG_CPM_SPR = 0;

	/* Restore registers */

	RESTORE(GPIO_GPDR0, 32); RESTORE(GPIO_GPDR1, 32); RESTORE(GPIO_GPDR2, 32); 
	RESTORE(GPIO_GPDR3, 32);
	RESTORE(GPIO_GPDIR0, 32); RESTORE(GPIO_GPDIR1, 32); RESTORE(GPIO_GPDIR2, 32); 
	RESTORE(GPIO_GPDIR3, 32);
	RESTORE(GPIO_GPODR0, 32); RESTORE(GPIO_GPODR1, 32); RESTORE(GPIO_GPODR2, 32); 
	RESTORE(GPIO_GPODR3, 32);
	RESTORE(GPIO_GPPUR0, 32); RESTORE(GPIO_GPPUR1, 32); RESTORE(GPIO_GPPUR2, 32); 
	RESTORE(GPIO_GPPUR3, 32);
	RESTORE(GPIO_GPALR0, 32); RESTORE(GPIO_GPALR1, 32); RESTORE(GPIO_GPALR2, 32); 
	RESTORE(GPIO_GPALR3, 32);
	RESTORE(GPIO_GPAUR0, 32); RESTORE(GPIO_GPAUR1, 32); RESTORE(GPIO_GPAUR2, 32); 
	RESTORE(GPIO_GPAUR3, 32);
	RESTORE(GPIO_GPIDLR0, 32);RESTORE(GPIO_GPIDLR1, 32);RESTORE(GPIO_GPIDLR2, 32);
	RESTORE(GPIO_GPIDLR3, 32);
	RESTORE(GPIO_GPIDUR0, 32);RESTORE(GPIO_GPIDUR1, 32);RESTORE(GPIO_GPIDUR2, 32); 
	RESTORE(GPIO_GPIDUR3, 32);
	RESTORE(GPIO_GPIER0, 32); RESTORE(GPIO_GPIER1, 32); RESTORE(GPIO_GPIER2, 32);
	RESTORE(GPIO_GPIER3, 32);
	RESTORE(GPIO_GPIMR0, 32); RESTORE(GPIO_GPIMR1, 32); RESTORE(GPIO_GPIMR2, 32); 
	RESTORE(GPIO_GPIMR3, 32);
	RESTORE(GPIO_GPFR0, 32); RESTORE(GPIO_GPFR1, 32); RESTORE(GPIO_GPFR2, 32); 
	RESTORE(GPIO_GPFR3, 32);

	RESTORE(HARB_HAPOR, 32); RESTORE(HARB_HMCTR, 32); RESTORE(HARB_HMLTR, 32);

	RESTORE(OST_TCNT0, 32);	RESTORE(OST_TCNT1, 32);	RESTORE(OST_TCNT2, 32);
	RESTORE(OST_TRDR0, 32);	RESTORE(OST_TRDR1, 32);	RESTORE(OST_TRDR2, 32);
	RESTORE(OST_TCSR0, 16);	RESTORE(OST_TCSR1, 16);	RESTORE(OST_TCSR2, 16);
	RESTORE(OST_TER, 8);

	RESTORE(DMAC_DMACR, 32);
	RESTORE(DMAC_DSAR0, 32); RESTORE(DMAC_DSAR1, 32); RESTORE(DMAC_DSAR2, 32); RESTORE(DMAC_DSAR3, 32); RESTORE(DMAC_DSAR4, 32); RESTORE(DMAC_DSAR5, 32); RESTORE(DMAC_DSAR6, 32); RESTORE(DMAC_DSAR7, 32); 
	RESTORE(DMAC_DDAR0, 32); RESTORE(DMAC_DDAR1, 32); RESTORE(DMAC_DDAR2, 32); RESTORE(DMAC_DDAR3, 32); RESTORE(DMAC_DDAR4, 32); RESTORE(DMAC_DDAR5, 32); RESTORE(DMAC_DDAR6, 32); RESTORE(DMAC_DDAR7, 32); 
	RESTORE(DMAC_DTCR0, 32); RESTORE(DMAC_DTCR1, 32); RESTORE(DMAC_DTCR2, 32); RESTORE(DMAC_DTCR3, 32); RESTORE(DMAC_DTCR4, 32); RESTORE(DMAC_DTCR5, 32); RESTORE(DMAC_DTCR6, 32); RESTORE(DMAC_DTCR7, 32); 
	RESTORE(DMAC_DRSR0, 32); RESTORE(DMAC_DRSR1, 32); RESTORE(DMAC_DRSR2, 32); RESTORE(DMAC_DRSR3, 32); RESTORE(DMAC_DRSR4, 32); RESTORE(DMAC_DRSR5, 32); RESTORE(DMAC_DRSR6, 32); RESTORE(DMAC_DRSR7, 32); 
	RESTORE(DMAC_DCCSR0, 32); RESTORE(DMAC_DCCSR1, 32); RESTORE(DMAC_DCCSR2, 32); RESTORE(DMAC_DCCSR3, 32); RESTORE(DMAC_DCCSR4, 32); RESTORE(DMAC_DCCSR5, 32); RESTORE(DMAC_DCCSR6, 32); RESTORE(DMAC_DCCSR7, 32);

	RESTORE(INTC_IMR, 32);

	/*
	 * Temporary solution.  This won't be necessary once
	 * we move jz support into the serial driver.
	 * Restore the on-chip UART.
	 */

	/* FIFO control reg, write-only */
	REG8(UART0_FCR) = UARTFCR_FE | UARTFCR_RFLS | UARTFCR_TFLS | UARTFCR_UUE;
	REG8(UART1_FCR) = UARTFCR_FE | UARTFCR_RFLS | UARTFCR_TFLS | UARTFCR_UUE;
	REG8(UART2_FCR) = UARTFCR_FE | UARTFCR_RFLS | UARTFCR_TFLS | UARTFCR_UUE;
	REG8(UART3_FCR) = UARTFCR_FE | UARTFCR_RFLS | UARTFCR_TFLS | UARTFCR_UUE;
 
	REG8(UART0_LCR) |= UARTLCR_DLAB; /* Access to DLLR/DLHR */
	RESTORE(UART0_DLLR, 8);	RESTORE(UART0_DLHR, 8);
	REG8(UART0_LCR) &= ~UARTLCR_DLAB; /* Access to IER */
	RESTORE(UART0_IER, 8);
	RESTORE(UART0_MCR, 8); RESTORE(UART0_SPR, 8); RESTORE(UART0_LCR, 8);

	REG8(UART1_LCR) |= UARTLCR_DLAB; /* Access to DLLR/DLHR */
	RESTORE(UART1_DLLR, 8);	RESTORE(UART1_DLHR, 8);
	REG8(UART1_LCR) &= ~UARTLCR_DLAB; /* Access to IER */
	RESTORE(UART1_IER, 8);
	RESTORE(UART1_MCR, 8); RESTORE(UART1_SPR, 8); RESTORE(UART1_LCR, 8);

	REG8(UART2_LCR) |= UARTLCR_DLAB; /* Access to DLLR/DLHR */
	RESTORE(UART2_DLLR, 8);	RESTORE(UART2_DLHR, 8);
	REG8(UART2_LCR) &= ~UARTLCR_DLAB; /* Access to IER */
	RESTORE(UART2_IER, 8);
	RESTORE(UART2_MCR, 8); RESTORE(UART2_SPR, 8); RESTORE(UART2_LCR, 8);

	REG8(UART3_LCR) |= UARTLCR_DLAB; /* Access to DLLR/DLHR */
	RESTORE(UART3_DLLR, 8);	RESTORE(UART3_DLHR, 8);
	REG8(UART3_LCR) &= ~UARTLCR_DLAB; /* Access to IER */
	RESTORE(UART3_IER, 8);
	RESTORE(UART3_MCR, 8); RESTORE(UART3_SPR, 8); RESTORE(UART3_LCR, 8);

	REG_CPM_OCR &= ~CPM_OCR_SUSPEND_PHY0; /* resume USB PHY 0 */
	REG_CPM_OCR &= ~CPM_OCR_SUSPEND_PHY1; /* resume USB PHY 1 */
	REG_CPM_OCR &= ~CPM_OCR_EXT_RTC_CLK;  /* reuse internal RTC clock (3686400/128=28.8KHz) */
}

//=======================================================================
//
// Global Routines
//
//=======================================================================


/*******************************************************************************
** Name:	  void pm_cpu_clock(void)
** Function:      Change the CPU clock during programs are running.
**                We just allow to change the four clocks: ICLK, SCLK, MCLK, PCLK.
**                Other clocks such as LCD_CLK, LCD_PIXEL_CLK are unchanged.
**
** Note 1:        We are assuming that the boot clocks are the max values, then
**                other program calls this function to scale down the four clocks.
**                PLL output frequency does not change in this implementation.
**
** Note 2:        The four divisors must be confirmed to the constraint conditions
**                as mentioned in the section << Clock and Power Management>> of
**                the jz4730 spec.
**
** Note 3:        Valid values for xxx_div have: 1, 2, 3, 4, 6, 8, 12, 16, 24, 32.
**
** PLL_OUT_FREQ = 3686400 * <NF> / (<NR> * <NO>)  -- PLL clock
** ICLK = PLL_OUT_FREQ / <ICLK_DIV>     -- CPU clock
** SCLK = PLL_OUT_FREQ / <SCLK_DIV>     -- System bus clock
** MCLK = PLL_OUT_FREQ / <MCLK_DIV>     -- External memory clock
** PCLK = PLL_OUT_FREQ / <PCLK_DIV>     -- Peripheral bus clock
**
*******************************************************************************/
void pm_cpu_clock(int iclk_div, int sclk_div, int mclk_div, int pclk_div)
{
	int div2val[] = {-1, 0, 1, 2, 3, -1, 4, -1,
			 5, -1, -1, -1, 6, -1, -1, -1,
			 7, -1, -1, -1, -1, -1, -1, -1,
			 8, -1, -1, -1, -1, -1, -1, -1,
			 9};
	unsigned int cfcr;
	unsigned int cur_mclk_val, new_mclk_val, new_rtcor_val;
	unsigned int tmp = 0, wait = PLL_WAIT_500NS; 
	static int first = 1;

	if (first) {
		first = 0;
		boot_rtcor_val = REG_EMC_RTCOR;
		boot_mclk_val = __cpm_get_mclk();
	}

	/*
	 * Calc the new value of CFCR register
	 */
	cfcr = REG_CPM_CFCR;  /* current value of CFCR register */
	cfcr &= ~(CPM_CFCR_IFR_MASK | CPM_CFCR_SFR_MASK | CPM_CFCR_MFR_MASK | CPM_CFCR_PFR_MASK);
	cfcr |= (div2val[iclk_div] << CPM_CFCR_IFR_BIT) | (div2val[sclk_div] << CPM_CFCR_SFR_BIT) | (div2val[mclk_div] << CPM_CFCR_MFR_BIT) | (div2val[pclk_div] << CPM_CFCR_PFR_BIT);
	cfcr |= CPM_CFCR_UPE;       /* update immediately */

	/*
	 * Update some SDRAM parameters.
	 * Here we need to update the refresh constant.
	 */
	cur_mclk_val = __cpm_get_mclk();
	new_mclk_val = __cpm_get_pllout() / mclk_div;

	new_rtcor_val = boot_rtcor_val * new_mclk_val / boot_mclk_val;
	if (new_rtcor_val < 1) new_rtcor_val = 1;
	if (new_rtcor_val > 255) new_rtcor_val = 255;

	/* 
	 * Stop module clocks here ..
	 */
	__cpm_stop_aic(1);
	__cpm_stop_aic(2);

	/* We're going SLOWER: first update RTCOR value
	 * before changing the frequency.
	 */
	if (new_mclk_val < cur_mclk_val) {
		REG_EMC_RTCOR = new_rtcor_val;
		REG_EMC_RTCNT = new_rtcor_val;
	}

	/* update register to change the clocks.
	 * align this code to a cache line.
	 */
	__asm__ __volatile__(
		".set noreorder\n\t"
		".align 5\n"
		"sw %1,0(%0)\n\t"
		"li %3,0\n\t"
		"1:\n\t"
		"bne %3,%2,1b\n\t"
		"addi %3, 1\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		".set reorder\n\t"
		:
		: "r" (CPM_CFCR), "r" (cfcr), "r" (wait), "r" (tmp));

	/* We're going FASTER, so update RTCOR
	 * after changing the frequency 
	 */
	if (new_mclk_val > cur_mclk_val) {
		REG_EMC_RTCOR = new_rtcor_val;
		REG_EMC_RTCNT = new_rtcor_val;
	}

	/* 
	 * Restart module clocks here ..
	 */
 	__cpm_start_aic(1);
	__cpm_start_aic(2);

#if 0
	printf("freqs: cpu_clk=%dMHz sys_clk=%dMHz mem_clk=%dMHz per_clk=%dMHz\n",
	       __cpm_get_iclk()/1000000, __cpm_get_sclk()/1000000, 
	       __cpm_get_mclk()/1000000, __cpm_get_pclk()/1000000);
#endif
}

/***********************************************************************
 * void pm_cpu_suspend(void)
 *
 * This is called externally to do the CPU suspend.
 *
 ***********************************************************************/
void pm_cpu_suspend(void)
{
	pm_cpu_suspend_enter();

	do_pm_cpu_suspend();

	pm_cpu_suspend_exit();
}
