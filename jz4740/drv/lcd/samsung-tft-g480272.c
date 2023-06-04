#if LCDTYPE == 1
#include <jz4740.h>

#define DISPOFF	(3 * 32 + 22)
#define LCD_RESET (2*32 + 23)
/*
#ifdef PMPVERSION == 1
#define LCD_RESET 63
#endif

#ifdef PMPVERSION == 22
#define LCD_RESET 60
#endif
*/

void lcd_board_init(void)
{
	u32 val;
	__gpio_as_output(DISPOFF);
	__gpio_set_pin(LCD_RESET);
	__gpio_as_output(LCD_RESET);
	__gpio_clear_pin(LCD_RESET);
	udelay(1000);
	__gpio_set_pin(LCD_RESET);

	
	__gpio_as_pwm4();
	__tcu_disable_pwm_output(4);
	__tcu_init_pwm_output_high(4);
	__tcu_select_clk_div1(4);

	__tcu_mask_half_match_irq(4);
	__tcu_mask_full_match_irq(4);

	val = __cpm_get_extalclk();
	val /= 900;

	__tcu_set_full_data(4,val/2);
	__tcu_set_half_data(4,val);
	__tcu_start_counter(4);
	__tcu_enable_pwm_output(4);
	__gpio_set_pin(DISPOFF);
}

#endif
