/*
**********************************************************************
*
*                            uC/MMC
*
*             (c) Copyright 2005 - 2007, Ingenic Semiconductor, Inc
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : mmc_core.h 
Purpose     : MMC core definitions.

----------------------------------------------------------------------
Version-Date-----Author-Explanation
----------------------------------------------------------------------
1.00.00 20060831 WeiJianli     First release

----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
(none)
---------------------------END-OF-HEADER------------------------------
*/

#ifndef __MMC_CORE__
#define __MMC_CORE__

#define ID_TO_RCA(x) ((x)+1)

/* the information structure of MMC/SD Card */
typedef struct MMC_INFO
{
	int             id;     /* Card index */
        int             sd;     /* MMC or SD card */
        int             rca;    /* RCA */
        u32             scr;    /* SCR 63:32*/        
	int             flags;  /* Ejected, inserted */
	enum card_state state;  /* empty, ident, ready, whatever */

	/* Card specific information */
	struct mmc_cid  cid;
	struct mmc_csd  csd;
	u32             block_num;
	u32             block_len;
	u32             erase_unit;
} mmc_info;

extern mmc_info mmcinfo;

struct mmc_request {
	int               index;      /* Slot index - used for CS lines */
	int               cmd;        /* Command to send */
	u32               arg;        /* Argument to send */
	enum mmc_rsp_t    rtype;      /* Response type expected */

	/* Data transfer (these may be modified at the low level) */
	u16               nob;        /* Number of blocks to transfer*/
	u16               block_len;  /* Block length */
	u8               *buffer;     /* Data buffer */
	u32               cnt;        /* Data length, for PIO */

	/* Results */
	u8                response[18]; /* Buffer to store response - CRC is optional */
	enum mmc_result_t result;
};

char * mmc_result_to_string( int );
int    mmc_unpack_csd( struct mmc_request *request, struct mmc_csd *csd );
int    mmc_unpack_r1( struct mmc_request *request, struct mmc_response_r1 *r1, enum card_state state );
int    mmc_unpack_r6( struct mmc_request *request, struct mmc_response_r1 *r1, enum card_state state, int *rca);
int    mmc_unpack_scr( struct mmc_request *request, struct mmc_response_r1 *r1, enum card_state state, u32 *scr);
int    mmc_unpack_cid( struct mmc_request *request, struct mmc_cid *cid );
int    mmc_unpack_r3( struct mmc_request *request, struct mmc_response_r3 *r3 );

void   mmc_send_cmd( struct mmc_request *request, int cmd, u32 arg, 
		     u16 nob, u16 block_len, enum mmc_rsp_t rtype, u8 *buffer);
u32    mmc_tran_speed( u8 ts );
void   jz_mmc_set_clock(int sd, u32 rate);
void   jz_mmc_hardware_init(void);

static inline void mmc_simple_cmd( struct mmc_request *request, int cmd, u32 arg, enum mmc_rsp_t rtype )
{
	mmc_send_cmd( request, cmd, arg, 0, 0, rtype, 0);
}

#endif  /* __MMC_CORE__ */
