/*
 * dma.c
 *
 * Handle all DMA relative operations.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Corp.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <regs.h>
#include <ops.h>

static unsigned int dma_mode[NUM_DMA], dma_unit_size[NUM_DMA], dma_irq[NUM_DMA];
static int inited = 0;
extern void dma_stop(int);

void dma_start(int ch, unsigned int srcAddr, unsigned int dstAddr,
	       unsigned int count)
{
	//dma_stop
	dma_stop(ch);
	//set_dma_addr
	REG_DMAC_DSAR(ch) = srcAddr;
	REG_DMAC_DDAR(ch) = dstAddr;
	//set_dma_count
	REG_DMAC_DTCR(ch) = count / dma_unit_size[ch];
	//enable_dma
	REG_DMAC_DCCSR(ch) = dma_mode[ch];
	REG_DMAC_DCCSR(ch) &= ~(DMAC_DCCSR_HLT|DMAC_DCCSR_TC|DMAC_DCCSR_AR);
	__dmac_enable_channel(ch);
	if (dma_irq[ch])
		__dmac_channel_enable_irq(ch);
}

void dma_stop(int ch)
{
	int i;
#define DMA_DISABLE_POLL	0x5000
	if (!__dmac_channel_enabled(ch))
		return;
	for (i=0;i<DMA_DISABLE_POLL;i++)
		if (__dmac_channel_transmit_end_detected(ch))
			break;
	__dmac_disable_channel(ch);
	if (dma_irq[ch])
		__dmac_channel_disable_irq(ch);
}

void dma_request(int ch, void (*irq_handler)(unsigned int), unsigned int arg,
		 unsigned int mode, unsigned int type)
{
	if (!inited) {
		inited = 1;
		__dmac_enable_all_channels();
	}

	dma_mode[ch] = mode;
	REG_DMAC_DRSR(ch) = type;
	if (irq_handler) {
		request_irq(IRQ_DMA_0 + ch, irq_handler, arg);
		dma_irq[ch] = 1;
	}
}

void dma_block_size(int ch, int nbyte)
{
	dma_mode[ch] &= ~DMAC_DCCSR_DS_MASK;
	dma_unit_size[ch] = nbyte;
	switch (nbyte) {
	case 1:
		dma_mode[ch] |= DMAC_DCCSR_DS_8b;
		break;
	case 2:
		dma_mode[ch] |= DMAC_DCCSR_DS_16b;
		break;
	case 4:
		dma_mode[ch] |= DMAC_DCCSR_DS_32b;
		break;
	case 16:
		dma_mode[ch] |= DMAC_DCCSR_DS_16B;
		break;
	case 32:
		dma_mode[ch] |= DMAC_DCCSR_DS_32B;
		break;
	}
}

void dma_dest_size(int ch, int nbit)
{
	dma_mode[ch] &= ~DMAC_DCCSR_DWDH_MASK;
	switch (nbit) {
	case 8:
		dma_mode[ch] |= DMAC_DCCSR_DWDH_8;
		break;
	case 16:
		dma_mode[ch] |= DMAC_DCCSR_DWDH_16;
		break;
	case 32:
		dma_mode[ch] |= DMAC_DCCSR_DWDH_32;
		break;
	}
}

void dma_src_size(int ch, int nbit)
{
	dma_mode[ch] &= ~DMAC_DCCSR_SWDH_MASK;
	switch (nbit) {
	case 8:
		dma_mode[ch] |= DMAC_DCCSR_SWDH_8;
		break;
	case 16:
		dma_mode[ch] |= DMAC_DCCSR_SWDH_16;
		break;
	case 32:
		dma_mode[ch] |= DMAC_DCCSR_SWDH_32;
		break;
	}
}

