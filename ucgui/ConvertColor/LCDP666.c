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
File        : LCDP888.C
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

#define B_BITS 6
#define G_BITS 6
#define R_BITS 6

#define R_MASK (((1 << R_BITS) -1) << 2)
#define G_MASK (((1 << G_BITS) -1) << 2)
#define B_MASK (((1 << B_BITS) -1) << 2)

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
*       LCD_Color2Index_666
*/
unsigned LCD_Color2Index_666(LCD_COLOR Color) {
	
  //printf("LCD_Color2Index_666 \r\n"); 
    unsigned int r,g,b;
   
	r = (Color >>  0) & 0xff;
        //if(r + 2 < 0xff)
	//	r += 2;
        g = (Color >>  8) & 0xff;
	//if(g + 2 < 0xff)
	//	g += 2;
	b = (Color >> 16) & 0xff;
	//if(g + 2 < 0xff)
	//	g += 2;
	return (r << 16)|(g << 8)| b;//& ((R_MASK << 16) | (G_MASK << 8) | (B_MASK) );
}

/*********************************************************************
*
*       LCD_GetIndexMask_666
*/
unsigned LCD_GetIndexMask_666(void) {
	printf("LCD_GetIndexMask_666 \r\n");
  return ((R_MASK << 16)| (G_MASK << 8) | (B_MASK));
}

/*************************** End of file ****************************/
