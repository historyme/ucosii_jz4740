/*
 * ac97.c
 *
 * JzSOC On-Chip AC97 audio driver.
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
#include "pcm.h"

static unsigned int k_8000[] = {
	  0,  42,  85, 128, 170, 213, 
};

static unsigned int reload_8000[] = {
	1, 0, 0, 0, 0, 0, 
};

static unsigned int k_11025[] = {
	  0,  58, 117, 176, 234,  37,  96, 154, 
	213,  16,  74, 133, 192, 250,  53, 112, 
	170, 229,  32,  90, 149, 208,  10,  69, 
	128, 186, 245,  48, 106, 165, 224,  26, 
	 85, 144, 202,   5,  64, 122, 181, 240, 
	 42, 101, 160, 218,  21,  80, 138, 197, 
};

static unsigned int reload_11025[] = {
	1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 
	0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 
	0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 
};

static unsigned int k_16000[] = {
	  0,  85, 170, 
};

static unsigned int reload_16000[] = {
	1, 0, 0, 
};

static unsigned int k_22050[] = {
	  0, 117, 234,  96, 213,  74, 192,  53, 
	170,  32, 149,  10, 128, 245, 106, 224, 
	 85, 202,  64, 181,  42, 160,  21, 138, 
};

static unsigned int reload_22050[] = {
	1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 
	1, 0, 1, 0, 1, 0, 1, 0, 
};

static unsigned int k_24000[] = {
	  0, 128, 
};

static unsigned int reload_24000[] = {
	1, 0, 
};

static unsigned int k_32000[] = {
	  0, 170,  85, 
};

static unsigned int reload_32000[] = {
	1, 0, 1, 
};

static unsigned int k_44100[] = {
	  0, 234, 213, 192, 170, 149, 128, 106, 
	 85,  64,  42,  21, 
};

static unsigned int reload_44100[] = {
	1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
};

static unsigned int k_48000[] = {
	  0, 
};

static unsigned int reload_48000[] = {
	1, 
};


static unsigned int f_scale_counts[8] = {
	6, 48, 3, 24, 2, 3, 12, 1, 
};

/* AC97 1.0 */
#define  AC97_RESET               0x0000      //
#define  AC97_MASTER_VOL_STEREO   0x0002      // Line Out
#define  AC97_HEADPHONE_VOL       0x0004      // 
#define  AC97_MASTER_VOL_MONO     0x0006      // TAD Output
#define  AC97_MASTER_TONE         0x0008      //
#define  AC97_PCBEEP_VOL          0x000a      // none
#define  AC97_PHONE_VOL           0x000c      // TAD Input (mono)
#define  AC97_MIC_VOL             0x000e      // MIC Input (mono)
#define  AC97_LINEIN_VOL          0x0010      // Line Input (stereo)
#define  AC97_CD_VOL              0x0012      // CD Input (stereo)
#define  AC97_VIDEO_VOL           0x0014      // none
#define  AC97_AUX_VOL             0x0016      // Aux Input (stereo)
#define  AC97_PCMOUT_VOL          0x0018      // Wave Output (stereo)
#define  AC97_RECORD_SELECT       0x001a      //
#define  AC97_RECORD_GAIN         0x001c
#define  AC97_RECORD_GAIN_MIC     0x001e
#define  AC97_GENERAL_PURPOSE     0x0020
#define  AC97_3D_CONTROL          0x0022
#define  AC97_MODEM_RATE          0x0024
#define  AC97_POWER_CONTROL       0x0026

/* AC'97 2.0 */
#define AC97_EXTENDED_ID          0x0028       /* Extended Audio ID */
#define AC97_EXTENDED_STATUS      0x002A       /* Extended Audio Status */
#define AC97_PCM_FRONT_DAC_RATE   0x002C       /* PCM Front DAC Rate */
#define AC97_PCM_SURR_DAC_RATE    0x002E       /* PCM Surround DAC Rate */
#define AC97_PCM_LFE_DAC_RATE     0x0030       /* PCM LFE DAC Rate */
#define AC97_PCM_LR_ADC_RATE      0x0032       /* PCM LR ADC Rate */
#define AC97_PCM_MIC_ADC_RATE     0x0034       /* PCM MIC ADC Rate */
#define AC97_CENTER_LFE_MASTER    0x0036       /* Center + LFE Master Volume */
#define AC97_SURROUND_MASTER      0x0038       /* Surround (Rear) Master Volume */
#define AC97_RESERVED_3A          0x003A       /* Reserved in AC '97 < 2.2 */

/* AC'97 2.2 */
#define AC97_SPDIF_CONTROL        0x003A       /* S/PDIF Control */

/* range 0x3c-0x58 - MODEM */
#define AC97_EXTENDED_MODEM_ID    0x003C
#define AC97_EXTEND_MODEM_STAT    0x003E
#define AC97_LINE1_RATE           0x0040
#define AC97_LINE2_RATE           0x0042
#define AC97_HANDSET_RATE         0x0044
#define AC97_LINE1_LEVEL          0x0046
#define AC97_LINE2_LEVEL          0x0048
#define AC97_HANDSET_LEVEL        0x004A
#define AC97_GPIO_CONFIG          0x004C
#define AC97_GPIO_POLARITY        0x004E
#define AC97_GPIO_STICKY          0x0050
#define AC97_GPIO_WAKE_UP         0x0052
#define AC97_GPIO_STATUS          0x0054
#define AC97_MISC_MODEM_STAT      0x0056
#define AC97_RESERVED_58          0x0058

/* registers 0x005a - 0x007a are vendor reserved */

#define AC97_VENDOR_ID1           0x007c
#define AC97_VENDOR_ID2           0x007e

/* volume control bit defines */
#define AC97_MUTE                 0x8000
#define AC97_MICBOOST             0x0040
#define AC97_LEFTVOL              0x3f00
#define AC97_RIGHTVOL             0x003f

/* record mux defines */
#define AC97_RECMUX_MIC           0x0000
#define AC97_RECMUX_CD            0x0101
#define AC97_RECMUX_VIDEO         0x0202
#define AC97_RECMUX_AUX           0x0303
#define AC97_RECMUX_LINE          0x0404
#define AC97_RECMUX_STEREO_MIX    0x0505
#define AC97_RECMUX_MONO_MIX      0x0606
#define AC97_RECMUX_PHONE         0x0707

/* general purpose register bit defines */
#define AC97_GP_LPBK              0x0080       /* Loopback mode */
#define AC97_GP_MS                0x0100       /* Mic Select 0=Mic1, 1=Mic2 */
#define AC97_GP_MIX               0x0200       /* Mono output select 0=Mix, 1=Mic */
#define AC97_GP_RLBK              0x0400       /* Remote Loopback - Modem line codec */
#define AC97_GP_LLBK              0x0800       /* Local Loopback - Modem Line codec */
#define AC97_GP_LD                0x1000       /* Loudness 1=on */
#define AC97_GP_3D                0x2000       /* 3D Enhancement 1=on */
#define AC97_GP_ST                0x4000       /* Stereo Enhancement 1=on */
#define AC97_GP_POP               0x8000       /* Pcm Out Path, 0=pre 3D, 1=post 3D */

/* extended audio status and control bit defines */
#define AC97_EA_VRA               0x0001       /* Variable bit rate enable bit */
#define AC97_EA_DRA               0x0002       /* Double-rate audio enable bit */
#define AC97_EA_SPDIF             0x0004       /* S/PDIF Enable bit */
#define AC97_EA_VRM               0x0008       /* Variable bit rate for MIC enable bit */
#define AC97_EA_CDAC              0x0040       /* PCM Center DAC is ready (Read only) */
#define AC97_EA_SDAC              0x0040       /* PCM Surround DACs are ready (Read only) */
#define AC97_EA_LDAC              0x0080       /* PCM LFE DAC is ready (Read only) */
#define AC97_EA_MDAC              0x0100       /* MIC ADC is ready (Read only) */
#define AC97_EA_SPCV              0x0400       /* S/PDIF configuration valid (Read only) */
#define AC97_EA_PRI               0x0800       /* Turns the PCM Center DAC off */
#define AC97_EA_PRJ               0x1000       /* Turns the PCM Surround DACs off */
#define AC97_EA_PRK               0x2000       /* Turns the PCM LFE DAC off */
#define AC97_EA_PRL               0x4000       /* Turns the MIC ADC off */
#define AC97_EA_SLOT_MASK         0xffcf       /* Mask for slot assignment bits */
#define AC97_EA_SPSA_3_4          0x0000       /* Slot assigned to 3 & 4 */
#define AC97_EA_SPSA_7_8          0x0010       /* Slot assigned to 7 & 8 */
#define AC97_EA_SPSA_6_9          0x0020       /* Slot assigned to 6 & 9 */
#define AC97_EA_SPSA_10_11        0x0030       /* Slot assigned to 10 & 11 */

/* S/PDIF control bit defines */
#define AC97_SC_PRO               0x0001       /* Professional status */
#define AC97_SC_NAUDIO            0x0002       /* Non audio stream */
#define AC97_SC_COPY              0x0004       /* Copyright status */
#define AC97_SC_PRE               0x0008       /* Preemphasis status */
#define AC97_SC_CC_MASK           0x07f0       /* Category Code mask */
#define AC97_SC_L                 0x0800       /* Generation Level status */
#define AC97_SC_SPSR_MASK         0xcfff       /* S/PDIF Sample Rate bits */
#define AC97_SC_SPSR_44K          0x0000       /* Use 44.1kHz Sample rate */
#define AC97_SC_SPSR_48K          0x2000       /* Use 48kHz Sample rate */
#define AC97_SC_SPSR_32K          0x3000       /* Use 32kHz Sample rate */
#define AC97_SC_DRS               0x4000       /* Double Rate S/PDIF */
#define AC97_SC_V                 0x8000       /* Validity status */

/* powerdown control and status bit defines */

/* status */
#define AC97_PWR_MDM              0x0010       /* Modem section ready */
#define AC97_PWR_REF              0x0008       /* Vref nominal */
#define AC97_PWR_ANL              0x0004       /* Analog section ready */
#define AC97_PWR_DAC              0x0002       /* DAC section ready */
#define AC97_PWR_ADC              0x0001       /* ADC section ready */

/* control */
#define AC97_PWR_PR0              0x0100       /* ADC and Mux powerdown */
#define AC97_PWR_PR1              0x0200       /* DAC powerdown */
#define AC97_PWR_PR2              0x0400       /* Output mixer powerdown (Vref on) */
#define AC97_PWR_PR3              0x0800       /* Output mixer powerdown (Vref off) */
#define AC97_PWR_PR4              0x1000       /* AC-link powerdown */
#define AC97_PWR_PR5              0x2000       /* Internal Clk disable */
#define AC97_PWR_PR6              0x4000       /* HP amp powerdown */
#define AC97_PWR_PR7              0x8000       /* Modem off - if supported */

/* extended audio ID register bit defines */
#define AC97_EXTID_VRA            0x0001
#define AC97_EXTID_DRA            0x0002
#define AC97_EXTID_SPDIF          0x0004
#define AC97_EXTID_VRM            0x0008
#define AC97_EXTID_DSA0           0x0010
#define AC97_EXTID_DSA1           0x0020
#define AC97_EXTID_CDAC           0x0040
#define AC97_EXTID_SDAC           0x0080
#define AC97_EXTID_LDAC           0x0100
#define AC97_EXTID_AMAP           0x0200
#define AC97_EXTID_REV0           0x0400
#define AC97_EXTID_REV1           0x0800
#define AC97_EXTID_ID0            0x4000
#define AC97_EXTID_ID1            0x8000

/* extended status register bit defines */
#define AC97_EXTSTAT_VRA          0x0001
#define AC97_EXTSTAT_DRA          0x0002
#define AC97_EXTSTAT_SPDIF        0x0004
#define AC97_EXTSTAT_VRM          0x0008
#define AC97_EXTSTAT_SPSA0        0x0010
#define AC97_EXTSTAT_SPSA1        0x0020
#define AC97_EXTSTAT_CDAC         0x0040
#define AC97_EXTSTAT_SDAC         0x0080
#define AC97_EXTSTAT_LDAC         0x0100
#define AC97_EXTSTAT_MADC         0x0200
#define AC97_EXTSTAT_SPCV         0x0400
#define AC97_EXTSTAT_PRI          0x0800
#define AC97_EXTSTAT_PRJ          0x1000
#define AC97_EXTSTAT_PRK          0x2000
#define AC97_EXTSTAT_PRL          0x4000

/* useful power states */
#define AC97_PWR_D0               0x0000      /* everything on */
#define AC97_PWR_D1              AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR4
#define AC97_PWR_D2              AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR2|AC97_PWR_PR3|AC97_PWR_PR4
#define AC97_PWR_D3              AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR2|AC97_PWR_PR3|AC97_PWR_PR4
#define AC97_PWR_ANLOFF          AC97_PWR_PR2|AC97_PWR_PR3  /* analog section off */

/* Total number of defined registers.  */
#define AC97_REG_CNT 64


/* OSS interface to the ac97s.. */
#define AC97_STEREO_MASK (SOUND_MASK_VOLUME|SOUND_MASK_PCM|\
	SOUND_MASK_LINE|SOUND_MASK_CD|\
	SOUND_MASK_ALTPCM|SOUND_MASK_IGAIN|\
	SOUND_MASK_LINE1|SOUND_MASK_VIDEO)

#define AC97_SUPPORTED_MASK (AC97_STEREO_MASK | \
	SOUND_MASK_BASS|SOUND_MASK_TREBLE|\
	SOUND_MASK_SPEAKER|SOUND_MASK_MIC|\
	SOUND_MASK_PHONEIN|SOUND_MASK_PHONEOUT)

#define AC97_RECORD_MASK (SOUND_MASK_MIC|\
	SOUND_MASK_CD|SOUND_MASK_IGAIN|SOUND_MASK_VIDEO|\
	SOUND_MASK_LINE1| SOUND_MASK_LINE|\
	SOUND_MASK_PHONEIN)

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

static const struct {
	unsigned int id;
	char *name;
} ac97_codec_ids[] = {
	{0x41445303, "Analog Devices AD1819"},
	{0x41445340, "Analog Devices AD1881"},
	{0x41445348, "Analog Devices AD1881A"},
	{0x41445360, "Analog Devices AD1885"},
	{0x41445361, "Analog Devices AD1886"},
	{0x41445460, "Analog Devices AD1885"},
	{0x41445461, "Analog Devices AD1886"},
	{0x414B4D00, "Asahi Kasei AK4540"},
	{0x414B4D01, "Asahi Kasei AK4542"},
	{0x414B4D02, "Asahi Kasei AK4543"},
	{0x414C4710, "ALC200/200P"},
	{0x414C4720, "ALC650"},
	{0x43525900, "Cirrus Logic CS4297"},
	{0x43525903, "Cirrus Logic CS4297"},
	{0x43525913, "Cirrus Logic CS4297A rev A"},
	{0x43525914, "Cirrus Logic CS4297A rev B"},
	{0x43525923, "Cirrus Logic CS4298"},
	{0x4352592B, "Cirrus Logic CS4294"},
	{0x4352592D, "Cirrus Logic CS4294"},
	{0x43525931, "Cirrus Logic CS4299 rev A"},
	{0x43525933, "Cirrus Logic CS4299 rev C"},
	{0x43525934, "Cirrus Logic CS4299 rev D"},
	{0x4352594d, "Cirrus Logic CS4201"},
	{0x45838308, "ESS Allegro ES1988"},
	{0x49434511, "ICE1232"},
	{0x4e534331, "National Semiconductor LM4549"},
	{0x50534304, "Philips UCB1400"},
	{0x53494c22, "Silicon Laboratory Si3036"},
	{0x53494c23, "Silicon Laboratory Si3038"},
	{0x545200FF, "TriTech TR?????"},
	{0x54524102, "TriTech TR28022"},
	{0x54524103, "TriTech TR28023"},
	{0x54524106, "TriTech TR28026"},
	{0x54524108, "TriTech TR28028"},
	{0x54524123, "TriTech TR A5"},
	{0x574D4C00, "Wolfson WM9700A"},
	{0x574D4C03, "Wolfson WM9703/WM9707"},
	{0x574D4C04, "Wolfson WM9704M/WM9704Q"},
	{0x83847600, "SigmaTel STAC????"},
	{0x83847604, "SigmaTel STAC9701/3/4/5"},
	{0x83847605, "SigmaTel STAC9704"},
	{0x83847608, "SigmaTel STAC9708"},
	{0x83847609, "SigmaTel STAC9721/23"},
	{0x83847644, "SigmaTel STAC9744/45"},
	{0x83847656, "SigmaTel STAC9756/57"},
	{0x83847666, "SigmaTel STAC9750T"},
	{0x83847684, "SigmaTel STAC9783/84?"},
	{0x57454301, "Winbond 83971D"},
	{0xffffffff, "End flag"},
};

void ac97codec_info(unsigned short id1, unsigned short id2)
{
	int i;
	unsigned int id = (id1 << 16) | id2;
	for (i=0;;i++) {
		if (ac97_codec_ids[i].id == 0xffffffff) {
			printf("AC97 codec: Unknown codec (%04x %04x)\n",
			       id1, id2);
			break;
		}
		if (ac97_codec_ids[i].id == id) {
			printf("AC97 codec: %s\n", ac97_codec_ids[i].name);
			break;
		}
	}
}

/* maxinum number of AC97 codecs connected, AC97 2.0 defined 4 */
#define NR_AC97		2

#define STANDARD_SPEED  48000
#define MAX_RETRY       100

static int		jz_audio_rate;
static char		jz_audio_format;
static char		jz_audio_channels;
static int              jz_audio_k;   /* rate expand multiple,  for record */
static int              jz_audio_q;   /* rate expand compensate, for record */
static int		jz_audio_count;  /* total count of voice data */
static int		last_jz_audio_count;
static int		bitscale;

static unsigned int	f_scale_count;
static unsigned int	*f_scale_array;
static unsigned int	*f_scale_reload;
static unsigned int	f_scale_idx;

static void jz_update_filler(int bits, int channels);

static void jz_ac97_replay_dma_irq(unsigned int);
static void jz_ac97_record_dma_irq(unsigned int);

static void (*replay_filler)(unsigned long src_start, int count, int id);
static int (*record_filler)(unsigned long dst_start, int count, int id);

extern u16 ac97_codec_read(u8 reg);
extern void ac97_codec_write(u8 reg, u16 data);

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

static int first_record_call = 1;

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
static void jz_ac97_record_dma_irq (unsigned int arg)
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

static void jz_ac97_replay_dma_irq (unsigned int arg)
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
			printf("Strange DMA finish interrupt for AC97 module\n");

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
 * Initialize the onchip AC97 controller
 */
static void jz_ac97_initHw(void)
{
	__ac97_disable();
	__ac97_reset();
	__ac97_enable();

	__ac97_cold_reset_codec();
	/* wait for a long time to let ac97 controller reset completely,
	 * otherwise, registers except ACFR will be clear by reset, can't be
	 * set correctly.
	 */
	udelay(160);

	__ac97_disable_record();
	__ac97_disable_replay();
	__ac97_disable_loopback();

	/* Set FIFO data size. Which shows valid data bits.
	 */
	__ac97_set_oass(8);
	__ac97_set_iass(8);

	__ac97_set_xs_stereo();
	__ac97_set_rs_stereo();
}


static int jz_audio_set_speed(int rate)
{
	/* 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 99999999 ? */
	u32 dacp;

	if (rate > 48000)
		rate = 48000;
	if (rate < 8000)
		rate = 8000;


	/* Power down the DAC */
	dacp=ac97_codec_read(AC97_POWER_CONTROL);
	ac97_codec_write(AC97_POWER_CONTROL, dacp|0x0200);
	ac97_codec_write(AC97_PCM_FRONT_DAC_RATE, 48000);

	/* Power it back up */
	ac97_codec_write(AC97_POWER_CONTROL, dacp);

	jz_audio_rate = rate;

	jz_audio_k = STANDARD_SPEED / rate;
	if (rate * jz_audio_k != STANDARD_SPEED) 
		jz_audio_q = rate / ((STANDARD_SPEED / jz_audio_k) - rate );
	else
		jz_audio_q = 0x1fffffff; /* a very big value, don't compensate */

	switch (rate) {
	case 8000:
		f_scale_count	= f_scale_counts[0];
		f_scale_array	= k_8000;
		f_scale_reload	= reload_8000;
		break;
	case 11025:
		f_scale_count	= f_scale_counts[1];
		f_scale_array	= k_11025;
		f_scale_reload	= reload_11025;
		break;
	case 16000:
		f_scale_count	= f_scale_counts[2];
		f_scale_array	= k_16000;
		f_scale_reload	= reload_16000;
		break;
	case 22050:
		f_scale_count	= f_scale_counts[3];
		f_scale_array	= k_22050;
		f_scale_reload	= reload_22050;
		break;
	case 24000:
		f_scale_count	= f_scale_counts[4];
		f_scale_array	= k_24000;
		f_scale_reload	= reload_24000;
		break;
	case 32000:
		f_scale_count	= f_scale_counts[5];
		f_scale_array	= k_32000;
		f_scale_reload	= reload_32000;
		break;
	case 44100:
		f_scale_count	= f_scale_counts[6];
		f_scale_array	= k_44100;
		f_scale_reload	= reload_44100;
		break;
	case 48000:
		f_scale_count	= f_scale_counts[7];
		f_scale_array	= k_48000;
		f_scale_reload	= reload_48000;
		break;
	}
	f_scale_idx	= 0;

	return jz_audio_rate;
}

static int record_fill_1x8_u(unsigned long dst_start, int count, int id)
{
	int cnt = 0;
	unsigned char data;
	volatile unsigned char *s = (unsigned char *)(in_dma_buf[id]);
	volatile unsigned char *dp = (unsigned char *)dst_start;

	while (count > 0) {
		count -= 2;		/* count in dword */
		if ((jz_audio_count++ % jz_audio_k) == 0) {
			cnt++;
			data = *s++;
			*dp++ = data + 0x80;
			s++;		/* skip the other channel */
		} else
			s += 2;		/* skip the redundancy */

		if (jz_audio_count - last_jz_audio_count >= jz_audio_q) {
			jz_audio_count++;
			last_jz_audio_count = jz_audio_count;
			count -= 2;
			s += 2;
		}
	}
	return cnt;
}

static int record_fill_2x8_u(unsigned long dst_start, int count, int id)
{
	int cnt = 0;
	unsigned char d1, d2;
	volatile unsigned char *s = (unsigned char *)(in_dma_buf[id]);
	volatile unsigned char *dp = (unsigned char *)dst_start;

	while (count > 0) {
		count -= 2;
		if ((jz_audio_count++ % jz_audio_k) == 0) {
			cnt += 2;
			d1 = *s++;
			*dp++ = d1 + 0x80;
			d2 = *s++;
			*dp++ = d2 + 0x80;
		} else
			s += 2;		/* skip the redundancy */

		if (jz_audio_count - last_jz_audio_count >= jz_audio_q * 2) {
			jz_audio_count += 2;
			last_jz_audio_count = jz_audio_count;
			count -= 2;
			s += 2;
		}
	}
	return cnt;
}

static int record_fill_1x16_s(unsigned long dst_start, int count, int id)
{
	int cnt = 0;
	unsigned short d1;
	unsigned short *s = (unsigned short *)(in_dma_buf[id]);
	unsigned short *dp = (unsigned short *)dst_start;

	while (count > 0) {
		count -= 2;		/* count in dword */
		if ((jz_audio_count++ % jz_audio_k) == 0) {
			cnt += 2;	/* count in byte */
			d1 = *s++;
			*dp++ = d1;
			s++;		/* skip the other channel */
		} else
			s += 2;		/* skip the redundancy */

		if (jz_audio_count - last_jz_audio_count >= jz_audio_q * 2) {
			jz_audio_count += 2;
			last_jz_audio_count = jz_audio_count;
			count -= 2;
			s += 2;
		}
	}
	return cnt;
}

static int record_fill_2x16_s(unsigned long dst_start, int count, int id)
{
	int cnt = 0;
	unsigned short d1, d2;
	unsigned short *s = (unsigned short *)(in_dma_buf[id]);
	unsigned short *dp = (unsigned short *)dst_start;

	while (count > 0) {
		count -= 2;		/* count in dword */
		if ((jz_audio_count++ % jz_audio_k) == 0) {
			cnt += 4;	/* count in byte */
			d1 = *s++;
			*dp++ = d1;
			d2 = *s++;
			*dp++ = d2;
		} else
			s += 2;		/* skip the redundancy */

		if (jz_audio_count - last_jz_audio_count >= jz_audio_q * 4) {
			jz_audio_count += 4;
			last_jz_audio_count = jz_audio_count;
			count -= 2;
			s += 2;
		}
	}
	return cnt;
}


static void replay_fill_1x8_u(unsigned long src_start, int count, int id)
{
	int i, cnt = 0;
	unsigned char data;
	unsigned char *s = (unsigned char *)src_start;
	unsigned char *dp = (unsigned char *)(out_dma_buf[id]);

	while (count > 0) {
		count--;
		jz_audio_count++;
		cnt += jz_audio_k;
		data = *s++ - 0x80;
		for (i=0;i<jz_audio_k;i++) {
			*dp++ = data;
			*dp++ = data;
		}
	}
	cnt = cnt * 2;
	out_dma_buf_data_count[id] = cnt;
}

static void replay_fill_2x8_u(unsigned long src_start, int count, int id)
{
	int i, cnt = 0;
	unsigned char d1, d2;
	unsigned char *s = (unsigned char *)src_start;
	unsigned char *dp = (unsigned char*)(out_dma_buf[id]);

	while (count > 0) {
		count -= 2;
		jz_audio_count += 2;
		cnt += 2 * jz_audio_k;
		d1 = *s++ - 0x80;
		d2 = *s++ - 0x80;
		for (i=0;i<jz_audio_k;i++) {
			*dp++ = d1;
			*dp++ = d2;
		}
		if (jz_audio_count - last_jz_audio_count >= jz_audio_q * 2) {
			cnt += 2 * jz_audio_k;
			last_jz_audio_count = jz_audio_count;
			for (i=0;i<jz_audio_k;i++) {
				*dp++ = d1;
				*dp++ = d2;
			}
		}
	}
	out_dma_buf_data_count[id] = cnt;
}

static void replay_fill_1x16_s(unsigned long src_start, int count, int id)
{
	int i, cnt = 0;
	static short d1, d2, d;
	short *s = (short *)src_start;
	short *dp = (short *)(out_dma_buf[id]);

	d2 = *s++;
	count -= 2;
	while (count >= 0) {
		if (f_scale_reload[f_scale_idx]) {
			d1 = d2;
			d2 = *s++;
			if (!count)
				break;
			count -= 2;
		}
		d = d1 + (((d2 - d1) * f_scale_array[f_scale_idx]) >> 8);
		*dp++ = d;
		*dp++ = d;
		cnt += 4;
		f_scale_idx ++;
		if (f_scale_idx >= f_scale_count)
			f_scale_idx = 0;
	}
	out_dma_buf_data_count[id] = cnt;
}

static void replay_fill_2x16_s(unsigned long src_start, int count, int id)
{
	int i, cnt = 0;
	static short d11, d12, d21, d22, d1, d2;
	short *s = (short *)src_start;
	short *dp = (short *)(out_dma_buf[id]);

	d12 = *s++;
	d22 = *s++;
	count -= 4;
	while (count >= 0) {
		register unsigned int kvalue;
		kvalue = f_scale_array[f_scale_idx];
		if (f_scale_reload[f_scale_idx]) {
			d11 = d12;
			d12 = *s++;
			d21 = d22;
			d22 = *s++;
			if (!count)
				break;
			count -= 4;
		}
		d1 = d11 + (((d12 - d11)*kvalue) >> 8);
		d2 = d21 + (((d22 - d21)*kvalue) >> 8);
		*dp++ = d1;
		*dp++ = d2;
		cnt += 4;
		f_scale_idx ++;
		if (f_scale_idx >= f_scale_count)
			f_scale_idx = 0;
	}
	out_dma_buf_data_count[id] = cnt;
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
		__ac97_set_xs_stereo();	// always stereo when playing
		__ac97_set_rs_stereo();	// always stereo when recording
		jz_audio_channels = channels;
		jz_update_filler(jz_audio_format, jz_audio_channels);
		break;
	case 2:
		__ac97_set_xs_stereo();
		__ac97_set_rs_stereo();
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

	__ac97_disable_replay();
	__ac97_disable_receive_dma();
	__ac97_disable_record();
	__ac97_disable_transmit_dma();

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

/* Read / Write AC97 codec registers */
static inline int jz_out_command_ready(void)
{
	int t2 = 1000;
	int done = 0;

	while (! done && t2-- > 0) {
		if (REG32(AC97_ACSR) & AIC_ACSR_CADT) {
			REG32(AC97_ACSR) &= ~AIC_ACSR_CADT;
			done = 1;
		} else
			udelay (1);
	}
	return done;
}
	
static inline int jz_in_status_ready(void)
{
	int t2 = 1000;
	int done = 0;

	while (! done && t2-- > 0)
		if (REG32(AC97_ACSR) & AIC_ACSR_SADR) {
			REG32(AC97_ACSR) &= ~AIC_ACSR_SADR;
			done = 1;
		} else {
			if (REG32(AC97_ACSR) & AIC_ACSR_RSTO) {
				REG32(AC97_ACSR) &= ~AIC_ACSR_RSTO;
				done = 0;
				break;
			}
			udelay (1);
		}

	return done;
}

static int jz_readAC97Reg (u8 reg)
{
	u16 value;

	if (reg < 128) {
		u8  ret_reg;
		__ac97_out_rcmd_addr(reg);
		if (jz_out_command_ready())
			while (jz_in_status_ready()) {
				ret_reg = __ac97_in_status_addr();
				value = __ac97_in_data();
				if (ret_reg == reg)
					return value;
				else
					return -1;
			}
	}
	value = __ac97_in_data();
	return -1;
}

static int jz_writeAC97Reg (u8 reg, u16 value)
{
	unsigned long flags;
	int done = 0;

	flags = spin_lock_irqsave();

	__ac97_out_wcmd_addr(reg);
	__ac97_out_data(value);
	if (jz_out_command_ready())
		done = 1;

	spin_unlock_irqrestore(flags);
	return done;
}

u16 ac97_codec_read(u8 reg)
{
	int res = jz_readAC97Reg(reg);
	int count = 0;

	while (res == -1) {
		udelay(1000);
		__ac97_warm_reset_codec();
		udelay(1000);
		res = jz_readAC97Reg(reg);
		count ++;
		if (count > MAX_RETRY)
			break;
	}
	return (u16)res;
}

void ac97_codec_write(u8 reg, u16 data)
{
	int done = jz_writeAC97Reg(reg, data);
	int count = 0;

	while (done == 0) {
		count ++;
		udelay (2000);
		__ac97_warm_reset_codec();
		udelay(2000);
		done = jz_writeAC97Reg(reg, data);
		if ( count > MAX_RETRY )
			break;
	}
}

/* AC97 codec initialisation. */
static int jz_ac97_codec_init(void)
{
	int num_ac97 = 0;
	int ready_2nd = 0;
	unsigned short eid, t;
	int i = 0;

	if (__ac97_codec_is_low_power_mode()) {
		printf("AC97 codec is low power mode, warm reset ...\n");
		__ac97_warm_reset_codec();
		udelay(10);
	}

	i = 0;
	while (!__ac97_codec_is_ready()) {
		i++;
		if ( i > 100) {
			printf("AC97 codec not ready, failed init ..\n");
			return -1;
		}
		udelay(10);
	}

	i = 0;
	/* AC97 probe codec */
	ac97_codec_write(AC97_RESET, 0L);
	udelay(10);
	if (ac97_codec_read(AC97_RESET) & 0x8000) {
		printf("AC97 codec not present...\n");
		return -1;
	}
	ac97_codec_write(AC97_EXTENDED_MODEM_ID, 0L);
	/*  Probe for Modem Codec */
	ac97_codec_read(AC97_EXTENDED_MODEM_ID);
	ac97codec_info(ac97_codec_read(AC97_VENDOR_ID1), ac97_codec_read(AC97_VENDOR_ID2));

	/*  Codec capacity */
	t = ac97_codec_read(AC97_RESET);
	printf("Support:");
	if (t & 0x04)
		printf(" BASS TREBLE");
	if (t & 0x10)
		printf(" ALTPCM");
	printf("\n");
	ac97_codec_write(AC97_MASTER_VOL_STEREO, 0x2020);
	if (ac97_codec_read(AC97_MASTER_VOL_STEREO) == 0x2020)
		bitscale = 6;
	else
		bitscale = 5;
	printf("AC97 Master Volume bit resolution: %dbit\n", bitscale);

	eid = ac97_codec_read(AC97_EXTENDED_ID);
	if (eid == 0xFFFF) {
		printf("AC97: no codec attached?\n");
		return -1;
	}

	if (!(eid & 0x0001))
		printf("AC97 codec: only supports fixed 48KHz sample rate\n");
	else {
		printf("AC97 codec: supports variable sample rate\n");

		/* enable head phone */
		ac97_codec_write(0x6a, ac97_codec_read(0x6a)|0x40);
#if 0
		ac97_codec_write(AC97_EXTENDED_STATUS, 9);
		ac97_codec_write(AC97_EXTENDED_STATUS,
				 ac97_codec_read(AC97_EXTENDED_STATUS)|0xe800);
		
#endif
		ac97_codec_write(AC97_POWER_CONTROL,
				 ac97_codec_read(AC97_POWER_CONTROL)&~0x7f00);
		for (i=10;i;i--)
			if ((ac97_codec_read(AC97_POWER_CONTROL) & 0xf) == 0xf)
				break;
#if 0
		if (!ac97_codec_read(AC97_EXTENDED_STATUS) & 1)
			printf("Codec refuse to support VRA, using 48Khz\n");
#endif
	}

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
		__aic_disable_mono2stereo();
		dma_block_size(AUDIO_READ_DMA, 4);
		dma_block_size(AUDIO_WRITE_DMA, 4);
		__aic_disable_unsignadj();
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
		arg = 100 - arg;
		arg = arg * ((1 << bitscale) - 1) / 100;
		arg = (arg << 8) | arg;
		ac97_codec_write(AC97_MASTER_VOL_STEREO, arg);
		ac97_codec_write(AC97_PCMOUT_VOL, arg);
		break;
	case PCM_GET_VOL:
		ac97_codec_read(AC97_MASTER_VOL_STEREO);
		break;
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

	if (count < 2*PAGE_SIZE / jz_audio_k) {
		copy_count = count * 16 / (jz_audio_channels * jz_audio_format);
	} else
		copy_count = ((2*PAGE_SIZE / jz_audio_k + 3) / 4) * 4;

	left_count = count;

	if (first_record_call) {
		first_record_call = 0;
		if ((id = get_buffer_id(&in_empty_queue)) >= 0) {
			put_buffer_id(&in_busy_queue, id);
			in_dma_buf_data_count[id] = copy_count * (jz_audio_format/8);
			__ac97_enable_receive_dma();
			__ac97_enable_record();
			dma_start(AUDIO_READ_DMA,
				  PHYADDR(AIC_DR), in_dma_pbuf[id],
				  in_dma_buf_data_count[id]);

			OSSemPend(rx_sem, 0, &err);

		} 
	}

	while (left_count > 0) {

		while (elements_in_queue(&in_full_queue) <= 0)
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
				__ac97_enable_receive_dma();
				__ac97_enable_record();
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

	if (count < 2*PAGE_SIZE / jz_audio_k)
		copy_count = count;
	else
		copy_count = ((2*PAGE_SIZE / jz_audio_k + 3) / 4) * 4;

	left_count = count;
  
	while (left_count > 0) {

		while (elements_in_queue(&out_empty_queue) == 0)
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

				__ac97_enable_transmit_dma();
				__ac97_enable_replay();
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

	jz_ac97_initHw();
	if (jz_ac97_codec_init() < 0)
		return -1;

	tx_sem = OSSemCreate(0);
	rx_sem = OSSemCreate(0);

	dma_request(AUDIO_READ_DMA, jz_ac97_record_dma_irq, 0,
		    DMAC_DCCSR_DAM|DMAC_DCCSR_RDIL_IGN,
		    DMAC_DRSR_RS_AICIN);

	dma_request(AUDIO_WRITE_DMA, jz_ac97_replay_dma_irq, 0,
		    DMAC_DCCSR_SAM|DMAC_DCCSR_RDIL_IGN,
		    DMAC_DRSR_RS_AICOUT);

	jz_audio_reset();
	jz_audio_set_format(AFMT_U8);
	jz_audio_set_speed(8000);
	jz_audio_set_channels(1);

	last_jz_audio_count = jz_audio_count = 0;
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
		p += PAGE_SIZE*16;
		in_dma_pbuf[i] = PHYADDR((unsigned int)in_dma_buf[i]);
	}

	pcm_ioctl(PCM_SET_SAMPLE_RATE, 16000);
	pcm_ioctl(PCM_SET_FORMAT, AFMT_S16_LE);
	pcm_ioctl(PCM_SET_CHANNEL, 2);
	pcm_ioctl(PCM_SET_VOL, 100);	/* 100% */

	return 0;
}

