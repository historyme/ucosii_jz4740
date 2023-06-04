/*
 * ak4182.c using national microwire protocol
 *
 * Touch screen driver interface to the AK4182A .
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <sysdefs.h>
#include <regs.h>
#include <ops.h>
#include "jztouch.h"
#include <ucos_ii.h>

#define TS_PIN  (98)


#define TS_SAMPLE_TIME (20)     
#define X_CH 	5   
#define Y_CH	1
#define BAT_CH 2

#define TOUCH_TASK_PRIO	3
#define JZ_EXTAL (12*1000*1000)

#define TS_IRQ  (IRQ_GPIO_0 + TS_PIN)

#define TS_SAMPLE_TICK (TS_SAMPLE_TIME / (1000 / OS_TICKS_PER_SEC))

#define ts_gpio_mask_irq(n) (REG_GPIO_GPIMR(n / 32) |= (1 << (n % 32)))

#define ts_gpio_unmask_irq(n) (REG_GPIO_GPIMR((n / 32)) &= ~(1 << (n % 32)))

#define ts_gpio_ack_irq(n) (REG_GPIO_GPFR((n / 32)) |= (1 << (n % 32)))

#define TS_enable_irq()                \
do{                                    \
	__gpio_unmask_irq(TS_PIN);       \
	__intc_unmask_irq(IRQ_SSI);      \
}while(0)

#define TS_disable_irq()              \
do{                                   \
	__gpio_mask_irq(TS_PIN);    \
	__intc_mask_irq(IRQ_SSI);   \
}while(0)

#define TASK_STK_SIZE	1024
static OS_EVENT *touchEvent;

static void jz47ssi_handler(unsigned int arg)
{
	__intc_mask_irq(IRQ_SSI);
	//OSSemPost((OS_EVENT *)arg);
	OSSemPost(touchEvent);
}
static void touchpen_handler(unsigned int arg)
{
	__gpio_mask_irq(TS_PIN);
//	OSSemPost((OS_EVENT *)arg);
	OSSemPost(touchEvent);
}
#define ak4182_ssi_reset()           \
do{				     \
	REG_SSI_CR0 = 0x0000;        \
	REG_SSI_CR1 = 0x00007960;    \
	REG_SSI_SR  = 0x00000098;    \
	REG_SSI_ITR = 0x0000;        \
	REG_SSI_ICR = 0x00;          \
	REG_SSI_GR  = 0x0000;        \
	__ssi_disable();             \
	__ssi_flush_fifo();          \
	__ssi_clear_errors();        \
	__ssi_select_ce();           \
}while(0)

#define ak4182_ssi_enable() __ssi_enable()
#define ak4182_ssi_disable() __ssi_disable()
#define ak4182_ssi_set_trans_mode_format() \
do{                                        \
	__ssi_microwire_format();                \
	__ssi_set_msb();                         \
	__ssi_set_microwire_command_length(8);   \
	__ssi_set_frame_length(12);              \
}while(0)
#define  ak4182_ssi_set_clk_div_ratio(dev_clk,ssi_clk)	__ssi_set_clk(dev_clk, ssi_clk)

//#define ak4182_ssi_set_normal_mode()	__ssi_normal_mode()

/*------------------ AK4182 routines ------------------*/
#define ak4182_reg_write(val)	__ssi_transmit_data(val)


#define ak4148_ssi_irq_set() \
do{                          \
	__ssi_set_rx_trigger(1);   \
	__ssi_enable_rx_intr();    \
	__ssi_disable_tx_intr();   \
}while(0);

#define ak4182_reg_read() __ssi_receive_data()



static U8 adStatus = 0;
static U16 ts_TimeOut = 0;
static U16 AdFlag = 0;
#define RETRY_COUNT 3


#define ADC_STARTBITNUM (1 << 7)  
#define ADC_CHANNEL(x) (x << 4)
#define ADC_MODE(x)    (x << 3)
#define ADC_SER(x)     (x << 2)
#define ADC_PD(y)      (y)

#define ADC_COMMAND(x,y,m) (ADC_STARTBITNUM | \
			    ADC_CHANNEL(x)  | \
			    ADC_MODE(0)     | \
			    ADC_SER(m)      | \
			    ADC_PD(y))

#define TOUCH_PD_MODE   2
#define ADC_PD_MODE     3
static PFN_CALIBRATE xCalibrate = 0;
static PFN_CALIBRATE yCalibrate = 0;
static PFN_CALIBRATE AdCalibrate = 0;

#define SETFLAG(x,val)        \
do{			      \
	x &= 0xf0;            \
	x |= val;             \
}while(0)

static void touchTaskEntry(void *arg)
{
	U8 err;
	U32 val;
	U8 count;
	U8 retrycount;
	U8 touch_pen = 0;
	while(1)
	{
		OSSemPend(touchEvent,ts_TimeOut,&err);
		switch(adStatus)
		{
			case 0:
				if(__gpio_get_pin(TS_PIN) == 0)
				{
					ts_gpio_ack_irq(TS_PIN);
					ts_gpio_mask_irq(TS_PIN);
					__gpio_mask_irq(TS_PIN);
					touch_pen = 1;
					ts_TimeOut = 2;
					adStatus = 1;
					count = 0;
					ak4182_reg_write((U32)ADC_COMMAND(X_CH,TOUCH_PD_MODE,0));
					retrycount = 0;
					
					
					//printf("pen down\r\n");
			      	}else
				{
					if(err == OS_NO_ERR)
					{
						
						if(__ssi_get_rxfifo_count())
							val = ak4182_reg_read();
						ts_TimeOut = 0;
						count = 0;
						retrycount = 0;
						ts_gpio_ack_irq(TS_PIN);
						ts_gpio_unmask_irq(TS_PIN);
						__gpio_unmask_irq(TS_PIN);
						
						//printf("unuse data \r\n");
						
					}
					if((touch_pen == 1)&&(__gpio_get_pin(TS_PIN) != 0))
					{
						touch_pen = 0;
						if(xCalibrate) xCalibrate(-1);
						if(yCalibrate) yCalibrate(-1);
					}	
				}
				
				break;	
			case 1:  //x_ch preCalibrate
				
				if(err == OS_NO_ERR)
				{	
					val = ak4182_reg_read();
					count++;
					
					retrycount = 0;
					if(count > 2)
					{
						adStatus = 2;
						count = 0;
					}

				}else if(__gpio_get_pin(TS_PIN) != 0)
				{
					ts_TimeOut = 0;
					adStatus = 0;
					retrycount = 0;
					touch_pen = 0;
					if(xCalibrate) xCalibrate(-1);
					if(yCalibrate) yCalibrate(-1);
					//printf("pen up\r\n");
					ts_gpio_ack_irq(TS_PIN);
					ts_gpio_unmask_irq(TS_PIN);
					__gpio_unmask_irq(TS_PIN);
					
					if(__ssi_get_rxfifo_count())
						val = ak4182_reg_read();
					
					
				} else
					retrycount++;
				
				
				if(adStatus != 0)
				{
					if(retrycount < RETRY_COUNT)
						ak4182_reg_write(ADC_COMMAND(X_CH,TOUCH_PD_MODE,0));
					else
					{
						retrycount = 0;
						ts_TimeOut = 0;
						adStatus = 0;
						ak4182_reg_write(ADC_COMMAND(0,0,1));
						
					}	
				}			
				break;
			case 2: //x_ch Calibrate
				if(err == OS_NO_ERR)
				{	
					val = (val + ak4182_reg_read()) / 2;
					
					count++;
					retrycount = 0;
					if(count > 2)
					{
						ak4182_reg_write(ADC_COMMAND(Y_CH,TOUCH_PD_MODE,0));			
						if(xCalibrate) xCalibrate((U16)val);
						adStatus = 3;
						count = 0;
					}else
						ak4182_reg_write(ADC_COMMAND(X_CH,TOUCH_PD_MODE,0));
				}else
				{	
					retrycount++;
					if(retrycount < RETRY_COUNT)
						ak4182_reg_write(ADC_COMMAND(X_CH,TOUCH_PD_MODE,0));
					else
					{
						retrycount = 0;
						ts_TimeOut = 0;
						adStatus = 0;
						ak4182_reg_write(ADC_COMMAND(0,0,1));
					}	
				}
				break;
			case 3: // Y_CH preCalibrate
				if(err == OS_NO_ERR)
				{	
						val = ak4182_reg_read();
						count++;
						retrycount = 0;
						if(count > 2)
						{
							adStatus = 4;
							count = 0;
						}
				}else
					retrycount++;
				if(retrycount < RETRY_COUNT)
					ak4182_reg_write(ADC_COMMAND(Y_CH,TOUCH_PD_MODE,0));
				else
				{
					retrycount = 0;
					ts_TimeOut = 0;
					adStatus = 0;
					ak4182_reg_write(ADC_COMMAND(0,0,1));
				}
				break;
			case 4: //Y_CH Calibrate
				if(err == OS_NO_ERR)
				{	
						val = (val + ak4182_reg_read()) / 2;
						count++;
						
						if(count > 2)
						{
								//Calibrate PD Down
								ak4182_reg_write(ADC_COMMAND(0,0,1));			
								
								if(yCalibrate) yCalibrate((U16)val);
								adStatus = 5;
								count = 0;
								
						}else
							ak4182_reg_write(ADC_COMMAND(Y_CH,TOUCH_PD_MODE,0));
				}else
				{	
					retrycount++;
					if(retrycount < RETRY_COUNT)
						ak4182_reg_write(ADC_COMMAND(Y_CH,TOUCH_PD_MODE,0));
					else
					{
						retrycount = 0;
						adStatus = 0;
						ts_TimeOut = 0;
						ak4182_reg_write(ADC_COMMAND(0,0,1));
					}	
				}
				break;
			case 5:  //wait sample period
					if(err == OS_NO_ERR)
						val = ak4182_reg_read();
					__ssi_flush_fifo();
					ts_TimeOut = TS_SAMPLE_TICK;
					adStatus = 1;
					count = 0;
					break;	
		}
		if((AdFlag & 0x80) && ((adStatus == 0)||(adStatus == 6)))
		{
			ts_gpio_mask_irq(TS_PIN);
			printf("No Touch Calibrate\r\n");
			switch(AdFlag & 0x70)
			{
			case 0x10:
				ak4182_reg_write(ADC_COMMAND(AdFlag & 0x0f,ADC_PD_MODE,1));
				adStatus = 6;
				count = 0;
				ts_TimeOut = 1;
				AdFlag &= 0x8f;
				AdFlag |= 0x20;
				retrycount = 0;
				break;
			case 0x20: //preCalibrate
				if(err == OS_NO_ERR)
				{	
					val = ak4182_reg_read();
					count++;
					if(count > 2)
					{
						AdFlag &= 0x8f;
						AdFlag |= 0x30;
						count = 0;
					}
				}else
					retrycount++;
				if(retrycount < RETRY_COUNT)	
					ak4182_reg_write(ADC_COMMAND(AdFlag & 0x0f,ADC_PD_MODE,1));
				else
				{
					retrycount = 0;
					AdFlag = 0;
					ts_TimeOut = 0;
					ak4182_reg_write(ADC_COMMAND(0,0,1));
				}
				break;
			case 0x30:
				if(err == OS_NO_ERR)
				{	
					val = (val + ak4182_reg_read()) / 2;
					count++;
					if(count > 3)
					{
						AdFlag &= 0x8f;
						AdFlag |= 0x40;
						count = 0;
						if(AdCalibrate) AdCalibrate((U16)val);
						__gpio_unmask_irq(TS_PIN);
					}
					ak4182_reg_write(ADC_COMMAND(AdFlag & 0x0f,ADC_PD_MODE,1));
					
				}else
				{
					retrycount++;
					if(retrycount < RETRY_COUNT)
						ak4182_reg_write(ADC_COMMAND(AdFlag & 0x0f,ADC_PD_MODE,1));
					else
					{
						count = 0;
						AdFlag = 0;
						ts_TimeOut = 0;
						ak4182_reg_write(ADC_COMMAND(0,0,0));
					}
				}
				break;
			case 0x40:
				if(err == OS_NO_ERR)
					val = ak4182_reg_read();
				
				AdFlag = 0;
				adStatus = 0;
				ts_TimeOut = 0;
				ts_gpio_unmask_irq(TS_PIN);
				break;
			}
		}
		__intc_unmask_irq(IRQ_SSI);;
	}
}
#define TASK_STK_SIZE	1024
static OS_STK touchTaskStack[TASK_STK_SIZE];
int TS_init(void)
{
	touchEvent = OSSemCreate(1);
	__gpio_as_ssi();
	
	ak4182_ssi_reset();

	ak4182_ssi_set_trans_mode_format();
	REG_SSI_ITR = 1;
	ak4182_ssi_set_clk_div_ratio(JZ_EXTAL, 125*1000);//DCLK is 125K Hz max
	
	ak4148_ssi_irq_set();

	request_irq(IRQ_SSI, jz47ssi_handler, (unsigned int)touchEvent);
	

        //touch_pen_init
	__gpio_as_irq_fall_edge(TS_PIN);

	/* register irq handler */

	request_irq(TS_IRQ, touchpen_handler,(unsigned int)touchEvent);
	OSTaskCreate(touchTaskEntry, (void *)0,
		     (void *)&touchTaskStack[TASK_STK_SIZE - 1],
		     TOUCH_TASK_PRIO);
	ts_gpio_ack_irq(TS_PIN); 
	
	TS_enable_irq();
	
	ak4182_ssi_enable();
	ak4182_reg_write(ADC_COMMAND(0,0,1));
	printf("AK4182 touch screen driver initialized\n");
	return 0;
}
void TS_SetxCalibration(PFN_CALIBRATE xCal,PFN_CALIBRATE yCal)
{
	xCalibrate = xCal;
	yCalibrate = yCal;
}
