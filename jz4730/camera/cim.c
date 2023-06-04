/*
 * ucosii/jz4730/drv/cim.c
 *
 * Camera Interface Module (CIM) driver for JzSOC
 * This driver is independent of the camera sensor
 *
 * Copyright (C) 2005  JunZheng semiconductor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h" //ucosii

#include "regs.h"
#include "ops.h"  
#include "clock.h"  //for __cpm_get_sclk()

#include "camera.h" // Select one of the sensors

#ifndef u8
#define u8	unsigned char
#endif

#ifndef u16
#define u16	unsigned short
#endif

#ifndef u32
#define u32	unsigned int
#endif

#ifndef NULL
#define NULL	0
#endif

#define CIM_NAME        "cim"

/*
 * Define the Max Image Size
 */
#define MAX_IMAGE_WIDTH  640
#define MAX_IMAGE_HEIGHT 480
#define MAX_IMAGE_BPP    16  
#define MAX_FRAME_SIZE   (MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * MAX_IMAGE_BPP / 8)

typedef struct
{
	u32 width;
	u32 height;
	u32 bpp;
} img_param_t;

typedef struct
{
	u32 cfg;
	u32 ctrl;
	u32 mclk;
} cim_config_t;

/*
 * CIM configuration.
 */

struct cim_config {
	int mclk;/* Master clock output to the sensor */
	int vsp; /* VSYNC Polarity:0-rising edge is active,1-falling edge is active */
	int hsp; /* HSYNC Polarity:0-rising edge is active,1-falling edge is active */
	int pcp; /* PCLK Polarity:0-rising edge sampling,1-falling edge sampling */
	int dsm; /* Data Sampling Mode:0-CCIR656 Progressive Mode, 1-CCIR656 Interlace Mode,2-Gated Clock Mode,3-Non-Gated Clock Mode */
	int pack; /* Data Packing Mode: suppose received data sequence were 0x11 0x22 0x33 0x44, then packing mode will rearrange the sequence in the RXFIFO:
		     0: 0x11 0x22 0x33 0x44
		     1: 0x22 0x33 0x44 0x11
		     2: 0x33 0x44 0x11 0x22
		     3: 0x44 0x11 0x22 0x33
		     4: 0x44 0x33 0x22 0x11
		     5: 0x33 0x22 0x11 0x44
		     6: 0x22 0x11 0x44 0x33
		     7: 0x11 0x44 0x33 0x22
		  */
	int inv_dat;    /* Inverse Input Data Enable */
	int frc;        /* Frame Rate Control:0-15 */
	int trig;   /* RXFIFO Trigger: 
		       0: 4
		       1: 8
		       2: 12
		       3: 16
		       4: 20
		       5: 24
		       6: 28
		       7: 32
		    */
};

static struct cim_config cim_cfg = {
#ifdef CONFIG_OV7660
        24000000, 1, 0, 0, 2, 2, 0, 0, 1,
#endif
#ifdef CONFIG_KSMOV7649
	24000000, 0, 0, 0, 0, 0, 0, 0, 3,
#endif
#ifdef CONFIG_HV7131
	24000000, 0, 0, 1, 2, 2, 0, 0, 1,
#endif
};

/* Actual image size, must less than max values */
static int img_width = IMG_WIDTH, img_height = IMG_HEIGHT, img_bpp = IMG_BPP;

/*
 * CIM DMA descriptor
 */
struct cim_desc {
	u32 nextdesc;   /* Physical address of next desc */
	u32 framebuf;   /* Physical address of frame buffer */
	u32 frameid;    /* Frame ID */ 
	u32 dmacmd;     /* DMA command */
};

/*
 * CIM device structure
 */
struct cim_device {
	unsigned char *framebuf;
	unsigned int frame_size;
  //	unsigned int page_order;
  //	wait_queue_head_t wait_queue;
        OS_EVENT *WaitSem;
	struct cim_desc frame_desc __attribute__ ((aligned (16)));
};

// global
static struct cim_device *cim_dev;

/*==========================================================================
 * CIM init routines
 *========================================================================*/

static void cim_config(cim_config_t *c)
{
	REG_CIM_CFG = c->cfg;
	REG_CIM_CTRL = c->ctrl;

	// Set the master clock output
	__cim_set_master_clk(__cpm_get_sclk(), c->mclk);

	// Enable sof, eof and stop interrupts
	__cim_enable_sof_intr();
	__cim_enable_eof_intr();
	__cim_enable_stop_intr();
}

/*==========================================================================
 * CIM start/stop operations
 *========================================================================*/

static int cim_start_dma(unsigned char *ubuf)
{
        u8 err;

	__cim_disable();
	//	dma_cache_wback((unsigned long)cim_dev->framebuf, (2 ^ (cim_dev->page_order)) * 4096);
	__dcache_writeback_all();  //?
	// set the desc addr
	__cim_set_da(PHYS(&(cim_dev->frame_desc)));

	__cim_clear_state();	// clear state register
	__cim_reset_rxfifo();	// resetting rxfifo
	__cim_unreset_rxfifo();
	__cim_enable_dma();	// enable dma
	//printf("2cim dam in.\n");
	// start
	__cim_enable();

	// wait for interrupts
	//	interruptible_sleep_on(&cim_dev->wait_queue);
	OSSemPend(cim_dev->WaitSem, OS_TICKS_PER_SEC, &err);
	//printf("3cim dam in.\n");
	// copy frame data to user buffer
	memcpy(ubuf, cim_dev->framebuf, cim_dev->frame_size);

	return cim_dev->frame_size;
}

static void cim_stop(void)
{
	__cim_disable();
	__cim_clear_state();
}

/*==========================================================================
 * Framebuffer allocation and destroy
 *========================================================================*/

static void cim_fb_destroy(void)
{
	if (cim_dev->framebuf) {
	  cim_dev->framebuf = NULL;
	}
}

static int cim_fb_alloc(void)
{
  static u8 cim_heap[IMG_WIDTH * IMG_HEIGHT * IMG_BPP / 8 + 0x10];
  cim_dev->frame_size = img_width * img_height * (img_bpp/8);

	cim_dev->framebuf = (u8 *)(((u32)cim_heap+0x10) & ~0xf);

	if ( !(cim_dev->framebuf) ) {
		return -1;
	}

	cim_dev->frame_desc.nextdesc = PHYS(&(cim_dev->frame_desc));
	cim_dev->frame_desc.framebuf = PHYS(cim_dev->framebuf);
	cim_dev->frame_desc.frameid = 0x52052018;
	cim_dev->frame_desc.dmacmd = CIM_CMD_EOFINT | CIM_CMD_STOP | (cim_dev->frame_size >> 2); // stop after capturing a frame

	//	dma_cache_wback((unsigned long)(&(cim_dev->frame_desc)), 16);
	__dcache_writeback_all();  //?
	return 0;
}

/*==========================================================================
 * File operations
 *========================================================================*/

int cim_read(unsigned char *buf)
{
  return cim_start_dma(buf);
}

void cim_close(void)
{
        cim_stop();  //
  	free_irq(IRQ_CIM);
	cim_fb_destroy();
}


/*==========================================================================
 * Interrupt handler
 *========================================================================*/

static void cim_irq_handler(unsigned int arg)
{
	u32 state = REG_CIM_STATE;
#if 0
	if (state & CIM_STATE_DMA_EOF) {
	  //		wake_up_interruptible(&cim_dev->wait_queue);
	  OSSemPost(cim_dev->WaitSem);
	}
#endif
	if (state & CIM_STATE_DMA_STOP) {
		// Got a frame, wake up wait routine
	  //		wake_up_interruptible(&cim_dev->wait_queue);
	  // printf("irq \n");
	  OSSemPost(cim_dev->WaitSem);
	}

	// clear status flags
	REG_CIM_STATE = 0;
}

/*==========================================================================
 * Module init and exit
 *========================================================================*/

int cim_init(void)
{
        static struct cim_device *dev;
	int ret;
	cim_config_t c;
	static struct cim_device dev0;

	/* config cim */
	c.mclk = cim_cfg.mclk;
	c.cfg = (cim_cfg.vsp << 14) | (cim_cfg.hsp << 13) | (cim_cfg.pcp << 12) | (cim_cfg.dsm << 0) | (cim_cfg.pack << 4) | (cim_cfg.inv_dat << 15);
	c.ctrl = (cim_cfg.frc << 16) | (cim_cfg.trig << 4);
	cim_config(&c);

	/* cim image parameter isn't set newly */

	/* allocate device */
        dev = &dev0;
	/* record device */
	cim_dev = dev;

	/* allocate a frame buffer */
	if (cim_fb_alloc() < 0) {
		return -1;
	}

	//	init_waitqueue_head(&dev->wait_queue);
	dev->WaitSem = OSSemCreate(0);

	if ((ret = request_irq(IRQ_CIM, cim_irq_handler, dev)) == -1) {
		cim_fb_destroy();
		printf("CIM could not get IRQ");
		return ret;
	}

	//	printf("JzSOC Camera Interface Module (CIM) driver OK\n");

	return 0;
}

