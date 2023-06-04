/*
 * i2s.c
 *
 * JzSOC On-Chip I2S audio driver.
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

#include <includes.h>
#include <regs.h>
#include <ops.h>
#include <clock.h>
#include "pcm.h"

#define CONFIG_I2S_TSC2301

#ifndef u8
#define u8 unsigned char
#endif
#ifndef u16
#define u16 unsigned short
#endif
#ifndef u32
#define u32 unsigned int
#endif

#define AUDIO_READ_DMA	0
#define AUDIO_WRITE_DMA	1

#ifdef PHYADDR
#undef PHYADDR
#endif
#define PHYADDR(n)	((n) & 0x1fffffff)
#ifdef KSEG1ADDR
#undef KSEG1ADDR
#endif
#define	KSEG1ADDR(n)	(PHYADDR(n) | 0xa0000000)

#ifdef PAGE_SIZE
#undef PAGE_SIZE
#endif
#define PAGE_SIZE	0x1000

extern void printf(const char *fmt, ...);

#define STANDARD_SPEED  48000
#define SYNC_CLK	48000

#define MAX_RETRY       100

static unsigned long i2s_clk;

static int		jz_audio_rate;
static char		jz_audio_format;
static char		jz_audio_channels;

static void jz_update_filler(int bits, int channels);

static void jz_i2s_replay_dma_irq(unsigned int);
static void jz_i2s_record_dma_irq(unsigned int);

static void (*replay_filler)(unsigned long src_start, int count, int id);
static int (*record_filler)(unsigned long dst_start, int count, int id);

#define QUEUE_MAX 2

typedef struct buffer_queue_s {
	int count;
	int id[QUEUE_MAX];
	int lock;
} buffer_queue_t;

static unsigned long out_dma_buf[QUEUE_MAX+1];
static unsigned long out_dma_pbuf[QUEUE_MAX+1];
static unsigned long out_dma_buf_data_count[QUEUE_MAX+1];
static unsigned long in_dma_buf[QUEUE_MAX+1];
static unsigned long in_dma_pbuf[QUEUE_MAX+1];
static unsigned long in_dma_buf_data_count[QUEUE_MAX+1];

static buffer_queue_t out_empty_queue;
static buffer_queue_t out_full_queue;
static buffer_queue_t out_busy_queue;

static buffer_queue_t in_empty_queue;
static buffer_queue_t in_full_queue;
static buffer_queue_t in_busy_queue;

static int first_record_call = 0;

static OS_EVENT *tx_sem;
static OS_EVENT *rx_sem;

static inline int get_buffer_id(struct buffer_queue_s *q)
{
	int r;
	unsigned long flags;
	int i;
	flags = spin_lock_irqsave();
	if (q->count == 0) return -1;
	r = q->id[0];
	for (i=0;i < q->count-1;i++)
		q->id[i] = q->id[i+1];
	q->count --;
	spin_unlock_irqrestore(flags);
	return r;
}

static inline void put_buffer_id(struct buffer_queue_s *q, int id)
{
	unsigned long flags;
	flags = spin_lock_irqsave();
	q->id[q->count] = id;
	q->count ++;
	spin_unlock_irqrestore(flags);
}

static  inline int elements_in_queue(struct buffer_queue_s *q)
{
	int r;
	unsigned long flags;
	flags=spin_lock_irqsave();
	r = q->count;
	spin_unlock_irqrestore(flags);
	return r;
}

/****************************************************************************
 * Architecture related routines
 ****************************************************************************/
static void jz_i2s_record_dma_irq (unsigned int arg)
{
	int dma = AUDIO_READ_DMA;
	int id1, id2;

	dma_stop(dma);

	if (__dmac_channel_address_error_detected(dma)) {
		printf("%s: DMAC address error.\n", __FUNCTION__);
		__dmac_channel_clear_address_error(dma);
	}
	if (__dmac_channel_transmit_end_detected(dma)) {
		__dmac_channel_clear_transmit_end(dma);

		id1 = get_buffer_id(&in_busy_queue);
		put_buffer_id(&in_full_queue, id1);
//		__dcache_invalidate_all();

		OSSemPost(rx_sem);

		if ((id2 = get_buffer_id(&in_empty_queue)) >= 0) {
			put_buffer_id(&in_busy_queue, id2);

			in_dma_buf_data_count[id2] = in_dma_buf_data_count[id1];

			dma_start(dma, PHYADDR(AIC_DR), in_dma_pbuf[id2],
				  in_dma_buf_data_count[id2]);
		} else
			in_busy_queue.count = 0;
	}
}

static void jz_i2s_replay_dma_irq (unsigned int arg)
{
	int dma = AUDIO_WRITE_DMA;
	int id;
	dma_stop(dma);
	if (__dmac_channel_address_error_detected(dma)) {
		printf("%s: DMAC address error.\n", __FUNCTION__);
		__dmac_channel_clear_address_error(dma);
	}

	if (__dmac_channel_transmit_end_detected(dma)) {
		__dmac_channel_clear_transmit_end(dma);
		if ((id = get_buffer_id(&out_busy_queue)) < 0)
			printf("Strange DMA finish interrupt for AIC module\n");

		put_buffer_id(&out_empty_queue, id);
		if ((id = get_buffer_id(&out_full_queue)) >= 0) {
			put_buffer_id(&out_busy_queue, id);

			dma_start(dma, out_dma_pbuf[id], PHYADDR(AIC_DR),
				  out_dma_buf_data_count[id]);

		} else
			out_busy_queue.count = 0;

		if (elements_in_queue(&out_empty_queue) > 0)
			OSSemPost(tx_sem);
	}
}

/*
 * Initialize the onchip I2S controller
 */
static void jz_i2s_initHw(void)
{
	i2s_clk = __cpm_get_i2sclk();

	__i2s_disable();
	__i2s_as_master();
	__i2s_set_sample_rate(i2s_clk, SYNC_CLK);
	__i2s_enable();
	__i2s_reset();
	/* wait for a long time to let ac97 controller reset completely,
	 * otherwise, registers except ACFR will be clear by reset, can't be
	 * set correctly.
	 */
	udelay(160);
	__i2s_reset_codec();
	__i2s_set_sample_size(16);

	__i2s_disable_record();
	__i2s_disable_replay();
	__i2s_disable_loopback();

	__i2s_set_transmit_trigger(7);
	__i2s_set_receive_trigger(7);

}


static int jz_audio_set_speed(int rate)
{
/* 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 99999999 ? */
	if (rate > 48000)
		rate = 48000;
	if (rate < 8000)
		rate = 8000;
	jz_audio_rate = rate;

	i2s_codec_set_samplerate(rate);
	return rate;
}

static int record_fill_1x8_u(unsigned long dst_start, int count, int id)
{
	int cnt = 0;
	unsigned char data;
	volatile unsigned char *s = (unsigned char *)(in_dma_buf[id]);
	volatile unsigned char *dp = (unsigned char *)dst_start;

	while (count > 0) {
		count -= 2;		/* count in dword */
		cnt++;
		*dp++ = *s++ + 0x80;
		s++;			/* skip other channel */
	}

	return cnt;
}

static int record_fill_2x8_u(unsigned long dst_start, int count, int id)
{
	int cnt = 0;
	volatile unsigned char *s = (unsigned char *)(in_dma_buf[id]);
	volatile unsigned char *dp = (unsigned char *)dst_start;

	while (count > 0) {
		count -= 2;
		cnt += 2;
		*dp++ = *s++ + 0x80;
		*dp++ = *s++ + 0x80;
	}

	return cnt;
}

static int record_fill_1x16_s(unsigned long dst_start, int count, int id)
{
	int cnt = 0;
	unsigned short *s = (unsigned short *)(in_dma_buf[id]);
	unsigned short *dp = (unsigned short *)dst_start;

	while (count > 0) {
		count -= 2;		/* count in dword */
		cnt += 2;		/* count in byte */
		*dp++ = *s++;
		s++;			/* skip other channel */
	}
	return cnt;
}

static int record_fill_2x16_s(unsigned long dst_start, int count, int id)
{
	memcpy(dst_start, in_dma_buf[id], count);
	return count;
}


static void replay_fill_1x8_u(unsigned long src_start, int count, int id)
{
	int cnt = 0;
	unsigned char data;
	unsigned char *s = (unsigned char *)src_start;
	unsigned short *dp = (unsigned short *)(out_dma_buf[id]);

	while (count > 0) {
		count--;
		cnt += 2;
		data = *s++ - 0x80;
		*dp++ = (data << 8) | data;
	}
	out_dma_buf_data_count[id] = cnt;
}

static void replay_fill_2x8_u(unsigned long src_start, int count, int id)
{
	memcpy(out_dma_buf[id], src_start, count);
	out_dma_buf_data_count[id] = count;
}

static void replay_fill_1x16_s(unsigned long src_start, int count, int id)
{
	int i, cnt = 0;
	unsigned short d1;
	unsigned short *s = (unsigned short *)src_start;
	unsigned int *dp = (unsigned int *)(out_dma_buf[id]);

	while (count > 0) {
		count -= 2;
		cnt += 4;
		d1 = *s++;
		*dp++ = (d1 << 16) | d1;
	}
	out_dma_buf_data_count[id] = cnt;
}

static void replay_fill_2x16_s(unsigned long src_start, int count, int id)
{
	memcpy(out_dma_buf[id], src_start, count);
	out_dma_buf_data_count[id] = count;
}

static unsigned int jz_audio_set_format(unsigned int fmt)
{
	switch (fmt) {
	        case AFMT_U8:
		case AFMT_S16_LE:
			jz_audio_format = fmt;
			jz_update_filler(fmt, jz_audio_channels);
		break;
	}
	return jz_audio_format;
}

static short jz_audio_set_channels(short channels)
{
	switch (channels) {
	case 1:
		i2s_codec_set_channel(1);
		jz_audio_channels = channels;
		jz_update_filler(jz_audio_format, jz_audio_channels);
		break;
	case 2:
		jz_audio_channels = channels;
		jz_update_filler(jz_audio_format, jz_audio_channels);
		break;
	case 0:
		break;
	}
	return jz_audio_channels;
}


static void jz_audio_reset(void)
{
	int i;

	__i2s_disable_replay();
	__i2s_disable_receive_dma();
	__i2s_disable_record();
	__i2s_disable_transmit_dma();

	in_empty_queue.count = QUEUE_MAX;
	out_empty_queue.count = QUEUE_MAX;

	for (i=0;i<QUEUE_MAX;i++) {
		in_empty_queue.id[i] = i;
		out_empty_queue.id[i] = i;
	}

	in_busy_queue.count = 0;
	in_full_queue.count = 0;
	out_busy_queue.count = 0;
	out_full_queue.count = 0;
}


/* I2S codec initialisation. */
static int jz_i2s_codec_init(void)
{
	i2s_codec_init();
	return 0;
}

static void jz_update_filler(int format, int channels)
{
#define TYPE(fmt,ch) (((fmt)<<3) | ((ch)&7))	/* up to 8 chans supported. */


	switch (TYPE(format, channels)) {
	default:

	case TYPE(AFMT_U8, 1):
		__aic_disable_mono2stereo();
		__aic_disable_unsignadj();
		dma_block_size(AUDIO_READ_DMA, 4);
		dma_block_size(AUDIO_WRITE_DMA, 4);
		replay_filler = replay_fill_1x8_u;
		record_filler = record_fill_1x8_u;
		__ac97_set_oass(8);
		__ac97_set_iass(8);
		dma_src_size(AUDIO_READ_DMA, 8);
		dma_dest_size(AUDIO_WRITE_DMA, 8);
		break;
	case TYPE(AFMT_U8, 2):
		__aic_disable_mono2stereo();
		__aic_disable_unsignadj();
		dma_block_size(AUDIO_READ_DMA, 4);
		dma_block_size(AUDIO_WRITE_DMA, 4);
		replay_filler = replay_fill_2x8_u;
		record_filler = record_fill_2x8_u;
		__ac97_set_oass(8);
		__ac97_set_iass(8);
		dma_src_size(AUDIO_READ_DMA, 8);
		dma_dest_size(AUDIO_WRITE_DMA, 8);
		break;
	case TYPE(AFMT_S16_LE, 1):
		dma_block_size(AUDIO_READ_DMA, 4);
		dma_block_size(AUDIO_WRITE_DMA, 4);
		__aic_disable_unsignadj();
		__aic_disable_mono2stereo();
		replay_filler = replay_fill_1x16_s;
		record_filler = record_fill_1x16_s;
		__ac97_set_oass(16);
		__ac97_set_iass(16);
		dma_src_size(AUDIO_READ_DMA, 16);
		dma_dest_size(AUDIO_WRITE_DMA, 16);

		break;

	case TYPE(AFMT_S16_LE, 2):
		dma_block_size(AUDIO_READ_DMA, 4);
		dma_block_size(AUDIO_WRITE_DMA, 4);
		__aic_disable_mono2stereo();
		__aic_disable_unsignadj();
		replay_filler = replay_fill_2x16_s;
		record_filler = record_fill_2x16_s;
		__ac97_set_oass(16);
		__ac97_set_iass(16);
		dma_src_size(AUDIO_READ_DMA, 16);
		dma_dest_size(AUDIO_WRITE_DMA, 16);
		break;
	}
}

int pcm_ioctl(unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case PCM_SET_SAMPLE_RATE:
		jz_audio_set_speed(arg);
		break;
	case PCM_SET_CHANNEL:
		jz_audio_set_channels(arg);
		break;
	case PCM_SET_FORMAT:
		jz_audio_set_format(arg);
		break;
	case PCM_SET_VOL:
		if (arg > 100)
			arg = 100;
		i2s_codec_set_volume(arg);
		break;
	case PCM_GET_VOL:
		return i2s_codec_get_volume();
	default:
		printf("pcm_ioctl:Unsupported I/O command: %08x\n", cmd);
		return -1;
	}
	return 0;
}

int pcm_can_write(void)
{
	if (elements_in_queue(&out_empty_queue) > 0)
		return 1;
	return 0;
}

int pcm_can_read(void)
{
	if (elements_in_queue(&in_full_queue) > 0)
		return 1;
	return 0;
}

int pcm_read(char *buffer, int count)
{
	int id, ret = 0, left_count, copy_count, cnt = 0;
	U8 err;

	if (count < 0)
		return -1;

	if (count < 2*PAGE_SIZE) {
		copy_count = count * 16 / (jz_audio_channels * jz_audio_format);
	} else
		copy_count = 2*PAGE_SIZE;

	left_count = count;

	if (first_record_call) {
		first_record_call = 0;
		if ((id = get_buffer_id(&in_empty_queue)) >= 0) {
			put_buffer_id(&in_busy_queue, id);
			in_dma_buf_data_count[id] = copy_count * (jz_audio_format/8);
			__i2s_enable_receive_dma();
			__i2s_enable_record();

			dma_start(AUDIO_READ_DMA,
				  PHYADDR(AIC_DR), in_dma_pbuf[id],
				  in_dma_buf_data_count[id]);

			OSSemPend(rx_sem, 0, &err);

		} 
	}

	while (left_count > 0) {

		if (elements_in_queue(&in_full_queue) <= 0)
			OSSemPend(rx_sem, 0, &err);

		if ((id = get_buffer_id(&in_full_queue)) >= 0) {
			/*
			 * FIXME: maybe the buffer is too small.
			 */
			cnt = record_filler((unsigned long)buffer+ret, copy_count, id);
			put_buffer_id(&in_empty_queue, id);
		}

		if (elements_in_queue(&in_busy_queue) == 0) {
			if ((id=get_buffer_id(&in_empty_queue)) >= 0) {
				put_buffer_id(&in_busy_queue, id);
				in_dma_buf_data_count[id] = copy_count * (jz_audio_format/8);
				__i2s_enable_receive_dma();
				__i2s_enable_record();

				dma_start(AUDIO_READ_DMA,
					  PHYADDR(AIC_DR), in_dma_pbuf[id],
					  in_dma_buf_data_count[id]);
			}
		}

		if (ret + cnt > count)
			cnt = count - ret;

		ret += cnt;
		left_count -= cnt;
	}

	return ret;
}

int pcm_write(char *buffer, int count)
{
	int id, ret = 0, left_count, copy_count;
	U8 err;

	if (count <= 0)
		return -1;

	/* The data buffer size of the user space is always a PAGE_SIZE
	 * scale, so the process can be simplified. 
	 */

	if (count < 2*PAGE_SIZE)
		copy_count = count;
	else
		copy_count = 2*PAGE_SIZE;

	left_count = count;
  
	while (left_count > 0) {

		if (elements_in_queue(&out_empty_queue) == 0)
			OSSemPend(tx_sem, 0, &err);
 
		/* the end fragment size in this write */
		if (ret + copy_count > count)
			copy_count = count - ret;

		if ((id = get_buffer_id(&out_empty_queue)) >= 0) {
			/*
			 * FIXME: maybe the buffer is too small.
			 */
			replay_filler( (unsigned long)buffer + ret, copy_count, id);
			put_buffer_id(&out_full_queue, id);
			__dcache_writeback_all();
		} 
 
		left_count = left_count - copy_count;
		ret += copy_count;

		if (elements_in_queue(&out_busy_queue) == 0) {
			if ((id=get_buffer_id(&out_full_queue)) >= 0) {
				put_buffer_id(&out_busy_queue, id);

				__i2s_enable_transmit_dma();
				__i2s_enable_replay();

				dma_start(AUDIO_WRITE_DMA,
					  out_dma_pbuf[id], PHYADDR(AIC_DR),
					  out_dma_buf_data_count[id]);
				
			}
		}
	}

	return ret;
}

static unsigned char dma_pool[PAGE_SIZE * (16 *( QUEUE_MAX + 1)*2)];

int pcm_init(void)
{
	int i;
	unsigned char *p = dma_pool;

	jz_i2s_initHw();
	if (jz_i2s_codec_init() < 0)
		return -1;

	tx_sem = OSSemCreate(1);
	rx_sem = OSSemCreate(1);

	dma_request(AUDIO_READ_DMA, jz_i2s_record_dma_irq, 0,
		    DMAC_DCCSR_DAM|DMAC_DCCSR_RDIL_IGN,
		    DMAC_DRSR_RS_AICIN);

	dma_request(AUDIO_WRITE_DMA, jz_i2s_replay_dma_irq, 0,
		    DMAC_DCCSR_SAM|DMAC_DCCSR_RDIL_IGN,
		    DMAC_DRSR_RS_AICOUT);

	jz_audio_reset();
	jz_audio_set_format(AFMT_U8);
	jz_audio_set_speed(8000);
	jz_audio_set_channels(1);

	in_empty_queue.count = QUEUE_MAX;
	out_empty_queue.count = QUEUE_MAX;

	for (i=0;i<QUEUE_MAX;i++) {
		in_empty_queue.id[i] = i;
		out_empty_queue.id[i] = i;
	}

	in_full_queue.count = 0;
	in_busy_queue.count = 0;
	out_busy_queue.count = 0;
	out_full_queue.count = 0;

	/* Aligned in PAGE_SIZE */
	p = (unsigned char *)(((unsigned int)p + PAGE_SIZE) & ~(PAGE_SIZE-1));
 
	for (i = 0; i < QUEUE_MAX; i++) {
		out_dma_buf[i] = (unsigned long)p;
		p += PAGE_SIZE*16;
		out_dma_pbuf[i] = PHYADDR((unsigned int)out_dma_buf[i]);
		in_dma_buf[i] = (unsigned long)p;
		//	p += PAGE_SIZE*16;
		in_dma_pbuf[i] = PHYADDR((unsigned int)in_dma_buf[i]);
	}

	pcm_ioctl(PCM_SET_SAMPLE_RATE, 16000);
	pcm_ioctl(PCM_SET_FORMAT, AFMT_S16_LE);
	pcm_ioctl(PCM_SET_CHANNEL, 2);
	pcm_ioctl(PCM_SET_VOL, 100);	/* 100% */

	return 0;
}

