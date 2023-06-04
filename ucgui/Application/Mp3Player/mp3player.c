/*
*********************************************************************************************************
*                                                uC/GUI
*                        Universal graphic software for embedded applications
*
*                       (c) Copyright 2002, Micrium Inc., Weston, FL
*                       (c) Copyright 2002, SEGGER Microcontroller Systeme GmbH
*
*              µC/GUI is protected by international copyright laws. Knowledge of the
*              source code may not be used to write a similar product. This file may
*              only be used in accordance with a license and should not be redistributed
*              in any way. We appreciate your understanding and fairness.
*
----------------------------------------------------------------------
File        : WIDGET_Multiedit.c
Purpose     : Demonstrates the use of the MULTIEDIT widget.
----------------------------------------------------------------------
*/

#include "GUI.h"
#include "MULTIEDIT.h"
#include "FRAMEWIN.h"
#include "fs_api.h"
#include <string.h>

/*********************************************************************
*
*       defines
*
**********************************************************************
*/

#define SPEED         1000

#define MIN_SIZE_X    200
#define MIN_SIZE_Y     80

#define RESIZE_X      (1<<0)
#define RESIZE_Y      (1<<1)
#define REPOS_X       (1<<2)
#define REPOS_Y       (1<<3)

#define ID_NONEWRAP   (GUI_ID_USER + 0)
#define ID_WORDWRAP   (GUI_ID_USER + 1)
#define ID_CHARWRAP   (GUI_ID_USER + 2)
#define ID_OVERWRITE  (GUI_ID_USER + 3)
#define ID_READONLY   (GUI_ID_USER + 4)
#define ID_PASSWORD   (GUI_ID_USER + 5)
#define ID_TXT (GUI_ID_USER + 6)

/*********************************************************************
*
*       static data
*
**********************************************************************
*/

static WM_CALLBACK* _pcbFrameWin;
static WM_CALLBACK* _pcbFrameWinClient;
static WM_CALLBACK* _pcbMultiEdit;
static WM_HWIN      _hMEdit, _hFrame = 0, _hClient;
static WM_HWIN      _hWrapButton;
static int          _CaptureX;
static int          _CaptureY;
static int          _HasCaptured;
static int          _ReadOnly;
static int          _Overwrite;
static int          _Password;
static char         _acInfoText[100] = {0};


/*********************************************************************
*
*       _SetCapture
*/
static void _SetCapture(FRAMEWIN_Handle hWin, int x, int y, int Mode) {
  if ((_HasCaptured & REPOS_X) == 0) {
    _CaptureX = x;
  }
  if ((_HasCaptured & REPOS_Y) == 0) {
    _CaptureY = y;
  }
  if (!_HasCaptured) {
    WM_SetCapture(hWin, 1);
    _HasCaptured = Mode;
  }
}

/*********************************************************************
*
*       _ChangeWindowPosSize
*/
static void _ChangeWindowPosSize(FRAMEWIN_Handle hWin, int* px, int* py) {
  int dx = 0, dy = 0;
  GUI_RECT Rect;
  WM_GetClientRectEx(hWin, &Rect);
  /* Calculate new size of window */
  if (_HasCaptured & RESIZE_X) {
    dx = (_HasCaptured & REPOS_X) ? (_CaptureX - *px) : (*px - _CaptureX);
  }
  if (_HasCaptured & RESIZE_Y) {
    dy = (_HasCaptured & REPOS_Y) ? (_CaptureY - *py) : (*py - _CaptureY);
  }
  /* Check the minimal size of window */
  if ((Rect.x1 + dx + 1) < MIN_SIZE_X) {
    dx = MIN_SIZE_X - (Rect.x1 + 1);
    *px = _CaptureX;
  }
  if ((Rect.y1 + dy + 1) < MIN_SIZE_Y) {
    dy = MIN_SIZE_Y - (Rect.y1 + 1);
    *py = _CaptureY;
  }
  /* Set new window position */
  if (_HasCaptured & REPOS_X) {
    WM_MoveWindow(hWin, -dx, 0);
  }
  if (_HasCaptured & REPOS_Y) {
    WM_MoveWindow(hWin, 0, -dy);
  }
  /* Set new window size */
  WM_ResizeWindow(hWin, dx, dy);
}

/*********************************************************************
*
*       _OnTouch
*/
static int _OnTouch(FRAMEWIN_Handle hWin, WM_MESSAGE* pMsg) {
  if (pMsg->Data.p) {  /* Something happened in our area (pressed or released) */
    GUI_PID_STATE* pState;
    pState = (GUI_PID_STATE*)pMsg->Data.p;
    if (pState->Pressed) {
      int x, y;
      x = pState->x;
      y = pState->y;
      if (WM_HasCaptured(hWin) == 0) {
        GUI_RECT Rect;
        int Mode = 0;
        int BorderSize = 4;
        WM_GetClientRectEx(hWin, &Rect);
        if (x > (Rect.x1 - BorderSize)) {
          Mode |= RESIZE_X;
        } else if (x < BorderSize) {
          Mode |= RESIZE_X | REPOS_X;
        }
        if (y > (Rect.y1 - BorderSize)) {
          Mode |= RESIZE_Y;
        } else if (y < BorderSize) {
          Mode |= RESIZE_Y | REPOS_Y;
        }
        if (Mode) {
          WM_SetFocus(hWin);
          WM_BringToTop(hWin);
          _SetCapture(hWin, x, y, Mode);
          return 1;
        }
      } else if (_HasCaptured) {
        _ChangeWindowPosSize(hWin, &x, &y);
        _SetCapture(hWin, x, y, 0);
        return 1;
      }
    }
  }
  _HasCaptured = 0;
  return 0;
}

/*********************************************************************
*
*       _CreateLButton
*/
static WM_HWIN _CreateLButton(const char* pText, int x, int w, int h, WM_HWIN hParent, int Id) {
  WM_HWIN hButton;
  GUI_RECT Rect;
  WM_GetClientRectEx(hParent, &Rect);
  hButton = BUTTON_CreateEx(x, Rect.y1 - h + 1, w, h, hParent, WM_CF_SHOW | WM_CF_ANCHOR_BOTTOM, 0, Id);
  BUTTON_SetText(hButton, pText);
  BUTTON_SetFont(hButton, &GUI_Font8_ASCII);
  return hButton;
}

/*********************************************************************
*
*       _CreateRButton
*/
static WM_HWIN _CreateRButton(const char* pText, int x, int w, int h, WM_HWIN hParent, int Id) {
  WM_HWIN hButton;
  GUI_RECT Rect;
  WM_GetClientRectEx(hParent, &Rect);
  hButton = BUTTON_CreateEx(Rect.x1 - x - w + 1, Rect.y1 - h + 1, w, h, hParent,
                            WM_CF_SHOW | WM_CF_ANCHOR_BOTTOM | WM_CF_ANCHOR_RIGHT, 0, Id);
  BUTTON_SetText(hButton, pText);
  BUTTON_SetFont(hButton, &GUI_Font8_ASCII);
  return hButton;
}

/*********************************************************************
*
*       _SetButtonState
*/
static void _SetButtonState(WM_HWIN hButton, int State) {
  if (State) {
    BUTTON_SetTextColor(hButton, 0, 0x0040F0);
    BUTTON_SetTextColor(hButton, 1, 0x0040F0);
  } else {
    BUTTON_SetTextColor(hButton, 0, GUI_BLACK);
    BUTTON_SetTextColor(hButton, 1, GUI_BLACK);
  }
}

/*********************************************************************
*
*       static code, callbacks
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbFrameWin
*/
static void _cbFrameWin(WM_MESSAGE* pMsg) {
  switch(pMsg->MsgId) {
  case WM_TOUCH:
    if (FRAMEWIN_IsMinimized(pMsg->hWin) == 0) {
      if (_OnTouch(pMsg->hWin, pMsg)) {
        return;
      }
    }
    break;
  case WM_DELETE:
    _hFrame  = 0;
    _hClient = 0;
    _hMEdit  = 0;
    _hWrapButton = 0;
	_pcbFrameWin = 0;
	printf("on wm_delete!\n");
	
    break;
  }
  if (_pcbFrameWin) {

	  (*_pcbFrameWin)(pMsg);
  }
}

/*********************************************************************
*
*       _cbFrameWinClient
*/
static void _cbFrameWinClient(WM_MESSAGE* pMsg) {
  switch(pMsg->MsgId) {
  case WM_NOTIFY_PARENT:
    if (pMsg->Data.v == WM_NOTIFICATION_RELEASED) {
      int Id = WM_GetId(pMsg->hWinSrc);
      if ((Id >= ID_NONEWRAP) && (Id <= ID_CHARWRAP)) {
        _SetButtonState(_hWrapButton, 0);
        if (Id == ID_NONEWRAP) {
          MULTIEDIT_SetWrapNone(_hMEdit);
        } else if (Id == ID_WORDWRAP) {
          MULTIEDIT_SetWrapWord(_hMEdit);
        } else if (Id == ID_CHARWRAP) {
          MULTIEDIT_SetWrapChar(_hMEdit);
        }
        _hWrapButton = pMsg->hWinSrc;
        _SetButtonState(_hWrapButton, 1);
      } else if (Id == ID_OVERWRITE) {
        _Overwrite ^= 1;
        MULTIEDIT_SetInsertMode(_hMEdit, 1 - _Overwrite);
        _SetButtonState(pMsg->hWinSrc, _Overwrite);
      } else if (Id == ID_READONLY) {
        _ReadOnly ^= 1;
        MULTIEDIT_SetReadOnly(_hMEdit, _ReadOnly);
        _SetButtonState(pMsg->hWinSrc, _ReadOnly);
      } else if (Id == ID_PASSWORD) {
        _Password ^= 1;
        MULTIEDIT_SetPasswordMode(_hMEdit, _Password);
        _SetButtonState(pMsg->hWinSrc, _Password);
      }
    }
    return;
  }
  if (_pcbFrameWinClient) {
    (*_pcbFrameWinClient)(pMsg);
  }
}
static char info[128];
extern const GUI_FONT GUI_FontHZ12;
static void Mp3PlayFinish(void)
{
	if(_hFrame)
	{
		printf("send delete msg!\n");
	  WM_DeleteWindow(_hFrame);
	  //WM_MESSAGE Message;
	  //Message.MsgId  = WM_DELETE;
		//WM_SendMessage(_hFrame,&Message);
		
		
		
	}
}
/*********************************************************************
*
*        _DemoMp3
*/
void _DemoMp3(int handle,int *hparam,int *lparam) {
  GUI_RECT Rect;
  int WinFlags;
  if(_hFrame != 0)
	{
		GUI_MessageBox("File Browser not open 2!",
                  "File Browser", GUI_MESSAGEBOX_CF_MOVEABLE);
		return;
	}
  _hFrame = 1;
  _Overwrite = 0;
  _ReadOnly  = 0;
  _Password  = 0;
  
  /* Create framewin */
	if(lparam)
		_hFrame = FRAMEWIN_CreateEx(60, 80, 200, 120, handle, WM_CF_SHOW, 0, 0, (char *)lparam, 0);
	else
	  _hFrame = FRAMEWIN_CreateEx(60, 80, 200, 120, WM_HBKWIN, WM_CF_SHOW, 0, 0, "MP3 Player", 0);
  
  
  _hClient = WM_GetClientWindow(_hFrame);
  _pcbFrameWin       = WM_SetCallback(_hFrame,  _cbFrameWin);
  _pcbFrameWinClient = WM_SetCallback(_hClient, _cbFrameWinClient);
  /* Set framewin properties */
  FRAMEWIN_SetMoveable(_hFrame, 1);
  FRAMEWIN_SetActive(_hFrame, 1);
  FRAMEWIN_SetTextAlign(_hFrame, GUI_TA_HCENTER | GUI_TA_VCENTER);
  FRAMEWIN_SetFont(_hFrame, &GUI_FontHZ12);
  FRAMEWIN_SetTitleHeight(_hFrame, 16);
  /* Add framewin buttons */
  FRAMEWIN_AddCloseButton(_hFrame, FRAMEWIN_BUTTON_LEFT,  0);
  FRAMEWIN_AddMaxButton(_hFrame, FRAMEWIN_BUTTON_RIGHT, 0);
  FRAMEWIN_AddMinButton(_hFrame, FRAMEWIN_BUTTON_RIGHT, 1);
  WM_InvalidateWindow(_hFrame);
  
  /* Create multiedit */
  /*
  WinFlags = WM_CF_SHOW | WM_CF_ANCHOR_RIGHT | WM_CF_ANCHOR_LEFT | WM_CF_ANCHOR_TOP | WM_CF_ANCHOR_BOTTOM;
  WM_GetClientRectEx(_hClient, &Rect);
  _hMEdit = MULTIEDIT_CreateEx(0, 0, 0, Rect.y1 - 16 + 1, _hClient, WinFlags, MULTIEDIT_CF_INSERT, 0, 0, "");
  _pcbMultiEdit = WM_SetCallback(_hMEdit,  _cbMultiEdit);
  MULTIEDIT_SetAutoScrollH(_hMEdit, 1);
  MULTIEDIT_SetAutoScrollV(_hMEdit, 1);
  WM_SetFocus(_hMEdit);
  //MULTIEDIT_SetText(_hMEdit, "This sample demonstrates the use of a multiedit widget!");
  MULTIEDIT_SetFont(_hMEdit, &GUI_FontHZ12);
  MULTIEDIT_SetTextColor(_hMEdit, 0, 0xE00000);
  MULTIEDIT_SetBkColor(_hMEdit, 0, 0xD0FFFF);
  MULTIEDIT_SetWrapWord(_hMEdit);
  MULTIEDIT_SetHBorder(_hMEdit, 3);
  */
  if(lparam)
  {
	  	struct id3tag1 {
			char tag2[3];
			char title[30];
			char artist[30];
			char album[30];
			char year[4];
			char comment[30];
			unsigned char genre;
		}*mp3info;
  	WinFlags = WM_CF_SHOW | WM_CF_ANCHOR_LEFT;
  	int WinFlags2 = WM_CF_SHOW | WM_CF_ANCHOR_RIGHT;
  	
  	int dlen = MP3_Play((char *)lparam,info,Mp3PlayFinish);
  	mp3info = (struct id3tag1 *)info;
  	TEXT_Handle hTxt;
		WM_GetClientRectEx(_hClient, &Rect);
  	int cx = Rect.x1;
  	int x1 = 64;
  	int i;
  	hTxt = TEXT_CreateEx(0,0,64,14,_hClient,WinFlags,0,ID_TXT,"Title:");
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
		
		hTxt = TEXT_CreateEx(x1,0,cx,14,_hClient,WinFlags2,0,ID_TXT,mp3info->title);
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
//		for(i = 0;i < 30;i++)
//		{
//			printf("%x,%c\n",mp3info->title[i],mp3info->title[i]);
//		}
		hTxt = TEXT_CreateEx(0,20,64,14,_hClient,WinFlags,0,ID_TXT,"Artist:");
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
		hTxt = TEXT_CreateEx(x1,20,cx,14,_hClient,WinFlags2,0,ID_TXT,mp3info->artist);
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
		
		hTxt = TEXT_CreateEx(0,40,64,14,_hClient,WinFlags,0,ID_TXT,"Album:");
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
		hTxt =TEXT_CreateEx(x1,40,cx,14,_hClient,WinFlags2,0,ID_TXT,mp3info->album);
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
		
		hTxt = TEXT_CreateEx(0,60,64,14,_hClient,WinFlags,0,ID_TXT,"Year:");
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
		hTxt = TEXT_CreateEx(x1,60,cx,14,_hClient,WinFlags2,0,ID_TXT,mp3info->year);
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
		
		hTxt = TEXT_CreateEx(0,80,64,14,_hClient,WinFlags,0,ID_TXT,"Comment:");
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
		hTxt = TEXT_CreateEx(x1,80,64,14,_hClient,WinFlags2,0,ID_TXT,mp3info->comment);
		TEXT_SetFont(hTxt,&GUI_FontHZ12);
	  
	  
  }
  
  
//MULTIEDIT_SetPrompt(_hMEdit, "Type: ");
}

