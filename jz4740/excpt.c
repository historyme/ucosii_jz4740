/*
 * excpt.c
 *
 * Handle exceptions, dump all generic registers for debug.
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

#include <mipsregs.h>

const static char *regstr[] = {
	"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

void c_except_handler(unsigned int *sp)
{
	int i;
	printf("CAUSE=%08x EPC=%08x\n", read_c0_cause(), read_c0_epc());
	for (i=0;i<32;i++) {
		if (i % 4 == 0)
			printf("\n");
		printf("%4s %08x ", regstr[i], sp[i]);
	}
	printf("\n");
	while(1);
}

