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

#define CAPTURE_TEST 0
#define NAND_TEST 0
#define NAND_UCFS_TEST 0
#define NAND_UCFS_TEST 0
#define MP3_TEST 1

#include "includes.h"
#include <stdio.h>

#if MP3_TEST
#include <../jz4730/sample/mp3test.c>
#endif

#if CAPTURE_TEST
#include <../jz4730/sample/capture.c>
#endif

#if NAND_TEST
#include <../jz4730/sample/nandtest.c>
#endif

#if NAND_UCFS_TEST
#include <../jz4730/sample/nanducfs.c>
#endif

#define  TASK_START_ID        0      /* Application tasks IDs                         */
#define  TASK_START_PRIO      10

#define APP_TASK_START_STK_SIZE   1024   /* Size of each task's stacks (# of WORDs) */
void TaskStart (void *data);

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/
OS_STK   TaskStartStk[APP_TASK_START_STK_SIZE];

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
   heapInit();
	
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

}

/*
*********************************************************************************************************
*                                                   Local functions
*********************************************************************************************************
*/


void TaskStart (void *data)
{
  int x;
  data = data;                            // Prevent compiler warning 
  printf("\n\n  uC/OS-II, The Real-Time Kernel MIPS Ported version, ");
  printf("LWIP EXAMPLE 2.0\n\n");

  JZ_StartTicker(OS_TICKS_PER_SEC);	// OS_TICKS_PER_SEC config in os_cfg.h 
  OSStatInit();                           // Initialize uC/OS-II's statistics 

#if CAPTURE_TEST
  capture_init(data);
#endif
#if NAND_TEST
  NandTest();
#endif
#if NAND_UCFS_TEST
  TestNANDUCFS();
#endif
#if MP3_TEST
  TestMp3_jz4730();
#endif

  while(1) {
    OSTimeDlyHMSM(0, 1, 0, 0);                  // Wait one minute
    printf("%d tasks running, ", OSTaskCtr);    // Display #tasks running 
    printf("CPU usage %d%, ", OSCPUUsage);      // Display CPU usage in % 
    printf("context switches %ld times\n", OSCtxSwCtr); // Display #context switches 
    OSCtxSwCtr = 0;
  }

}

#if 0
void ShowColorBar(void) {
	int x0 = 60, y0 = 40, yStep = 15, i;
	int NumColors = LCD_GetDevCap(LCD_DEVCAP_NUMCOLORS);
	int xsize = LCD_GetDevCap(LCD_DEVCAP_XSIZE) - x0;
	GUI_SetFont(&GUI_Font13HB_1);
	GUI_DispStringHCenterAt("UC/GUI-sample: Show color bars", 160, 0);
	GUI_SetFont(&GUI_Font8x8);
	GUI_SetColor(GUI_WHITE);
	GUI_SetBkColor(GUI_BLACK);
#if (LCD_FIXEDPALETTE)
	GUI_DispString("Fixed palette: ");
	GUI_DispDecMin(LCD_FIXEDPALETTE);
#endif
	GUI_DispStringAt("Red", 0, y0 + yStep);
	GUI_DispStringAt("Green", 0, y0 + 3 * yStep);
	GUI_DispStringAt("Blue", 0, y0 + 5 * yStep);
	GUI_DispStringAt("Gray", 0, y0 + 6 * yStep);
	GUI_DispStringAt("Yellow", 0, y0 + 8 * yStep);
	GUI_DispStringAt("Cyan", 0, y0 + 10 * yStep);
	GUI_DispStringAt("Magenta", 0, y0 + 12 * yStep);
	for (i = 0; i < xsize; i++) {
		U16 cs = (255 * (U32)i) / xsize;
		U16 x = x0 + i;;
/* Red */
		GUI_SetColor(cs);
		GUI_DrawVLine(x, y0 , y0 + yStep - 1);
		GUI_SetColor(0xff + (255 - cs) * 0x10100L);
		GUI_DrawVLine(x, y0 + yStep, y0 + 2 * yStep - 1);
/* Green */
		GUI_SetColor(cs<<8);
		GUI_DrawVLine(x, y0 + 2 * yStep, y0 + 3 * yStep - 1);
		GUI_SetColor(0xff00 + (255 - cs) * 0x10001L);
		GUI_DrawVLine(x, y0 + 3 * yStep, y0 + 4 * yStep - 1);
/* Blue */
		GUI_SetColor(cs * 0x10000L);
		GUI_DrawVLine(x, y0 + 4 * yStep, y0 + 5 * yStep - 1);
		GUI_SetColor(0xff0000 + (255 - cs) * 0x101L);
		GUI_DrawVLine(x, y0 + 5 * yStep, y0 + 6 * yStep - 1);
/* Gray */
		GUI_SetColor((U32)cs * 0x10101L);
		GUI_DrawVLine(x, y0 + 6 * yStep, y0 + 7 * yStep - 1);
/* Yellow */
		GUI_SetColor(cs * 0x101);
		GUI_DrawVLine(x, y0 + 7 * yStep, y0 + 8 * yStep - 1);
		GUI_SetColor(0xffff + (255 - cs) * 0x10000L);
		GUI_DrawVLine(x, y0 + 8 * yStep, y0 + 9 * yStep - 1);
/* Cyan */
		GUI_SetColor(cs * 0x10100L);
		GUI_DrawVLine(x, y0 + 9 * yStep, y0 + 10 * yStep - 1);
		GUI_SetColor(0xffff00 + (255 - cs) * 0x1L);
		GUI_DrawVLine(x, y0 + 10 * yStep, y0 + 11 * yStep - 1);
/* Magenta */
		GUI_SetColor(cs * 0x10001);
		GUI_DrawVLine(x, y0 + 11 * yStep, y0 + 12 * yStep - 1);
		GUI_SetColor(0xff00ff + (255 - cs) * 0x100L);
		GUI_DrawVLine(x, y0 + 12 * yStep, y0 + 13 * yStep - 1);
	}
}
#endif
