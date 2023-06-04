/**************************************************************************
*                                                                         *
*   PROJECT     :                                
*                                                                         *
*   MODULE      :                                                
*                                                                         *
*   AUTHOR      : Regen     email: lhhuang@ingenic.cn                               
*                                                                         *
*   PROCESSOR   : jz4730                                                  *
*                                                                         *
*   TOOL-CHAIN  :                                                         *
*                                                                         *
*   DESCRIPTION :                                                         *
*   This is a sample code to test LWIP on  uC/OS-II                       *
*                                                                         *
**************************************************************************/



#include "includes.h"
#include <stdio.h>
#include "app_cfg.h"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/
//TASK PRIORITY defined in ucosii/src/app_cfg.h
#define  HTTP_PORT        80
#define  TELNET_PORT      23 // the port telnet users will be connecting to
#define  BACKLOG          10 // how many pending connections queue will hold in telnet

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/
OS_STK   TaskStartStk[APP_TASK_START_STK_SIZE];

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void TaskStart (void *data);


void lcd_test()
{
	unsigned short *frame;
	unsigned int i;
	
	
	frame = lcd_get_frame_one();
	for(i = 0;i < 10;i++)
	{
		printf("lcd test = 0x%x \r\n",frame[i * 480]);
	}


	for(i = 0;i < 480 * 272 / 3;i++)
	{
		frame[i] = 0xf800;
	}
	for(;i < 480 * 272 / 3 * 2;i++)
	{
		frame[i] = 0x7e0;
	}
	for(;i < 480 * 272 / 3 * 3;i++)
	{
		frame[i] = 0x1f;
	}
	for(i = 0;i < 10;i++)
	{
		printf("lcd test = 0x%x \r\n",frame[i * 480]);
	}
 	
	
}


void lcd_test1()
{
	unsigned short *frame;
	unsigned int i;
	
	
	frame = lcd_get_frame_two();
	for(i = 0;i < 10;i++)
	{
		printf("lcd test = 0x%x \r\n",frame[i * 480]);
	}


	for(i = 0;i < 480 * 272 / 3;i++)
	{
		frame[i] = 0x1f;
	}
	for(;i < 480 * 272 / 3 * 2;i++)
	{
		frame[i] = 0x7e0;
	}
	for(;i < 480 * 272 / 3 * 3;i++)
	{
		frame[i] = 0xf800;
	}
	for(i = 0;i < 10;i++)
	{
		printf("lcd test = 0x%x \r\n",frame[i * 480]);
	}
 	
	
}

/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/



void APP_vMain (void)
{
   OS_STK *ptos;
   OS_STK *pbos;
   INT32U  size;

   //   OSTaskCreate(TaskStart, (void *)0, (void *)&TaskStartStk[TASK_STK_SIZE - 1], TASK_START_PRIO);
  
   ptos        = &TaskStartStk[APP_TASK_START_STK_SIZE - 1];
   pbos        = &TaskStartStk[0];
   size        = APP_TASK_START_STK_SIZE;
   OSTaskCreateExt(TaskStart,
                   (void *)0,
                   ptos,
                   TASK_START_PRIO,
                   TASK_START_ID,
                   pbos,
                   size,
                   (void *)0,
                   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

   OSStart();                              /* Start multitasking */
   while(1);
   FS_Exit();
}



void TaskStart (void *data)
{
  int x;
  data = data;                            // Prevent compiler warning 
  printf("\n\n  uC/OS-II, The Real-Time Kernel MIPS Ported version, ");
  printf("LWIP EXAMPLE 2.0\n\n");
  jzlcd_init();

  JZ_StartTicker(OS_TICKS_PER_SEC);	// OS_TICKS_PER_SEC config in os_cfg.h 
  OSStatInit();                           // Initialize uC/OS-II's statistics 
   lcd_test();
   lcd_test1();
  while(1) {

    change();  
	OSTimeDlyHMSM(0, 0, 2, 0);                  // Wait one minute
    
//    OSTimeDlyHMSM(0, 0, 2, 0);                  // Wait one minute
	printf("%d tasks running, ", OSTaskCtr);    // Display #tasks running 
    printf("CPU usage %d%, ", OSCPUUsage);      // Display CPU usage in % 
    printf("context switches %ld times\n", OSCtxSwCtr); // Display #context switches 
    OSCtxSwCtr = 0;
  }

}
