/*
*********************************************************************************************************
*                                                uC/GUI
*                        Universal graphic software for embedded applications
*
*                       (c) Copyright 2002, Micrium Inc., Weston, FL
*                       (c) Copyright 2002, SEGGER Microcontroller Systeme GmbH
*
*              �C/GUI is protected by international copyright laws. Knowledge of the
*              source code may not be used to write a similar product. This file may
*              only be used in accordance with a license and should not be redistributed
*              in any way. We appreciate your understanding and fairness.
*
----------------------------------------------------------------------
File        : LCDP565.C
Purpose     : Color conversion routines
---------------------------END-OF-HEADER------------------------------
*/

#include "LCD_Protected.h"    /* inter modul definitions */

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define B_BITS 5
#define G_BITS 6
#define R_BITS 5

#define R_MASK ((1 << R_BITS) -1)
#define G_MASK ((1 << G_BITS) -1)
#define B_MASK ((1 << B_BITS) -1)

/*********************************************************************
*
*       Public code,
*
*       LCD_FIXEDPALETTE == 565, 65536 colors, BBBBBGGGGGGRRRRR
*
**********************************************************************
*/
/*********************************************************************
*
*       LCD_Color2Index_565
*/
unsigned LCD_Color2Index_565(LCD_COLOR Color) {
  int r,g,b;
  b = (Color>> (8  - R_BITS)) & R_MASK;
  g = (Color>> (16 - G_BITS)) & G_MASK;
  r = (Color>> (24 - B_BITS)) & B_MASK;
  return r + (g << R_BITS) + (b << (G_BITS + R_BITS));
}

/*********************************************************************
*
*       LCD_GetIndexMask_565
*/
unsigned LCD_GetIndexMask_565(void) {
  return 0xffff;
}

/*************************** End of file ****************************/
