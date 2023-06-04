#if ((LCDTYPE == 2)||(LCDTYPE == 1))
#define DISPOFF	93
#define LCD_RESET 60

#if LCDTYPE == 1
#define LCD_RESET 63
#endif

#if LCDTYPE == 2
#define LCD_RESET 60
#endif



#include <regs.h>
#include <ops.h>

void lcd_board_init(void)
{
	__gpio_as_output(DISPOFF);
	__gpio_set_pin(LCD_RESET);
	__gpio_as_output(LCD_RESET);
	__gpio_clear_pin(LCD_RESET);
	udelay(1000);
	__gpio_set_pin(LCD_RESET);
	__gpio_as_pwm();
	
	REG_PWM_DUT(0) = 8;
	REG_PWM_PER(0) = 7;
	REG_PWM_CTR(0) = 0x81;
	
	__gpio_set_pin(DISPOFF);
}

#endif
