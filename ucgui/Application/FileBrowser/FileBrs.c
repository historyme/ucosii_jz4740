#include <stddef.h>
#include <string.h>
#include "GUI.h"
#include "LISTVIEW.h"
#include "FRAMEWIN.h"
#include "fs_api.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define SPEED 650

#define MSG_CHANGE_MAIN_TEXT (WM_USER + 0)
#define MSG_CHANGE_INFO_TEXT (WM_USER + 1)

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/


extern const GUI_FONT GUI_FontHZ12;


static char brColumn_name[][10] = {"文件名","大小","类型","日期"};
static int brColumn_width[] = {200,80,80,100};

/*********************************************************************
*
*       static data
*
**********************************************************************
*/
#define MIN_SIZE_X    200
#define MIN_SIZE_Y     80

#define RESIZE_X      (1<<0)
#define RESIZE_Y      (1<<1)
#define REPOS_X       (1<<2)
#define REPOS_Y       (1<<3)

#define ID_COPYFILE   (GUI_ID_USER + 0)
#define ID_PASTEFILE   (GUI_ID_USER + 1)
#define ID_BACKDIR   (GUI_ID_USER + 2)
#define ID_NEWFILE  (GUI_ID_USER + 3)
#define ID_RENAME   (GUI_ID_USER + 4)
#define ID_FRONTDIR   (GUI_ID_USER + 5)

#define ID_FILE_BROWNSER (GUI_ID_USER + 6)

static LISTVIEW_Handle _hListView;
static WM_CALLBACK* _pcbFrameWin;
static WM_CALLBACK* _pcbFrameWinClient;
static WM_HWIN      _hFrame = 0,_hClient;
static WM_HWIN      _hCopyButton;
static int          _CaptureX;
static int          _CaptureY;
static int          _HasCaptured;

static void _show_directory(const char *name);
typedef struct
{
	unsigned int length;
	unsigned int relate_id;
	char fs_name[40];
}FS_ROW,*PFS_ROW;
#define MAX_FS_ROW 200

static FS_ROW fs_row[MAX_FS_ROW];
static int curfs_row = 0;
static char curpath[255];



/*********************************************************************
*
*       _Demo
*/
static void _Demo(char *str) {
  unsigned int i;
  HEADER_Handle hHeader;
  hHeader = LISTVIEW_GetHeader(_hListView);
  HEADER_SetFont(hHeader,&GUI_FontHZ12);
  WM_SetFocus(_hListView);
  for(i = 0; i < sizeof(brColumn_width) / 4;i++)
  {
	  if(i == 0)
		  LISTVIEW_AddColumn(_hListView, brColumn_width[i], brColumn_name[i],GUI_TA_LEFT);
	  else
		  LISTVIEW_AddColumn(_hListView, brColumn_width[i], brColumn_name[i],GUI_TA_CENTER);
  }

  _show_directory(str);
}
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
	 _hCopyButton = 0;
	 _pcbFrameWin = 0;
	 _hListView = 0;
    break;
  }
  if (_pcbFrameWin) {
    (*_pcbFrameWin)(pMsg);
  }
}

void _HandleSelectFile(int handle)
{
	int index = LISTVIEW_GetSel(_hListView);
	char fname[255];
	if(index != -1)
	{
		if(fs_row[index].relate_id == -2)
		{
				printf("open dir %s\n",fs_row[index].fs_name);
				_show_directory(fs_row[index].fs_name);		
		}
		else if(fs_row[index].relate_id != -1)
		{
				sprintf(fname,"%s\\%s",curpath,fs_row[index].fs_name);
				printf("open %s \n",fname);
				ExeRelateProg(fs_row[index].relate_id,handle,0,fname);
		}
		return;
	}
	return;
	//GUI_MessageBox(_aTable_1[index][0],
     //              "Caption/Title", GUI_MESSAGEBOX_CF_MOVEABLE);
}
/*********************************************************************
*
*       _cbFrameWinClient
*/
static void _cbFrameWinClient(WM_MESSAGE* pMsg) {
  switch(pMsg->MsgId) {
  case WM_NOTIFY_PARENT:
	if(pMsg->Data.v == WM_NOTIFICATION_SEL_CHANGED)
	{
		int Id = WM_GetId(pMsg->hWinSrc);
		if (Id == ID_FILE_BROWNSER)
		{
		  _HandleSelectFile(pMsg->hWin);
		}
	}
		
    if (pMsg->Data.v == WM_NOTIFICATION_RELEASED) {
      int Id = WM_GetId(pMsg->hWinSrc);
	  if ((Id >= ID_COPYFILE) && (Id <= ID_BACKDIR)) {
        if (Id == ID_COPYFILE) {

        } else if (Id == ID_PASTEFILE) {

        } else if (Id == ID_BACKDIR) {
        }
      } else if (Id == ID_NEWFILE) {
      } else if (Id == ID_RENAME) {
      } else if (Id == ID_FRONTDIR) {
      }
    }
    return;
  }
  if (_pcbFrameWinClient) {
    (*_pcbFrameWinClient)(pMsg);
  }
}


/*********************************************************************
*
*       _DemoFileBrownser
*/

static void _DemoFileBrowser(char *dir,int handle) {
  GUI_RECT Rect;
  int WinFlags;

  _hFrame = 1;

  /* Create framewin */
  if(dir)
	_hFrame = FRAMEWIN_CreateEx(60, 80, 200, 120, handle, WM_CF_SHOW, 0, 0, dir, 0);
  else
	_hFrame = FRAMEWIN_CreateEx(60, 80, 200, 120, handle, WM_CF_SHOW, 0, 0, "FileBrowser", 0);
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
  /* Create buttons */
  _hCopyButton = _CreateLButton("Copy",   0, 36, 16, _hClient, ID_COPYFILE);
                 _CreateLButton("Paste",  37, 36, 16, _hClient, ID_PASTEFILE);
                 _CreateLButton("Back",  74, 36, 16, _hClient, ID_BACKDIR);
                 _CreateRButton("Front",   52, 25, 16, _hClient, ID_FRONTDIR);
                 _CreateRButton("New",   26, 25, 16, _hClient, ID_NEWFILE);
                 _CreateRButton("Rename",    0, 25, 16, _hClient, ID_RENAME);
  //_SetButtonState(_hCopyButton, 1);
  /* Create multiedit */
  WinFlags = WM_CF_SHOW | WM_CF_ANCHOR_RIGHT | WM_CF_ANCHOR_LEFT | WM_CF_ANCHOR_TOP | WM_CF_ANCHOR_BOTTOM;
  
  WM_GetClientRectEx(_hClient, &Rect);
  _hListView = LISTVIEW_CreateEx(0, 0, 0, Rect.y1 - 16 + 1, _hClient, WinFlags, 1234,ID_FILE_BROWNSER);
  LISTVIEW_SetFont(_hListView,&GUI_FontHZ12);
  SCROLLBAR_CreateAttached(_hListView, SCROLLBAR_CF_VERTICAL);			
  _Demo(dir);
  
}


static unsigned int GetFileLen(char *fs)
{
	int ret = 0;
	char ss[255];
	FS_FILE *fp;
	sprintf(ss,"%s\\%s",curpath,fs); 
	fp = FS_FOpen(ss,"rb");
	if(fp)
	{
		ret = fp->size;
		FS_FClose(fp);
	}
	return ret;
}
static char *GetFileExeName(char *fs)
{
	int i,n;
	n = 0;
	for(i = 0;i < strlen(fs); i++)
	{
		if(fs[i] == '.')
			return &fs[i + 1]; 
	}
	return 0;
}
static void setFs_Row(char *s,unsigned int len,unsigned int id)
{
	strcpy(fs_row[curfs_row].fs_name,s); 
	fs_row[curfs_row].relate_id = id;
	fs_row[curfs_row].length = len;
	curfs_row++;
}

static char * trimspace(char *s)
{
	char *s1 = s;
	char *s2 = s;
	while(*s)
	{
		if(*s != ' ')
			*s2++ = *s;
		s++;
	}
	*s2 =  0;
	return s1;	
}
static char * dirstr(char *s)
{
	char *s1 = trimspace(s);
	s1[strlen(s1) -1] = 0;
	return s1;
}
static void copedir(char *t,char *s)
{
	int d;;
	if(t[0] == 0)
		strcpy(t , s);
	else
		sprintf(&t[strlen(t)],"\\%s",s);
    d = strlen(t);
	if(t[d - 1] == '\\')
		t[d - 1] = 0;
}
static char * updir(char *s)
{
	int i = strlen(s) - 1;
	for(;i>= 0;i--)
	{
		if(s[i] == '\\')
		{
			s[i] = 0;
			break;
		}
	}
	return s;

}
static char buf[80];
static GUI_ConstString s_buf[4] = {&buf[0],&buf[20],&buf[30],&buf[40]};  
	
static void _show_directory(const char *name) {	
	
	
	struct FS_DIRENT *direntp;
	int i,x;
	char sdir[12];
	FS_DIR *pDir = NULL;
	printf("_hListView:%x\n",_hListView);
	unsigned int d;
	
	if(strcmp(name,".") != 0)
	{
		if(strcmp(name,"..") == 0)
			updir(curpath);
		else
			copedir(curpath , name);	
	}
    
 
	pDir = FS_OpenDir(curpath);
	if (pDir) {

		d = LISTVIEW_GetNumRows(_hListView);
		if(d !=0 )		
		{
			for(i = 0; i < d;i++)
			  LISTVIEW_DeleteRow(_hListView,0);
		}
		curfs_row = 0;
		memset(buf,0,80);
		strcpy(s_buf[0],".");
		strcpy(s_buf[1]," ");
		strcpy(s_buf[2],"文件夹");
		strcpy(s_buf[3]," ");
		FRAMEWIN_SetText(_hFrame,curpath);
		
		do {
			direntp = FS_ReadDir(pDir);
			if (direntp) {
			 strcpy(sdir,direntp->d_name);
				if(direntp->FAT_DirAttr == 16)
				{
					sprintf(s_buf[0],"%s",dirstr(sdir));
					strcpy(s_buf[1]," ");
					sprintf(s_buf[2],"%s","文件夹");
					x = -2;
					d = -1;
				}else
				{
				  sprintf(s_buf[0],"%s",trimspace(sdir));
				  d = GetFileLen(s_buf[0]);
				  if(d < 1000)
						sprintf(s_buf[1],"%d",d);
				  else if(d < 1000000)
				     sprintf(s_buf[1],"%d kb",d / 1000);
				  else
					 sprintf(s_buf[1],"%d mb",d / 1000000);
			   x = -1;
			   x = SeekRelate(GetFileExeName(s_buf[0]));	 
				 if(x != -1)
				 {
				 		sprintf(s_buf[2],"%s",GetRelateDesc(x));
				 }else
				 		strcpy(s_buf[2]," ");
				 		 
			 }			 
       setFs_Row(s_buf[0],d,x);

			
			 LISTVIEW_AddRow(_hListView,s_buf); 
			}
		} while (direntp);
		FS_CloseDir(pDir);
	}
}
void OpenMMC(int handle,int *hParam,int *lParam)
{
	if(_hFrame != 0)
	{
		GUI_MessageBox("File Browser not open 2!",
                  "File Browser", GUI_MESSAGEBOX_CF_MOVEABLE);
		return;
	}
	memset(curpath,0,sizeof(curpath));
	_DemoFileBrowser("mmc:",handle);
	
}

void OpenNAND(int handle,int *hParam,int *lParam)
{
	if(_hFrame != 0)
	{
		GUI_MessageBox("File Browser not open 2!",
                  "File Browser", GUI_MESSAGEBOX_CF_MOVEABLE);
		return;
	}
	memset(curpath,0,sizeof(curpath));
	_DemoFileBrowser("nfl:",handle);
	
}
