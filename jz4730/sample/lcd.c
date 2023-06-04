#include <sysdefs.h>
#include <regs.h>
#include <ops.h>

#include "clock.h"

#ifdef UNCACHED
#undef UNCACHED
#endif
#define UNCACHED(addr)	((unsigned int)(addr) | 0x20000000)

#define STATIC_MALLOC

#define MODE_MASK		0x0f
#define MODE_TFT_GEN		0x00
#define MODE_TFT_SHARP		0x01
#define MODE_TFT_CASIO		0x02
#define MODE_TFT_SAMSUNG	0x03
#define MODE_CCIR656_NONINT	0x04
#define MODE_CCIR656_INT	0x05
#define MODE_STN_COLOR_SINGLE	0x08
#define MODE_STN_MONO_SINGLE	0x09
#define MODE_STN_COLOR_DUAL	0x0a
#define MODE_STN_MONO_DUAL	0x0b

#define STN_DAT_PIN1	(0x00 << 4)
#define STN_DAT_PIN2	(0x01 << 4)
#define STN_DAT_PIN4	(0x02 << 4)
#define STN_DAT_PIN8	(0x03 << 4)
#define STN_DAT_PINMASK	STN_DAT_PIN8

#define STFT_PSHI	(1 << 15)
#define STFT_CLSHI	(1 << 14)
#define STFT_SPLHI	(1 << 13)
#define STFT_REVHI	(1 << 12)

#define SYNC_MASTER	(0 << 16)
#define SYNC_SLAVE	(1 << 16)

#define DE_P		(0 << 9)
#define DE_N		(1 << 9)

#define PCLK_P		(0 << 10)
#define PCLK_N		(1 << 10)

#define HSYNC_P		(0 << 11)
#define HSYNC_N		(1 << 11)

#define VSYNC_P		(0 << 8)
#define VSYNC_N		(1 << 8)

#define DATA_NORMAL	(0 << 17)
#define DATA_INVERSE	(1 << 17)

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

#define dprintf(x...)

#define NR_PALETTE	256

struct lcd_desc {
	u32 next;
	u32 buf;
	u32 id;
	u32 cmd;
};

struct jzfb_info {

	u32 cfg;	/* panel mode and pin usage etc. */
	u32 w;
	u32 h;
	u32 bpp;	/* bit per pixel */
	u32 fclk;	/* frame clk */
	u32 hsw;	/* hsync width, in pclk */
	u32 vsw;	/* vsync width, in line count */
	u32 elw;	/* end of line, in pclk */
	u32 blw;	/* begin of line, in pclk */
	u32 efw;	/* end of frame, in line count */
	u32 bfw;	/* begin of frame, in line count */

	u8 *cpal;	/* Cacheable Palette Buffer */
	u8 *pal;	/* Non-cacheable Palette Buffer */
	u8 *cframe;	/* Cacheable Frame Buffer */
	u8 *frame;	/* Non-cacheable Frame Buffer */

	struct {
		u8 red, green, blue;
	} palette[NR_PALETTE];
};

struct jzfb_info jzfb = {
#if 1
	MODE_TFT_GEN | HSYNC_N | VSYNC_N,
	480, 272, 16, 60, 41, 10, 2, 2, 2, 2
#else
	MODE_TFT_GEN | HSYNC_N | VSYNC_N | PCLK_N,
	320, 240, 16, 60, 30, 3, 38, 20, 11, 8
#endif
};

u32 lcd_get_width(void) {return jzfb.w;}
u32 lcd_get_height(void) {return jzfb.h;}
u32 lcd_get_bpp(void) {return jzfb.bpp;}
u8* lcd_get_frame(void) {return jzfb.frame;}
u8* lcd_get_cframe(void) {return jzfb.cframe;}



static struct lcd_desc lcd_palette_desc __attribute__ ((aligned (16)));
static struct lcd_desc lcd_frame_desc0 __attribute__ ((aligned (16)));
static struct lcd_desc lcd_frame_desc1 __attribute__ ((aligned (16)));

static int jzfb_setcolreg(u32 regno, u8 red, u8 green, u8 blue)
{
	u16 *ptr, ctmp;

	if (regno >= NR_PALETTE)
		return 1;

	red	&= 0xff;
	green	&= 0xff;
	blue	&= 0xff;
	
	jzfb.palette[regno].red		= red ;
	jzfb.palette[regno].green	= green;
	jzfb.palette[regno].blue	= blue;
	
	if (jzfb.bpp <= 8) {
		if (((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_SINGLE) ||
		    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL)) {
			ctmp = (77L * red + 150L * green + 29L * blue) >> 8;
			ctmp = ((ctmp >> 3) << 11) | ((ctmp >> 2) << 5) |
				(ctmp >> 3);
		} else {
			/* RGB 565 */
			if (((red >> 3) == 0) && ((red >> 2) != 0))
				red = 1 << 3;
			if (((blue >> 3) == 0) && ((blue >> 2) != 0))
				blue = 1 << 3;
			ctmp = ((red >> 3) << 11) 
				| ((green >> 2) << 5) | (blue >> 3);
		}

		ptr = (u16 *)jzfb.pal;
		ptr[regno] = ctmp;
		REG_LCD_DA0 = PHYS(&lcd_palette_desc);
	} else
		printf("No palette used.\n");

	return 0;
}


#ifdef STATIC_MALLOC
static u8 lcd_heap[1024*1024] __attribute__ ((aligned (4096)));;
static u8 lcd_heap1[1024*1024] __attribute__ ((aligned (4096)));;
#endif

u8* lcd_get_frame_one(void) {return UNCACHED(lcd_heap + 0x1000);}
u8* lcd_get_frame_two(void) {return UNCACHED(lcd_heap1 + 0x1000);}

/*
 * Map screen memory
 */
static int fb_malloc(void)
{
	struct page * map = NULL;
	u8 *tmp;
	u32 page_shift, needroom, t;

	if (jzfb.bpp == 15)
		t = 16;
	else
		t = jzfb.bpp;

	needroom = ((jzfb.w * t + 7) >> 3) * jzfb.h;

#ifndef STATIC_MALLOC
	jzfb.cpal = (u8 *)malloc(512+0x1000);
	jzfb.cpal = (u8 *)((u32)jzfb.cpal & ~0xfff);	/* 4KB aligned */
	jzfb.cframe = (u8 *)malloc(needroom+0x1000);
	jzfb.cframe = (u8 *)((u32)jzfb.cframe & ~0xfff);/* 4KB aligned */
#else
	jzfb.cpal = (u8 *)(((u32)lcd_heap+0x1000) & ~0xfff);
	jzfb.cframe = (u8 *)((u32)jzfb.cpal + 0x1000);
#endif

	jzfb.pal = (u8 *)UNCACHED(jzfb.cpal);
	jzfb.frame = (u8 *)UNCACHED(jzfb.cframe);

	if ((!jzfb.cpal) || (!jzfb.cframe))
		return -1;	/* No memory */

	memset(jzfb.cpal, 0, 512);
	memset(jzfb.cframe, 0, needroom);

	return 0;
}
void change()
{
	struct lcd_desc *frame_desc0 = (struct lcd_desc *)UNCACHED(&lcd_frame_desc0);

	printf("frame = 0x%08x\r\n",frame_desc0->buf);
	
    if(frame_desc0->buf == PHYS(lcd_heap) + 0x1000)
	{
		frame_desc0->buf = PHYS(lcd_heap1) + 0x1000;
	}
	else
	{
		frame_desc0->buf = PHYS(lcd_heap) + 0x1000;
	}
	
}

static void lcd_descriptor_init(void)
{
	int i;
	unsigned int pal_size;
	unsigned int frm_size, ln_size;
	unsigned char dual_panel = 0;
	struct lcd_desc *pal_desc, *frame_desc0, *frame_desc1;

	pal_desc	= &lcd_palette_desc;
	frame_desc0	= &lcd_frame_desc0;
	frame_desc1	= &lcd_frame_desc1;

	i = jzfb.bpp;
	if (i == 15)
		i = 16;
	frm_size = (jzfb.w*jzfb.h*i)>>3;
	ln_size = (jzfb.w*i)>>3;

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL)) {
		dual_panel = 1;
		frm_size >>= 1;
	}

	frm_size = frm_size / 4;
	ln_size = ln_size / 4;

	switch (jzfb.bpp) {
	case 1:
		pal_size = 4;
		break;
	case 2:
		pal_size = 8;
		break;
	case 4:
		pal_size = 32;
		break;
	case 8:
	default:
		pal_size = 512;
		break;
	}

	pal_size /= 4;

	/* Palette Descriptor */
	pal_desc->next	= PHYS(frame_desc0);
	pal_desc->buf	= PHYS(jzfb.pal);
	pal_desc->id	= 0xdeadbeaf;
	pal_desc->cmd	= pal_size|LCD_CMD_PAL; /* Palette Descriptor */

	/* Frame Descriptor 0 */
	frame_desc0->next	= PHYS(frame_desc0);
	frame_desc0->buf	= PHYS(jzfb.frame);
	frame_desc0->id		= 0xbeafbeaf;
	frame_desc0->cmd	= LCD_CMD_SOFINT | LCD_CMD_EOFINT | frm_size;

	if (!(dual_panel))
		return;

	/* Frame Descriptor 1 */
	frame_desc1->next	= PHYS(frame_desc1);
	frame_desc1->buf	= PHYS(jzfb.frame) + frm_size * 4;
	frame_desc1->id		= 0xdeaddead;
	frame_desc1->cmd	 = LCD_CMD_SOFINT | LCD_CMD_EOFINT | frm_size;

}

static int controller_init(void)
{
	unsigned int val = 0;
	unsigned int pclk;
	unsigned int stnH;
	int ret = 0;

	/* Setting Control register */
	switch (jzfb.bpp) {
	case 1:
		val |= LCD_CTRL_BPP_1;
		break;
	case 2:
		val |= LCD_CTRL_BPP_2;
		break;
	case 4:
		val |= LCD_CTRL_BPP_4;
		break;
	case 8:
		val |= LCD_CTRL_BPP_8;
		break;
	case 15:
		val |= LCD_CTRL_RGB555;
	case 16:
		val |= LCD_CTRL_BPP_16;
		break;
	default:
		printf("The BPP %d is not supported\n", jzfb.bpp);
		val |= LCD_CTRL_BPP_16;
		break;
	}

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		switch (jzfb.bpp) {
		case 1:
//			val |= LCD_CTRL_PEDN;
		case 2:
			val |= LCD_CTRL_FRC_2;
			break;
		case 4:
			val |= LCD_CTRL_FRC_4;
			break;
		case 8:
		default:
			val |= LCD_CTRL_FRC_16;
			break;
		}
		break;
	}

	val |= LCD_CTRL_BST_16;		/* Burst Length is 16WORD=64Byte */

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		switch (jzfb.cfg & STN_DAT_PINMASK) {
#define align2(n) (n)=((((n)+1)>>1)<<1)
#define align4(n) (n)=((((n)+3)>>2)<<2)
#define align8(n) (n)=((((n)+7)>>3)<<3)
		case STN_DAT_PIN1:
			/* Do not adjust the hori-param value. */
			break;
		case STN_DAT_PIN2:
			align2(jzfb.hsw);
			align2(jzfb.elw);
			align2(jzfb.blw);
			break;
		case STN_DAT_PIN4:
			align4(jzfb.hsw);
			align4(jzfb.elw);
			align4(jzfb.blw);
			break;
		case STN_DAT_PIN8:
			align8(jzfb.hsw);
			align8(jzfb.elw);
			align8(jzfb.blw);
			break;
		}
		break;
	}

	REG_LCD_CTRL = val;

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		if (((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL) ||
		    ((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL))
			stnH = jzfb.h >> 1;
		else
			stnH = jzfb.h;

		REG_LCD_VSYNC = (0 << 16) | jzfb.vsw;
		REG_LCD_HSYNC = ((jzfb.blw+jzfb.w) << 16) | (jzfb.blw+jzfb.w+jzfb.hsw);

		/* Screen setting */
		REG_LCD_VAT = ((jzfb.blw + jzfb.w + jzfb.hsw + jzfb.elw) << 16) | (stnH + jzfb.vsw + jzfb.bfw + jzfb.efw);
		REG_LCD_DAH = (jzfb.blw << 16) | (jzfb.blw + jzfb.w);
		REG_LCD_DAV = (0 << 16) | (stnH);

		/* AC BIAs signal */
		REG_LCD_PS = (0 << 16) | (stnH+jzfb.vsw+jzfb.efw+jzfb.bfw);

		break;

	case MODE_TFT_GEN:
	case MODE_TFT_SHARP:
	case MODE_TFT_CASIO:
	case MODE_TFT_SAMSUNG:
		REG_LCD_VSYNC = (0 << 16) | jzfb.vsw;
		REG_LCD_DAV = ((jzfb.vsw + jzfb.bfw) << 16) | (jzfb.vsw + jzfb.bfw + jzfb.h);
		REG_LCD_VAT = (((jzfb.blw + jzfb.w + jzfb.elw + jzfb.hsw)) << 16) | (jzfb.vsw + jzfb.bfw + jzfb.h + jzfb.efw);
		REG_LCD_HSYNC = (0 << 16) | jzfb.hsw;
		REG_LCD_DAH = ((jzfb.hsw + jzfb.blw) << 16) | (jzfb.hsw + jzfb.blw + jzfb.w);
		break;
	}

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_TFT_SAMSUNG:
	case MODE_TFT_SHARP:
	case MODE_TFT_CASIO:
		printf("LCD DOES NOT supported.\n");
		break;
	}

	/* Configure the LCD panel */
	REG_LCD_CFG = jzfb.cfg;

	/* Timing setting */
	__cpm_stop_lcd();

	val = jzfb.fclk; /* frame clk */
	pclk = val * (jzfb.w + jzfb.hsw + jzfb.elw + jzfb.blw) *
	       (jzfb.h + jzfb.vsw + jzfb.efw + jzfb.bfw); /* Pixclk */

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL))
		pclk = (pclk * 3);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		pclk = pclk >> ((jzfb.cfg & STN_DAT_PINMASK) >> 4);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		pclk >>= 1;

	val = __cpm_get_pllout() / pclk;
	REG_CPM_CFCR2 = val - 1;
	val = __cpm_get_pllout() / (pclk * 4);
	val = __cpm_divisor_encode(val);
	__cpm_set_lcdclk_div(val);
	REG_CPM_CFCR |= CPM_CFCR_UPE;

	printf("LCDC: PixClock:%d LcdClock:%d\n",
		__cpm_get_pixclk(), __cpm_get_lcdclk());

	__cpm_start_lcd();
	udelay(1000);
	return ret;
}

static void lcd_interrupt_handler(u32 irq)
{
	u32 state;

	state = REG_LCD_STATE;

	if (state & LCD_STATE_EOF) /* End of frame */
		REG_LCD_STATE = state & ~LCD_STATE_EOF;

	if (state & LCD_STATE_IFU0) {
		dprintf("InFiFo0 underrun\n");
		REG_LCD_STATE = state & ~LCD_STATE_IFU0;
	}

	if (state & LCD_STATE_OFU) { /* Out fifo underrun */
		dprintf("Out FiFo underrun.\n");
		REG_LCD_STATE = state & ~LCD_STATE_OFU;
	}
}

static int hw_init(void)
{
	lcd_board_init();

	if (controller_init() < 0)
		return -1;

	if (jzfb.bpp <= 8)
		REG_LCD_DA0 = PHYS(&lcd_palette_desc);
	else
		REG_LCD_DA0 = PHYS(&lcd_frame_desc0);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		REG_LCD_DA1 = PHYS(&lcd_frame_desc1);

	__lcd_set_ena();

	return 0;
}

int jzlcd_init(void)
{
	int err = 0;

	err = fb_malloc();
	if (err) 
		goto failed;

	lcd_descriptor_init();

	__dcache_writeback_all();

	if (hw_init() < 0)
		goto failed;

	request_irq(IRQ_LCD, lcd_interrupt_handler, 0);

	__lcd_enable_ofu_intr();
	__lcd_enable_ifu0_intr();

	return 0;

failed:
	return err;

}


