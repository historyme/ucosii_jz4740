/*
 * init.c
 *
 * Perform the early initialization. Include CP0.status, install exception
 * handlers, fill all interrupt entries.
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
 /*
#pragma	weak	pcm_init
void pcm_init(void) {}
*/
#include <archdefs.h>
#include <mipsregs.h>

unsigned int g_stack[2049];

extern void except_common_entry(void);

void CONSOL_SendCh(unsigned char ch)
{
	serial_putc(ch);
}

void CONSOL_GetChar(unsigned char *ch)
{
	int r;
	r = serial_getc();
	if (r > 0)
		*ch = (unsigned char)r;
	else
		*ch = 0;
}

typedef void (*pfunc)(void);

extern pfunc __CTOR_LIST__[];
extern pfunc __CTOR_END__[];

void c_main(void)
{
	pfunc *p;
	write_c0_status(0x10000400);

	memcpy((void *)A_K0BASE, except_common_entry, 0x20);
	memcpy((void *)(A_K0BASE + 0x180), except_common_entry, 0x20);
	memcpy((void *)(A_K0BASE + 0x200), except_common_entry, 0x20);

	__dcache_writeback_all();
	__icache_invalidate_all();

	intc_init();
	detect_clock();
	gpio_init();
	serial_init();
	OSInit();

	/* Invoke constroctor functions when needed. */
	for (p=&__CTOR_END__[-1]; p >= __CTOR_LIST__; p--)
		(*p)();

	APP_vMain();
	while(1);
}

