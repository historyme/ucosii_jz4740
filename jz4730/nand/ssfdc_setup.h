#ifndef __SSFDC_SETUP_H_

#define YES 1
#define NO 0

int get_nand_page_size(void);
int get_nand_page_per_block(void);
int get_nand_block_size(void);
unsigned short get_sectors_per_page(void);

//unsigned short get_hw_ecc_enable(void);
unsigned short get_sw_ecc_enable(void);
unsigned short get_wear_enable(void);
unsigned short get_write_verify_enable(void);

int get_start_block(void);
int get_total_blocks(void);
int get_allow_bad_blocks(void);

int get_ssfdc_major(void);
void test(void);

#endif /*__SSFDC_SETUP_H_ */
