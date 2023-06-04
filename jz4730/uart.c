/*
 * uart.c
 *
 * Simple UART console implemented.
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

#ifndef u8
#define u8 unsigned char
#endif

#ifndef u16
#define u16 unsigned short
#endif

#ifndef u32
#define u32 unsigned int
#endif

#undef UART_BASE
#define UART_BASE  UART3_BASE

#define DEV_CLK    12000000
#define BAUDRATE   57600

void serial_setbrg (void)
{
	volatile u8 *uart_lcr = (volatile u8 *)(UART_BASE + OFF_LCR);
	volatile u8 *uart_dlhr = (volatile u8 *)(UART_BASE + OFF_DLHR);
	volatile u8 *uart_dllr = (volatile u8 *)(UART_BASE + OFF_DLLR);
	u32 baud_div, tmp;

	baud_div = DEV_CLK / 16 / BAUDRATE;
	tmp = *uart_lcr;
	tmp |= UARTLCR_DLAB;
	*uart_lcr = tmp;

	*uart_dlhr = (baud_div >> 8) & 0xff;
	*uart_dllr = baud_div & 0xff;

	tmp &= ~UARTLCR_DLAB;
	*uart_lcr = tmp;
}

int serial_init (void)
{
	volatile u8 *uart_fcr = (volatile u8 *)(UART_BASE + OFF_FCR);
	volatile u8 *uart_lcr = (volatile u8 *)(UART_BASE + OFF_LCR);
	volatile u8 *uart_ier = (volatile u8 *)(UART_BASE + OFF_IER);
	volatile u8 *uart_sircr = (volatile u8 *)(UART_BASE + OFF_SIRCR);

	/* Disable port interrupts while changing hardware */
	*uart_ier = 0;

	/* Disable UART unit function */
	*uart_fcr = ~UARTFCR_UUE;

	/* Set both receiver and transmitter in UART mode (not SIR) */
	*uart_sircr = ~(SIRCR_RSIRE | SIRCR_TSIRE);

	/* Set databits, stopbits and parity. (8-bit data, 1 stopbit, no parity) */
	*uart_lcr = UARTLCR_WLEN_8;// | UARTLCR_STOP_1;

	/* Set baud rate */
	serial_setbrg();

	/* Enable UART unit, enable and clear FIFO */
	*uart_fcr = UARTFCR_UUE | UARTFCR_FE | UARTFCR_TFLS | UARTFCR_RFLS;

	return 0;
}

void serial_putc (const char c)
{
	volatile u8 *uart_lsr = (volatile u8 *)(UART_BASE + OFF_LSR);
	volatile u8 *uart_tdr = (volatile u8 *)(UART_BASE + OFF_TDR);

	if (c == '\n') serial_putc ('\r');

	/* Wait for fifo to shift out some bytes */
	while ( !((*uart_lsr & (UARTLSR_TDRQ | UARTLSR_TEMT)) == 0x60) );

	*uart_tdr = (u8)c;
}

void serial_puts (const char *s)
{
	while (*s)
		serial_putc (*s++);
}

int serial_tstc (void)
{
	volatile u8 *uart_lsr = (volatile u8 *)(UART_BASE + OFF_LSR);

	if (*uart_lsr & UARTLSR_DR)
		/* Data in rfifo */
		return (1);

	return 0;
}

int serial_getc (void)
{
	volatile u8 *uart_rdr = (volatile u8 *)(UART_BASE + OFF_RDR);

	while (!serial_tstc());

	return *uart_rdr;
}

