/*
 * head.S
 *
 * Entry of the nandboot code.
 *
 * Copyright (c) 2005-2007  Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

	.set noreorder
	.global _start
_start:
	li	$26, 0x00800000		/* IV bit */
	mtc0	$26, $13		/* CP0_CAUSE */

	li	$26, 0x00000400
	mtc0	$26, $12		/* CP0_STATUS */

	la	$29, 0x80004000		/* stack pointer */
	j	nandloader
	nop
	.set reorder

