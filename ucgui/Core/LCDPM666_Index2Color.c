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
File        : LCDPM666_Index2Color.c
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
#define B_BITS 8
#define G_BITS 8
#define R_BITS 8


#define R_BITCOUNT 6
#define G_BITCOUNT 6
#define B_BITCOUNT 6

#define R_MASK (1 << (R_BITCOUNT -1))
#define G_MASK (1 << (G_BITCOUNT -1))
#define B_MASK (1 << (B_BITCOUNT -1))

/*********************************************************************
*
*       Public code,
*
*       LCD_FIXEDPALETTE == 666, 256K colors, RRRRRGGGGGGBBBBB
*
**********************************************************************
*/
/*********************************************************************
*
*       LCD_Index2Color_M666
*/
LCD_COLOR LCD_Index2Color_M666(int Index) {

	unsigned int r,g,b;
	register unsigned char d;
  
    /* Seperate the color masks */
	b = Index & 0xff;
	g = (Index >> 8)  & 0xff;
    r = (Index >> 16) & 0xff;
	printf("LCD_Index2Color_M666\r\n");
	
	return (r) + (g<<8) + (b << 16);
}
/*************************** End of file ****************************/
