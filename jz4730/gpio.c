/*
 * gpio.c
 *
 * Init GPIO pins for PMP board.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <regs.h>
#include <ops.h>

#define GPIO_PW_I         97
#define GPIO_PW_O         66
#define GPIO_LED_EN       92
#define GPIO_DISP_OFF_N   93
#define GPIO_RTC_IRQ      96
#define GPIO_USB_CLK_EN   29
#define GPIO_CHARG_STAT   125
#define GPIO_CIM_RST      69

void gpio_init(void)
{
	__gpio_as_uart0();
	__gpio_as_uart1();
	__gpio_as_uart2();
	__gpio_as_uart3();
	__gpio_as_emc();
	__gpio_as_dma();
	__gpio_as_msc();
	__gpio_as_lcd_master();
	__gpio_as_usb();
	__gpio_as_ac97();
	__gpio_as_cim();
	__gpio_as_eth();
	__harb_usb0_udc(); /* USB port 0 as device port */

	/* camera sensor reset  */
	__gpio_as_output(GPIO_CIM_RST);
	__gpio_set_pin(GPIO_CIM_RST);
	udelay(10);
	__gpio_clear_pin(GPIO_CIM_RST);

	/* First PW_I output high */
	__gpio_as_output(GPIO_PW_I);
	__gpio_set_pin(GPIO_PW_I);

	/* Then PW_O output high */
	__gpio_as_output(GPIO_PW_O);
	__gpio_set_pin(GPIO_PW_O);

	/* Last PW_I output low and as input */
	__gpio_clear_pin(GPIO_PW_I);
	__gpio_as_input(GPIO_PW_I);

	/* USB clock enable */
	__gpio_as_output(GPIO_USB_CLK_EN);
	__gpio_set_pin(GPIO_USB_CLK_EN);

	/* LED enable */
	__gpio_as_output(GPIO_LED_EN);
	__gpio_set_pin(GPIO_LED_EN);

	/* LCD display off */
	__gpio_as_output(GPIO_DISP_OFF_N);
	__gpio_clear_pin(GPIO_DISP_OFF_N);

	/* No backlight */
	__gpio_as_output(94); /* PWM0 */
	__gpio_clear_pin(94);

	/* RTC IRQ input */
	__gpio_as_input(GPIO_RTC_IRQ);

	/* make PW_I work properly */
	__gpio_disable_pullupdown(GPIO_PW_I);
}

