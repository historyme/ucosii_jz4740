#if LCDTYPE == 3
#include <regs.h>
#include <ops.h>

#define GPIO_DISP_OFF_N   93
#define MODE 0xcd 		/* 24bit parellel RGB */
#define SPEN	60       //LCD_SPL
#define SPCK	61       //LCD_CLS
#define SPDA	62       //LCD_PS
#define LCD_RET 63       //LCD_REV  //use for lcd reset
#define GPIO_PWM0 94

#define __spi_write_reg1(reg, val) \
	do { \
		unsigned char no;\
		unsigned short value;\
		unsigned char a=0;\
		unsigned char b=0;\
		a=reg;\
		b=val;\
		__gpio_set_pin(SPEN);\
		__gpio_set_pin(SPCK);\
		__gpio_clear_pin(SPDA);\
		__gpio_clear_pin(SPEN);\
		udelay(25);\
		value=((a<<8)|(b&0xFF));\
		for(no=0;no<16;no++)\
		{\
			__gpio_clear_pin(SPCK);\
			if((value&0x8000)==0x8000)\
			__gpio_set_pin(SPDA);\
			else\
			__gpio_clear_pin(SPDA);\
			udelay(25);\
			__gpio_set_pin(SPCK);\
			value=(value<<1); \
			udelay(25);\
		 }\
		__gpio_set_pin(SPEN);\
		udelay(100);\
	} while (0)

#define __spi_write_reg(reg, val) \
	do {\
		__spi_write_reg1((reg<<2|2), val);\
		udelay(100); \
	}while(0)

#define __lcd_special_pin_init() \
	do { \
		__gpio_as_output(SPEN); /* use SPDA */\
		__gpio_as_output(SPCK); /* use SPCK */\
		__gpio_as_output(SPDA); /* use SPDA */\
		__gpio_as_output(LCD_RET);\
	} while (0)

#define __lcd_special_on() \
	do { \
		udelay(50);\
		__gpio_clear_pin(LCD_RET);\
		udelay(150000);\
		__gpio_set_pin(LCD_RET);\
		udelay(10000);\
		__spi_write_reg(0x00, 0x03); \
		__spi_write_reg(0x01, 0x40); \
		__spi_write_reg(0x02, 0x11); \
		__spi_write_reg(0x03, MODE); /* mode */ \
		__spi_write_reg(0x04, 0x32); \
		__spi_write_reg(0x05, 0x0e); \
		__spi_write_reg(0x07, 0x03); \
		__spi_write_reg(0x08, 0x08); \
		__spi_write_reg(0x09, 0x40); \
		__spi_write_reg(0x0A, 0x88); \
		__spi_write_reg(0x0B, 0x88); \
		__spi_write_reg(0x0C, 0x20); \
		__spi_write_reg(0x0D, 0x20); \
	} while (0)	//reg 0x0a is control the display direction:DB0->horizontal level DB1->vertical level
	
#define __lcd_special_off() \
	do { \
		__spi_write_reg(0x00, 0x03); \
	} while (0)

#define __lcd_set_backlight_level(n) \
do { \
	__gpio_as_pwm(); \
	REG_PWM_DUT(0) = n; \
	REG_PWM_PER(0) = 7; \
	REG_PWM_CTR(0) = 0xc1; \
} while (0)

#define __lcd_close_backlight() \
do { \
__gpio_as_output(GPIO_PWM0);\
__gpio_clear_pin(GPIO_PWM0);\
} while (0)

#define __lcd_display_pin_init() \
do { \
	__gpio_as_output(GPIO_DISP_OFF_N); \
	__lcd_set_backlight_level(8); \
	__lcd_special_pin_init(); \
} while (0)

#define __lcd_display_on() \
do { \
	__gpio_set_pin(GPIO_DISP_OFF_N); \
	__lcd_special_on(); \
	__lcd_set_backlight_level(8); \
} while (0)

#define __lcd_display_off() \
do { \
	__lcd_special_off(); \
	__lcd_close_backlight(); \
	__gpio_clear_pin(GPIO_DISP_OFF_N); \
} while (0)

void lcd_board_init(void)
{
	__lcd_display_pin_init();
	__lcd_display_on();
//	__lcd_display_off();
}
#endif
