#include <regs.h>
#include <ops.h>
#include <clock.h>

#define SCC1_BASE 0xB0044000
#define I2S_PDN  68

#define u8	unsigned char
#define u16	unsigned short
#define u32	unsigned int

int IS_WRITE_PCM;
u16 error;

static unsigned int i2c_addr = 0x13; //AK4642EN device address at I2C bus
static unsigned int i2c_clk = 100000;//AK4642EN 400kHz max,but 100kHz here

static void write_reg(u8 reg, u8 val)
{
	i2c_write(i2c_addr, &val, reg, 1);
}

static u8 read_reg(u8 reg)
{
	u8 val;
	i2c_read(i2c_addr, &val, reg, 1);
	return val;
}

static void i2s_codec_write(u8 reg, u16 data)
{

	u8 val = data & 0xff;
	write_reg(reg, val);
}

static u16 i2s_codec_read(u8 reg)
{

	u16 value;
	value = read_reg(reg);
	return value;
}

void i2s_codec_clear(void)
{
	if(IS_WRITE_PCM){
		i2s_codec_write(0x01, 0x0039);
		i2s_codec_write(0x01, 0x0009);
		i2s_codec_write(0x00, 0x0040);
		i2s_codec_write(0x0e, 0x0000);
		i2s_codec_write(0x0f, 0x0008);
	}
	else{
		i2s_codec_write(0x00, 0x0040);
		i2s_codec_write(0x10, 0x0000);
		i2s_codec_write(0x07, 0x0001);
	}
	i2s_codec_write(0x01, 0x0008);//master,PLL disable
	__gpio_clear_pin(I2S_PDN);
	udelay(2);
	REG_SCC1_CR(SCC1_BASE) &= 0 << 31;
	udelay(2);
}


void i2s_codec_init(void)
{

	//set AIC pin to I2S slave mode,only GPIO70,71,77,78
	__gpio_as_output(68);
	__gpio_clear_pin(68);
	__gpio_as_output(69);
	__gpio_clear_pin(69);
	__gpio_as_output(70);
	__gpio_clear_pin(70);
	__gpio_as_input(71);
	__gpio_clear_pin(71);
	__gpio_as_input(77);
	__gpio_clear_pin(77);
	__gpio_as_input(78);
	__gpio_clear_pin(78);
	__gpio_as_scc1();

	//set SCC clock initialization
	REG_SCC1_CR(SCC1_BASE) = 0x00000000;

	udelay(2);
	REG_SCC1_CR(SCC1_BASE) |= 1 << 31;
	udelay(2);

	__gpio_as_output(I2S_PDN);
	__gpio_set_pin(I2S_PDN);
	udelay(5);
	__gpio_clear_pin(I2S_PDN);
	udelay(1);//>150ns
	__gpio_set_pin(I2S_PDN);
	udelay(1000);
	REG_GPIO_GPALR(2) &= 0xC3FF0CFF;
	REG_GPIO_GPALR(2) |= 0x14005000;

	//set PLL Master mode
	i2s_codec_write(0x01, 0x0008);//master
	i2s_codec_write(0x04, 0x006b);//ref:12MHz;BITCLK:64fs;I2S compli
	i2s_codec_write(0x05, 0x000b);//sync:48KHz;
	i2s_codec_write(0x00, 0x0040);//PMVCM
	i2s_codec_write(0x01, 0x0009);//master,PLL enable
	//register IRQ  

	if(IS_WRITE_PCM){
		i2s_codec_write(0x05, 0x0027);
		i2s_codec_write(0x0f, 0x0009);
		i2s_codec_write(0x0e, 0x0015);//0x0014
		i2s_codec_write(0x09, 0x0091);//0x91
		i2s_codec_write(0x0c, 0x0091);
		i2s_codec_write(0x0a, 0x0028);//HP volume output value,0x0028
		i2s_codec_write(0x0d, 0x0028);
		i2s_codec_write(0x00, 0x0064);
		i2s_codec_write(0x01, 0x0039);
		i2s_codec_write(0x01, 0x0079);
	}
	else{
		i2s_codec_write(0x05, 0x0027);//0x27 for 44.1KHz,0x23 for 48KHz
		i2s_codec_write(0x02, 0x0004);//0x0005,0x0004
		i2s_codec_write(0x06, 0x003c);
		i2s_codec_write(0x08, 0x00e1);
		i2s_codec_write(0x0b, 0x0000);
		i2s_codec_write(0x07, 0x0021);//0x0021
		// Select Adc Channel = 10
		i2s_codec_write(0x00, 0x0041);
		i2s_codec_write(0x10, 0x0000);//0x0001,0x0018
		
		i2s_codec_write(0x09, 0x0091);//0x91
		i2s_codec_write(0x0c, 0x0091);
	}
}

void i2s_codec_set_volume(u16 v) /* 0 <= v <= 100 */
{
	if(IS_WRITE_PCM){	
		u16 codec_volume = 255 - 255 * v / 100;
		i2s_codec_write(0x0a, codec_volume);
		i2s_codec_write(0x0d, codec_volume);
	}else{
		u16 codec_volume = 241 * v /100;
		i2s_codec_write(0x09, codec_volume);
		i2s_codec_write(0x0c, codec_volume);
	}
}
u16 i2s_codec_get_volume(void)
{
	u16 val;
	int ret;
	if(IS_WRITE_PCM){
		u16 codec_volume =i2s_codec_read(0x0a);
		val = 100 - 100 * codec_volume / 255;
		ret = val << 8;
		val = val | ret;	
	}else{
		u16 codec_volume =i2s_codec_read(0x09);
		val = 100 * codec_volume / 241;
		ret = val << 8;
		val = val | ret;
	}	
	return val;
}

void i2s_codec_set_channel(u16 ch)
{
//	i2s_codec_write(I2S_KEYCLICK_CONTROL, 0x4415);
}

void i2s_codec_set_samplerate(u16 rate)
{
	unsigned short speed = 0;
	unsigned short val = 0;
	
	switch (rate) {
	case 8000:
		speed = 0x00;
		break;
	case 12000:
		speed = 0x01;
		break;
	case 16000:
		speed = 0x02;
		break;
	case 24000:
		speed = 0x03;
		break;
	case 7350:
		speed = 0x04;
		break;
	case 11025:
		speed = 0x05;
		break;
	case 14700:
		speed = 0x06;
		break;
	case 22050:
		speed = 0x07;
		break;
	case 32000:
		speed = 0x0a;
		break;
	case 48000:
		speed = 0x0b;
		break;
	case 294000:
		speed = 0x0e;
		break;
	case 44100:
		speed = 0x0f;
		break;			
	default:
		break;
	}
	
	val = speed & 0x08;
	val = val << 2;
	speed = speed & 0x07;
	val = val | speed;
	i2s_codec_write(0x05, val);
}

