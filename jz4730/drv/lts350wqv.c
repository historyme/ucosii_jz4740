#include <regs.h>
#include <ops.h>

#define DISPOFF	93

void lcd_board_init(void)
{
	__gpio_as_output(DISPOFF);
	__gpio_as_pwm();
	REG_PWM_DUT(0) = 8;
	REG_PWM_PER(0) = 7;
	REG_PWM_CTR(0) = 0x81;
	__gpio_set_pin(DISPOFF);
}

