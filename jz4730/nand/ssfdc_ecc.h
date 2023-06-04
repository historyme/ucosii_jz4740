#ifndef __SSFDC_ECC_H
#include <regs.h>
#include <ops.h>
#include "ssfdc_nftl.h"
//#include "jz4740.h"
#define ECC_ERROR 10

int ssfdc_rs_ecc_init(void);
void ssfdc_free_rs_ecc_init(void);

int ssfdc_verify_ecc (unsigned char *data, struct nand_page_info_t *info, unsigned short correct_enable);
#endif
