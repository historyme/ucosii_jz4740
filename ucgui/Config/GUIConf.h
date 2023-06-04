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
* File    : GUICONF.H
* Purpose : Configures µC/GUI abilities, fonts etc.	
*
* Version-Date---Author-Explanation
*
* 0.00.00 YYMMDD RS     Initial release
*********************************************************************************************************
*/


#ifndef GUICONF_H
#define GUICONF_H

/*********************************************************************
*
*         Configuration of desired functionality
*                                      
**********************************************************************

*/

#define GUI_OS                    (1)  /* Compile with multitasking support */
#define GUI_WINSUPPORT            (1)  /* Use window manager if true (1)  */
#define GUI_SUPPORT_MEMDEV        (1)  /* Support memory devices */
#define GUI_SUPPORT_TOUCH         (1)  /* Support a touch screen (req. win-manager) */
#define GUI_SUPPORT_UNICODE       (1)
#define GUI_SUPPORT_AA            (1)
/*********************************************************************
*
*         Configuration of dynamic memory
*                                      
**********************************************************************
Dynamic memory is used for memory devices and window manager.
If you do not use these features, there is no need for dynamic memory
and it may be switched off completely. (This section can be erased)
*/

#define GUI_ALLOC_SIZE          1024 * 1024 * 2  /* Size of dynamic memory */


/*
        ****************************************
        *                                      *
        * Configuration of available fonts     *
        *                                      *
        ****************************************
*/


#define GUI_DEFAULT_FONT    &GUI_Font6x8
//#define GUI_DEFAULT_FONT    &GUI_Font8x16
#endif /* GUICONF_H */
