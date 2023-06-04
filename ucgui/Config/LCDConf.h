/*
*********************************************************************************************************
*                                                µC/GUI
*                        Universal graphic software for embedded applications
*
*                       (c) Copyright 2002, Micrium Inc., Weston, FL
*                       (c) Copyright 2000, SEGGER Microcontroller Systeme GmbH          
*
*              µC/GUI is protected by international copyright laws. Knowledge of the
*              source code may not be used to write a similar product. This file may
*              only be used in accordance with a license and should not be redistributed 
*              in any way. We appreciate your understanding and fairness.
*
* File        : LCDConf_1375_C8_C320x240.h
* Purpose     : Sample configuration file

*********************************************************************************************************
*/


#ifndef LCDCONF_H
#define LCDCONF_H

/*********************************************************************
*
*                   General configuration of LCD
*
**********************************************************************
*/

//#define LCD_XSIZE      (320)   /* X-resolution of LCD, Logical coor. */
//#define LCD_YSIZE      (240)   /* Y-resolution of LCD, Logical coor. */
//#define LCD_VXSIZE      (320)   /* X-resolution of LCD, Logical coor. */
//#define LCD_VYSIZE      (240)

#define LCD_XSIZE      (480)   /* X-resolution of LCD, Logical coor. */
#define LCD_YSIZE      (272)   /* Y-resolution of LCD, Logical coor. */

#define LCD_VXSIZE     (480)   /* X-resolution of LCD, Logical coor. */
#define LCD_VYSIZE     (272)

#if (LCDTYPE == 0)
#error no add LCD driver
#endif
#ifndef JZ4740_PAV
#define LCD_BITSPERPIXEL (16)
#else
#define LCD_BITSPERPIXEL (18)
#endif
#define LCD_CONTROLLER 1375


/*********************************************************************
*
*                   Full bus configuration
*
**********************************************************************
*/

#define NR_PALETTE	256
#ifndef JZ4740_PAV
  #define LCD_READ_MEM(Off)            *((U16*)         (lcd_get_frame() + (((U32)(Off)) << 1)))
  #define LCD_WRITE_MEM(Off,data)      *((U16*)         (lcd_get_frame() +(((U32)(Off)) << 1))) = data
#else
  #define LCD_READ_MEM(Off)            *((U32*)(lcd_get_frame() + (((U32)(Off)) << 2)))
  #define LCD_WRITE_MEM(Off,data)      *((U32*)(lcd_get_frame() +(((U32)(Off)) << 2))) = data										 
#endif

#define LCD_INIT_CONTROLLER()       jzlcd_init()                                                             

  

#endif /* LCDCONF_H */

 
