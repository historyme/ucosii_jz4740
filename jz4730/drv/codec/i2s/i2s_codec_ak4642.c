#include <regs.h>
#include <ops.h>
#include <clock.h>
#include "AK4642.h"

#define u8	unsigned char
#define u16	unsigned short
#define u32	unsigned int
#define AK4642ADDRESS 0x13

static void i2s_codec_write(u8 reg, u16 data)
{
	i2c_write(AK4642ADDRESS,&data,reg,1);	
}

static u16 i2s_codec_read(u8 reg)
{
	u16 data;
	i2c_read(AK4642ADDRESS,(u8 *)&data,reg,1);
	return data;
}

void i2s_codec_init(void)
{
	printf("+++i2s_codec_init");
	i2s_codec_write(AKM_R01_PM2,AKM_MASTER_MODE);
	i2s_codec_write(AKM_R04_MC1,AKM_MCKI_12MHZ | AKM_BCKO_64FS | AKM_FORMAT_I2S);
	i2s_codec_write(AKM_R05_MC2,AKM_FS_44100);
	i2s_codec_write(AKM_R00_PM1,AKM_VCOM_ON);
	i2s_codec_write(AKM_R01_PM2,AKM_MASTER_MODE | AKM_PLL_MODE_ON);
	
	i2s_codec_write(AKM_R0E_MC3,AKM_DVOLC_ON | AKM_BOOST_MIN | AKM_DE_EMPHASIS_OFF);
	i2s_codec_write(AKM_R0F_MC4,AKM_IVOLC_ON);
	
	
	i2s_codec_write(AKM_R0A_LDVC,0xff);
	i2s_codec_write(AKM_R0D_RDVC,0xff);
	i2s_codec_write(AKM_R09_LIVC,0xff);
	i2s_codec_write(AKM_R0C_RIVC,0xff);
	printf("---i2s_codec_init");
}
void i2s_codec_set_volume(u16 v) /* 0 <= v <= 100 */
{
	v = 0xff - v * 0xff / 100;
	i2s_codec_write(AKM_R0A_LDVC,v);
	i2s_codec_write(AKM_R0D_RDVC,v);
	
}

u16 i2s_codec_get_volume(void)
{
	u16 v;
	v = i2s_codec_read(AKM_R0A_LDVC);
	v = (0xff - v) * 100 / 0xff;
	return v;
}

void i2s_codec_set_channel(u16 ch)
{
/*
	if(ch == 1)
	{
		i2s_codec_write(AKM_R02_SS1,0x60);
		i2s_codec_write(AKM_R02_SS1,0xe0);
	}else
	{
		i2s_codec_write(AKM_R02_SS1,v);
		i2s_codec_write(AKM_R02_SS1,v);
	}
*/
}

//------------------------------------------------------------------------------
// g_FreqTable: Freq Table for both input and output
//
//	When the AK4642 working under the PLL MASTER MODE and the MCKI is 12MHz,
//	the Frequency Sampling controlled by FS3, FS2, FS1 and FS0 of the
//	register 05H,
//		Addr	D7		D6		D5		D4		D3		D2		D1		D0
//		05H		-		-		FS3		-		-		FS2		FS1		FS0	
//	And the second parameter is the BIT need to set to register 05H
//
//	{	  0, 0x27 },
//	{ 44100, 0x27 },
//	{ 48000, 0x23 },
//	{ 32000, 0x22 },
//	{ 22050, 0x07 },
//	{  8000, 0x00 },
//	{ 12000, 0x01 },
//	{ 16000, 0x02 },
//	{ 24000, 0x03 },
//	{  7350, 0x04 },
//	{ 11025, 0x05 },
//	{ 14700, 0x06 },
//	{ 29400, 0x26 },

void i2s_codec_set_samplerate(u16 rate)
{
	u8 val = 0;
	switch (rate) {
	case 48000:
		val = 0x23;
		break;
	case 44100:
		val = 0x27;
		break;
	case 32000:
		val = 0x22;
		break;
	case 24000:
		val = 0x03;
		break;
	case 22050:
		val = 0x07;
		break;
	case 16000:
		val = 0x02;
		break;
	case 12000:
		val = 0x01;
		break;
	case 11025:
		val = 0x05;
		break;
	case 7350:
		val = 0x04;
		break;
	case 14700:
		val = 0x06;
		break;
	case 29400:
		val = 0x26;
		break;
	case 8000:
		val = 8;
		break;
		
	default:
		val = 0;
		break;
	}
	i2s_codec_write(AKM_R05_MC2,val);
	
}

