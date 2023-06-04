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
* File        : GUITouch.Conf.h
* Purpose     : Configures emWin GSC touch screen module
*********************************************************************************************************
*/


#ifndef GUITOUCH_CONF_H
#define GUITOUCH_CONF_H

#if JZ4740_PAV
#define GUI_TOUCH_AD_LEFT   0xc0   
#define GUI_TOUCH_AD_RIGHT  0xf49  
#define GUI_TOUCH_AD_TOP    0x19b 
#define GUI_TOUCH_AD_BOTTOM 0xf52

#else

#define GUI_TOUCH_AD_LEFT   0x077   
#define GUI_TOUCH_AD_RIGHT  0xf82  
#define GUI_TOUCH_AD_TOP    0x115 
#define GUI_TOUCH_AD_BOTTOM 0xf69

#endif
#define GUI_TOUCH_SWAP_XY    0
#define GUI_TOUCH_MIRROR_X   0
#define GUI_TOUCH_MIRROR_Y   0

#endif /* GUITOUCH_CONF_H */
