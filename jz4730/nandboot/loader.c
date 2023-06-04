/*
 * loader.c
 *
 * nandboot loader of the Linux kernel.
 *
 * Copyright (c) 2005-2007  Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "config.h"
#include "nand.h"
#include "jz4730.h"

/* Load image parameters */
#define LOAD_IMAGE_SIZE		0x200000   /* max data size */
#define LOAD_ADDR		0x80100000 /* load to address */
#define LOAD_START_BLOCK	1          /* load from nand block */

/* kernel parameters */
#define KERNEL_ENTRY		0x80100000
#define PARAM_BASE		0x80040000
#define TMPBUF_BASE		0x80050000

#define ECC_BLOCK		256 /* 3-bytes HW ECC per 256-bytes data */
#define ECC_POS			4   /* ECC offset to spare area */

static u32 *load_addr = 0, *param_addr = 0, *databuf32 = 0;
static u32 *stored_ecc = 0, *calc_ecc = 0;
static u8 *oobbuf = 0, *databuf = 0, *tmpbuf = 0;

const u8 cmdline[256] = CFG_CMDLINE;

typedef void (*linux_entry)(int, char **, char *);
static linux_entry entry;

static inline void nand_read_buf16(void *buf, int count)
{
	int i;
	u16 *p = (u16 *)buf;

	for (i = 0; i < count; i += 2)
		*p++ = __nand_data16();
}

static inline void nand_read_buf8(void *buf, int count)
{
	int i;
	u8 *p = (u8 *)buf;

	for (i = 0; i < count; i++)
		*p++ = __nand_data8();
}

static inline void nand_read_buf(void *buf, int count, int bw)
{
	if (bw == 8)
		nand_read_buf8(buf, count);
	else
		nand_read_buf16(buf, count);
}

/*
 * Main entry.
 */
void nandloader(void)
{
	register u32 i, j, k;
	register u32 cnt, num_blk, blkaddr, rowaddr, addr;
	register u32 boot_sel, tmp, ecc_bit, idx;
	register u32 buswidth, pagesize, rowaddrcycle, ecccount, oobsize, ppb;

	/*
	 * Init hardware
	 */
	gpio_init();
	serial_init();

	serial_puts("\n\nJZ4730 NANDBoot version 1.0\n\n");

	serial_puts("PLL init ... ");
	pll_init();
	serial_puts("done\n");

	serial_puts("SDRAM init ... ");
#if  (CFG_SDRAM_ROW == 12)
	sdram_init0();      /* init sdram as 64M */
#endif
	sdram_init();
	serial_puts("done\n");

	/*
	 * Decode the boot select
	 */
	boot_sel = (REG_EMC_NFCSR & 0x70) >> 4;
	buswidth = (boot_sel & 0x1) ? 16 : 8;           /* bus width */
	pagesize = (boot_sel & 0x2) ? 2048 : 512; /* page size */
	rowaddrcycle = (boot_sel & 0x4) ? 3 : 2;  /* row address cycles */

	ppb = (pagesize == 2048) ? 64 : 32;         /* pages per block */

	ecccount = pagesize / ECC_BLOCK;
	oobsize = pagesize / 32;

	/* Init buffers */
	load_addr = (u32 *)LOAD_ADDR;
	databuf32 = (u32 *)TMPBUF_BASE;
	databuf = (u8 *)databuf32;
	oobbuf = (u8 *)(TMPBUF_BASE + 0x1000);
	calc_ecc = (u32 *)(TMPBUF_BASE + 0x2000);
	entry = (linux_entry)KERNEL_ENTRY;

	/* Calc block number */
	num_blk = LOAD_IMAGE_SIZE / (pagesize * ppb);
	if ((num_blk * pagesize * ppb) < LOAD_IMAGE_SIZE)
		num_blk++;

	/*
	 * Start to load image
	 */
	serial_puts("NAND data load ... ");

	__nand_enable();

	cnt = 0;
	blkaddr = LOAD_START_BLOCK;

	while (cnt < num_blk) {
nand_load_block:
		/* read one block */
		rowaddr = blkaddr * ppb;
		for (i = 0; i < ppb; i++) {
			/* send READ0 command */
			__nand_cmd(NAND_CMD_READ0);

			/* col addr cycle */
			__nand_addr(0);
			if (pagesize == 2048)
				__nand_addr(0);

			/* row addr cycle */
			addr = rowaddr + i;
			__nand_addr(addr & 0xff);
			__nand_addr((addr >> 8) & 0xff);
			if (rowaddrcycle == 3)
				__nand_addr((addr >> 16) & 0xff);

			/* send READSTART command for 2048 ps NAND */
			if (pagesize == 2048)
				__nand_cmd(NAND_CMD_READSTART);

			/* wait ready */
			__nand_sync();

			/* read page and oob data */
			tmpbuf = databuf;
			for (j = 0; j < ecccount; j++) {
				__nand_ecc_enable();
				nand_read_buf((void *)tmpbuf, ECC_BLOCK, buswidth);
				__nand_ecc_disable();
				calc_ecc[j] = __nand_ecc();
				tmpbuf += ECC_BLOCK;
			}
			nand_read_buf((void *)oobbuf, oobsize, buswidth);

			/* check for bad block */
			if (i == 0) {
				if (oobbuf[0] != 0xff) {
					/* bad block, jump to next block */
					blkaddr ++;
					goto nand_load_block;
				}
			}

			/* check for ECC */
			stored_ecc = (u32 *)(((u32)oobbuf) + ECC_POS);
			tmpbuf = (u8 *)databuf;

			for (j = 0; j < ecccount; j++) {
				tmp = stored_ecc[j] ^ calc_ecc[j];
				if (tmp) { /* ECC error occurred */
					if (stored_ecc[j] == 0xffffffff) {
						/* block containing invalid data,
						 * finish loading.
						 */
						goto nand_load_finish;
					}
					ecc_bit = 0;
					for (k = 0; k < 24; k++)
						if ((tmp >> k) & 0x01)
							ecc_bit ++;
					if (ecc_bit != 11) {
						/* Fatal error */
						serial_puts("ECC error.\n");
						blkaddr ++;
						goto nand_load_block;
					} else {
						/* Correctable error */
						ecc_bit = 0;
						for (k = 12; k >= 1; k--) {
							ecc_bit <<= 1;
							ecc_bit |= ((tmp>>(k*2-1)) & 0x01);
						}
						idx = ecc_bit & 0x07;
						tmpbuf[j * ECC_BLOCK + (ecc_bit >> 3)] ^= (1 << idx);
					}
				}
			}

			/* Data is OK, transfer to target buffer */
			for (j = 0; j < ((pagesize == 2048) ? 512:128); j++)
				*load_addr++ = databuf32[j];
		}

		cnt ++;
		blkaddr ++;

	} /* end of while */

 nand_load_finish:

	serial_puts("done\n\n");

	/* prepare execute parameters and environment */
	param_addr = (u32 *)PARAM_BASE;
	param_addr[0] = 0;	/* might be address of ascii-z string: "memsize" */
	param_addr[1] = 0;	/* might be address of ascii-z string: "0x01000000" */
	param_addr[2] = 0;
	param_addr[3] = 0;
	param_addr[4] = 0;
	param_addr[5] = PARAM_BASE + 32;
	param_addr[6] = KERNEL_ENTRY;
	tmpbuf = (u8 *)(PARAM_BASE + 32);
	for (i = 0; i < 256; i++)
		tmpbuf[i] = cmdline[i];  /* linux command line */

	serial_puts("Starting kernel ...\n\n");

	/* flush dcache and icache */
	flush_cache_all();

	/* jump to kernel */
	entry(2, (char **)(PARAM_BASE+16), (char *)PARAM_BASE);
}
