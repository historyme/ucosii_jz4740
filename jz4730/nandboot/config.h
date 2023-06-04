/*
 *
 * This file contains the configuration parameters for the board.
 *
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Include the board header file. */
#include "board-pmp.h"

extern void sdram_init(void);
extern int serial_init (void);
extern void serial_puts (const char *s);

#endif /* __CONFIG_H__ */
