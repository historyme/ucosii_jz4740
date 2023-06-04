#ifndef __SSFDC_HM_ECC_H__
#define __SSFDC_HM_ECC_H__

/*
 * Calculate 3 byte ECC code for 256 byte block
 */
void ssfdc_hm_ecc_calculate (const unsigned char *dat, unsigned char *ecc_code);

/*
 * Detect and correct a 1 bit error for 256 byte block
 * returns  0   : no error
 *          1   : correct single bit error
 *          2   : ECC Code Error Correction
 *          -1  : uncorrectable error
 */
int ssfdc_hm_ecc_correct_data (unsigned char *dat, unsigned char *read_ecc, unsigned char *calc_ecc);

#endif /* __SSFDC_HM_ECC_H__*/
