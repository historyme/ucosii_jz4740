/*
*********************************************************************************************************
*                                                uC/GUI
*                        Universal graphic software for embedded applications
*
*                       (c) Copyright 2002, Micrium Inc., Weston, FL
*                       (c) Copyright 2002, SEGGER Microcontroller Systeme GmbH
*
*              礐/GUI is protected by international copyright laws. Knowledge of the
*              source code may not be used to write a similar product. This file may
*              only be used in accordance with a license and should not be redistributed
*              in any way. We appreciate your understanding and fairness.
*
----------------------------------------------------------------------
File        : Dialog_All.c
Purpose     : Example demonstrating DIALOG and widgets
----------------------------------------------------------------------
*/

#include <stddef.h>
//#include "includes.h"
#include "GUI.h"
#include "DIALOG.h"
#include "DROPDOWN.h"
#include "ucos_ii.h"
extern const GUI_BITMAP bmmp3;
typedef struct
{
	char    title[10];
	char    desc[60];
	WM_HWIN button;
	GUI_BITMAP *hBitMap_1;
	GUI_BITMAP *hBitMap_2;	
	void (*ExeProg)(int handle,int *hparam,int lparam);
}DesktopButton,*PDesktopButton;
extern void _DemoMultiedit(int handle,int *hparam,int *lparam);
extern void _DemoMp3(int handle,int *hparam,int *lparam);
extern void OpenMMC(int handle,int *hparam,int *lparam);
extern void OpenNAND(int handle,int *hparam,int *lparam);
extern void FiveChess(int handle,int *hparam,int *lparam);
#define TASK_STK_SIZE 1024 * 10
static OS_STK TaskStk[TASK_STK_SIZE];

#ifndef WIN32
  /* Stacks */
#endif
DesktopButton pButton[] = { 
	{"sd","SD文件系统演示",0,&bmmp3,&bmmp3,OpenMMC},
	{"nand","NANDFLASH文件系统演示",0,&bmmp3,&bmmp3,OpenNAND},
	{"mp3demo","播放MP3演示",0,&bmmp3,&bmmp3,_DemoMp3},
//	{"jpegdemo","播放JPEG文件",0,&bmmp3,&bmmp3,0},
	{"txtdemo","文本演示",0,&bmmp3,&bmmp3,_DemoMultiedit},
	{"game","五子棋游戏",0,&bmmp3,&bmmp3,FiveChess},
	{"udc","U盘功能演示",0,&bmmp3,&bmmp3,0},
    {0} 
};

extern const GUI_FONT GUI_FontHZ12;
int GetButtonID(WM_HWIN bt)
{
	int n = 0;
	while(1)
	{
		if(pButton[n].button == bt) 
			return n;
		n++;
	}
}
/*********************************************************************
*
*       _cbButton
*
* Purpose: 
*  1. Calls the owner draw function if the WM_PAINT message has been send
*  2. Calls the original callback for further messages
*  3. After processing the messages the function evaluates the pressed-state
*     if the WM_TOUCH message has been send
*/
//static void _cbButton(WM_MESSAGE *pMsg) {
// WM_HWIN hDlg;
// hDlg=pMsg->MsgId;
// if (pMsg->MsgId == WM_TOUCH) { 
//  
//  GUI_EndDialog(hDlg, 1);}
      
//}


/*******************************************************************
*
*       _cbWindow1
*/
int curbuttonid = -1;
static void _cbWindow1(WM_MESSAGE* pMsg) {
  int x, y,d;
  
  switch (pMsg->MsgId) {
  case WM_PAINT:
//    GUI_SetBkColor(_WindowColor1);
      
//    GUI_SetColor(GUI_WHITE);
//    GUI_SetFont(&GUI_Font24_ASCII);
	  if(curbuttonid != -1)
	  {
			GUI_SetColor(GUI_WHITE); 
			GUI_SetBkColor(GUI_BLUE);
			GUI_SetFont(&GUI_FontHZ12);		
			GUI_Clear(); 
			x = WM_GetWindowSizeX(pMsg->hWin);
			y = WM_GetWindowSizeY(pMsg->hWin);
			GUI_DispStringHCenterAt(pButton[curbuttonid].desc, x / 2, (y / 2) + 30);
		}
    break;
  case WM_NOTIFY_PARENT:
	  //#define WM_NOTIFICATION_CLICKED             1
      //#define WM_NOTIFICATION_RELEASED            2
	   if(pMsg->Data.v == WM_NOTIFICATION_RELEASED)
	   {
			x = WM_GetWindowSizeX(pMsg->hWin);
			y = WM_GetWindowSizeY(pMsg->hWin);
			d = GetButtonID(pMsg->hWinSrc);
			GUI_Clear();
			GUI_DispStringHCenterAt(pButton[d].desc, x / 2, (y / 2) + 30);
			curbuttonid = d;
			if(pButton[d].ExeProg)
			{
				//OSTaskCreate (pButton[d].ExeProg,pMsg->hWin, &TaskStk[TASK_STK_SIZE-1],20);
				printf("new 1 task\n");
				pButton[d].ExeProg(pMsg->hWin,0,0);
				
				 // CREATE_TASK(&aTCB[0],pButton[d].title,pButton[d].ExeProg,80,Stack_0);
			}
	   }
	  break;
  default:
    WM_DefaultProc(pMsg);
  }
}
void CreateButton(WM_HWIN Win1,PDesktopButton pButton,int cx,int cy)
{
	int i,x,y;
	int top = 10,left = 0;
	int size = 40;
	int space = 20;
	int bmsize = 32; 
	int width = 480;
	int height = 272;
	x = left;
	y = top;
    //sizeof(pButton)/sizeof(pButton[0])
	for(i = 0;i < 20;i++)
	{
		if(pButton[i].title[0] == 0)
			break;

		if(y + size < height)
		{
			pButton[i].button = BUTTON_CreateAsChild(x,y,size,size,Win1,10,WM_CF_SHOW);
			BUTTON_SetBitmapEx(pButton[i].button,0,pButton[i].hBitMap_1,(size - bmsize) / 2, 4);
			BUTTON_SetBitmapEx(pButton[i].button,1,pButton[i].hBitMap_2,(size - bmsize) / 2, 4);
			BUTTON_SetText(pButton[i].button,pButton[i].title);
			BUTTON_SetTextAlign(pButton[i].button,GUI_TA_BOTTOM | GUI_TA_HCENTER);
			//BUTTON_SetFont(pButton[i].button,&GUI_Font8x12_ASCII);
			//BUTTON_SetBkColor(pButton[i].button,0,GUI_GetBkColor());
			WM_Paint(pButton[i].button); 

		}
		x += size + space;
		if(x > width)
		{
			x = left;
			y += size + space;
		}
	}
}
void MainTask_DeskTop(void)
{
	int i;
	WM_HWIN Win1,Button1,Button2;
	GUI_Init();
	FS_Init();
	AddRelateFile("TXT","文本",_DemoMultiedit);
	AddRelateFile("MP3","音乐",_DemoMp3);
//	ucfs_sim_init();
//	Win1 = WM_CreateWindow(0,0,480,270,WM_CF_SHOW,NULL,0);
	Win1 = WM_CreateWindow(0,0,480,272,WM_CF_SHOW,_cbWindow1,0);	//houhh 20061024...
	
	WM_SelectWindow(Win1);
#if 1
    GUI_SetColor(GUI_WHITE); 
	GUI_SetBkColor(GUI_BLUE);
	GUI_SetFont(&GUI_FontHZ12);		
	GUI_Clear(); 
#endif	
	CreateButton(Win1,pButton,480,272);
	while (1) {
		if (!GUI_Exec())
			GUI_X_ExecIdle();
	}
//	ucfs_sim_deinit();
}


