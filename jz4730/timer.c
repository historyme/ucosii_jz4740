/*
 * timer.c
 *
 * Perform the system ticks.
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

#define EXTAL_CLK	12000000

void timerHander(unsigned int arg)
{
	OSTimeTick();
}

void JZ_StartTicker(unsigned int tps)
{
	unsigned int latch;

	request_irq(IRQ_OST0,timerHander,0);
	latch = (EXTAL_CLK + (tps>>1)) / tps;
	__ost_set_mode(0, OST_TCSR_UIE | OST_TCSR_CKS_EXTAL);
	__ost_set_reload(0, latch);
	__ost_set_count(0, latch);
	__ost_enable_channel(0);
}

void JZ_StopTicker(void)
{
	__ost_disable_channel(0);
	__ost_clear_uf(0);
}

unsigned int jz_timer_h = 0; 
void JZ_timerHander(unsigned int arg)
{
	__ost_clear_uf(1);
	jz_timer_h++;
	
}

void JZ_StartTimer()
{
	
	unsigned int tps = 1;
	unsigned int latch;
	__ost_disable_channel(1);
	request_irq(IRQ_OST1,JZ_timerHander,0);
	latch = (EXTAL_CLK + (tps >> 1)) / tps;
	__ost_set_mode(1, OST_TCSR_UIE | OST_TCSR_CKS_EXTAL);
	__ost_set_reload(1, latch);
	__ost_set_count(1, latch);
	__ost_enable_channel(1);
	
}

void JZ_StopTimer(void)
{
	__ost_disable_channel(1);
	__ost_clear_uf(1);
}
unsigned int *JZ_DiffTimer(unsigned int *tm3, unsigned int *tm1,unsigned int *tm2)
{
	unsigned int d1,d2,d3,d4;
	d1 = *tm1;
	d2 = *tm2;
	d3 = *(tm1 + 1);
	d4 = *(tm2 + 1);
	if(d1 > d2)
	{
		*tm3 = d1 - d2;
		*(tm3 + 1) = d3 - d4;
		return tm1;
	}else
	{
		*tm3 = 1000000 + d1 - d2;
		*(tm3 + 1) = d3 - d4 - 1;
		return tm1;
	}
	return 0;

}
void JZ_GetTimer(unsigned int *tm)
{
        unsigned int dh,dl;
	dh = jz_timer_h;
	dl = __ost_get_count(1);
	if(dl == -1)
	{
		if(dh != jz_timer_h)
		{
                   	dh = jz_timer_h;
			dl = __ost_get_count(1);
		}
	}
        
        dl = (EXTAL_CLK - dl) / (EXTAL_CLK / 1000000);
       
	*(tm++) = dl;
	*(tm++) = dh;
	
}
