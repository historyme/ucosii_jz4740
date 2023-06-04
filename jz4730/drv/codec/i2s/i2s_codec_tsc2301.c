#include <regs.h>
#include <ops.h>
#include <clock.h>

#define PAGE_NO			2
#define	I2S_CONTROL		0
#define I2S_ADC_VOLUE_CONTROL	1
#define I2S_DAC_VOLUE_CONTROL	2
#define I2S_KEYCLICK_CONTROL	4
#define I2S_POWER_CONTROL	5

#define u8	unsigned char
#define u16	unsigned short
#define u32	unsigned int

static void i2s_codec_write(u8 reg, u16 data)
{
	u16 cmd;
	cmd = (reg << 5) | (PAGE_NO << 11);
	spi_write(cmd);
	spi_write(data);
	udelay(100);
}

static u16 i2s_codec_read(u8 reg)
{
	u16 cmd;
	u8 buf[2];
	cmd = (1 << 15) | (reg << 5) | (PAGE_NO << 11);
	spi_write(cmd);
	return spi_read();
}

static int spi_inited = 0;
void i2s_codec_init(void)
{
	if (!spi_inited) {
		spi_init();
		spi_inited = 1;
	}

	i2s_codec_write(I2S_DAC_VOLUE_CONTROL, 0xffff);
	i2s_codec_write(I2S_ADC_VOLUE_CONTROL, 0xffff);
	i2s_codec_write(I2S_POWER_CONTROL, 0xffff);
	i2s_codec_write(I2S_POWER_CONTROL, 0x000f);
	i2s_codec_write(I2S_DAC_VOLUE_CONTROL, 0x7f7f);
	i2s_codec_write(I2S_ADC_VOLUE_CONTROL, 0x7f7f);
}

void i2s_codec_set_volume(u16 v) /* 0 <= v <= 100 */
{
	u16 vol;
	v = (v * 0x7f) / 100;
	vol = (v << 8) | v;
	i2s_codec_write(I2S_DAC_VOLUE_CONTROL, vol);
	i2s_codec_write(I2S_ADC_VOLUE_CONTROL, vol);
}

u16 i2s_codec_get_volume(void)
{
	u16 vol;
	vol = i2s_codec_read(I2S_DAC_VOLUE_CONTROL);
	vol = ((vol & 0xff) * 100) / 0x7f;
	return vol;
}

void i2s_codec_set_channel(u16 ch)
{
	i2s_codec_write(I2S_KEYCLICK_CONTROL, 0x4415);
}

void i2s_codec_set_samplerate(u16 rate)
{
	u16 speed = 0;
	u16 val = 0;
	u8  mclk = 0;
	switch (rate) {
	case 48000:
		speed = 0;
		mclk = 0;
		break;
	case 44100:
		speed = 1;
		mclk = 0;
		break;
	case 32000:
		speed = 2;
		mclk = 0;
		break;
	case 24000:
		speed = 3;
		mclk = 0;
		break;
	case 22050:
		speed = 4;
		mclk = 0;
		break;
	case 16000:
		speed = 5;
		mclk = 0;
		break;
	case 12000:
		speed = 6;
		mclk = 0;
		break;
	case 11025:
		speed = 7;
		mclk = 0;
		break;
	case 8000:
	default:
		speed = 8;
		mclk = 0;
		break;
	}

	val = (0xc2 << 8) | (mclk << 6) | (speed << 2) | 0x03;
	i2s_codec_write(I2S_CONTROL, val);
	__i2s_set_sample_rate(__cpm_get_i2sclk(), rate);
}

