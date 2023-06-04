/*
 *  File: drivers/ssfdc/ssfdc_setup.c
 * 
 *  Description: set things that depend on your special condition
 *  
 *  Author: Nancy, Ingenic Ltd
 *
 *  History: 27 Mar, 2007  -new
 */

/*----------------------------------------------------------------------
 * Define the ssfdc physical block info.
 *

	 NAND Flash Blocks 	block address
	 (e.g. total 4096)
	+---------------+	4095
	|		|
	|		|
	|		|
	|   SSFDCBLOCK	|
	|     (2048)	|
	|   		|
	|		|	2048
	+---------------+	2047
	|		|	
	|		|
	|		|
	|   MTDBLOCK 	|
	|    (2048)	|
	|		|
	|		|
	+---------------+	0

*/


#include "jz_nand.h"
#include "ssfdc_nftl.h"
#include "ssfdc_setup.h"

/* start physical block, it have to be even. (START_BLOCK % 2 ==0) */
static int start_block = CONFIG_SSFDC_START_BLOCK;

/* total block number of the ssfdc block */
static int total_blocks = CONFIG_SSFDC_TOTAL_BLOCKS;

/* Allowable max bad block number */
static int allow_bad_blocks = CONFIG_SSFDC_MAX_BAD_BLOCK_ALLOWD;

/*----------------------------------------------------------------------*/

/* ssfdc_major: ssfdc major number, can't set to 0 */
static int ssfdc_major = 254;

static unsigned short sectors_per_page = SECTORS_PER_PAGE;

/* Please do not change the interface below */
static int nand_page_size = CONFIG_SSFDC_NAND_PAGE_SIZE;
static int nand_page_per_block = CONFIG_SSFDC_NAND_PAGE_PER_BLOCK;
static int nand_block_size = NAND_BLOCK_SIZE;


unsigned short get_sectors_per_page(void)
{
	return sectors_per_page;
}

int get_nand_page_size(void)
{
	return nand_page_size;
}

int get_nand_page_per_block(void)
{
	return nand_page_per_block;
}

int get_nand_block_size(void)
{
	return nand_block_size;
}

int get_ssfdc_major(void)
{
	return ssfdc_major;
}


unsigned short get_sw_ecc_enable(void)
{
#ifdef CONFIG_SSFDC_SW_HM_ECC 
	return YES;
#endif

#ifdef CONFIG_SSFDC_SW_RS_ECC 
	return YES;
#endif

	return NO;
}

unsigned short get_wear_enable(void)
{
	return CONFIG_SSFDC_WEAR_ENABLE;
}

unsigned short get_write_verify_enable(void)
{
	return CONFIG_SSFDC_WRITE_VERIFY;
}

int get_start_block(void)
{
	return start_block;
}

int get_total_blocks(void)
{
	return total_blocks;
}

int get_allow_bad_blocks(void)
{
	return allow_bad_blocks;
}
  
void test(void)
{
	printf("all the value below from setup.c\n");
	printf("sectors_per_page: %d\n",get_sectors_per_page());
	printf("nand_page_size: %d\n",get_nand_page_size());
	printf("nand_page_per_block: %d\n",get_nand_page_per_block());
	printf("nand_block_size: %d\n",get_nand_block_size());
	printf("ssfdc_major: %d\n",get_ssfdc_major());
	printf("sw_ecc_enable: %d\n",get_sw_ecc_enable());
	printf("wear_enable: %d\n",get_wear_enable());
	printf("write_verify_enable: %d\n",get_write_verify_enable());
	printf("start_block: %d\n",get_start_block());
	printf("total_block: %d\n",get_total_blocks());
	printf("allow_bad_blocks: %d\n",get_allow_bad_blocks());
	printf("\n\n");	
}

