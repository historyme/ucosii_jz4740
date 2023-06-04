#ifndef __JZ_NAND_H__
#define __JZ_NAND_H__
//#include "ssfdc_nftl.h"
//#include "nandconfig.h"
#include "ssfdc_nftl.h"

//extern struct semaphore nand_flash_sem;
#define NANDFLASH_SPIN_LOCK()
#define NANDFLASH_SPIN_UNLOCK()
#if CONFIG_SSFDC_NAND_PAGE_SIZE==4096
#define NAND_PAGE_SHIFT                   12
#endif
#if CONFIG_SSFDC_NAND_PAGE_SIZE==2048
#define NAND_PAGE_SHIFT                   12
#endif
#if CONFIG_SSFDC_NAND_PAGE_SIZE==512
#define NAND_PAGE_SHIFT                   9
#endif

#define NAND_BLOCK_SIZE                   (CONFIG_SSFDC_NAND_PAGE_PER_BLOCK * CONFIG_SSFDC_NAND_PAGE_SIZE)
#define NAND_OOB_SIZE                     (CONFIG_SSFDC_NAND_PAGE_SIZE / 32)

#define NAND_CMD_READ1_00                 0x00
#define NAND_CMD_READ1_01                 0x01
#define NAND_CMD_READ2                    0x50
#define NAND_CMD_READ_ID1                 0x90
#define NAND_CMD_READ_ID2                 0x91
#define NAND_CMD_RESET                    0xFF
#define NAND_CMD_PAGE_PROGRAM_START       0x80
#define NAND_CMD_PAGE_PROGRAM_STOP        0x10
#define NAND_CMD_BLOCK_ERASE_START        0x60
#define NAND_CMD_BLOCK_ERASE_CONFIRM      0xD0
#define NAND_CMD_READ_STATUS              0x70

#define JZ_DATA_DELAY              1
#define JZ_CS_DELAY                2
#define JZ_CMD_TO_DATA_DELAY       20//32
#define JZ_WAIT_STAUTS_DELAY       32

#ifdef CONFIG_MIPS_JZ4730
#define NANDFLASH_CLE           0x00040000 //PA[18]
#define NANDFLASH_ALE           0x00080000 //PA[19]
#endif

#ifdef CONFIG_MIPS_JZ4740
#define NANDFLASH_CLE           0x00008000 //PA[15]
#define NANDFLASH_ALE           0x00010000 //PA[16]
#endif

#define NAND_IO_ADDR           *((volatile unsigned char *) NANDFLASH_BASE)

#define JZ_NAND_SET_CLE    (NANDFLASH_BASE |=  NANDFLASH_CLE)
#define JZ_NAND_CLR_CLE    (NANDFLASH_BASE &= ~NANDFLASH_CLE)
#define JZ_NAND_SET_ALE    (NANDFLASH_BASE |=  NANDFLASH_ALE)
#define JZ_NAND_CLR_ALE    (NANDFLASH_BASE &= ~NANDFLASH_ALE)

#ifdef CONFIG_MIPS_JZ4730
#define JZ_NAND_SELECT     (REG_EMC_NFCSR |= EMC_NFCSR_FCE)
#define JZ_NAND_DESELECT   (REG_EMC_NFCSR &= ~EMC_NFCSR_FCE)
#define __nand_ecc_enable()	(REG_EMC_NFCSR |= EMC_NFCSR_ECCE | EMC_NFCSR_ERST)
#define __nand_ecc_disable()	(REG_EMC_NFCSR &= ~EMC_NFCSR_ECCE)
#endif

#ifdef CONFIG_MIPS_JZ4740
#define JZ_NAND_SELECT     (REG_EMC_NFCSR |=  EMC_NFCSR_NFCE1)//EMC_NFCSR_NFE1 |
#define JZ_NAND_DESELECT   (REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))
#define __nand_ecc_enable()   (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST)
#define __nand_ecc_disable()  (REG_EMC_NFECR &= ~EMC_NFECR_ECCE)

#define __nand_ready()		((REG_GPIO_PXPIN(2) & 0x40000000) ? 1 : 0)
#define __nand_ecc_hamming()    (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_HAMMING)
#define __nand_ecc_rs_encoding()  (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_ENCODING)
#define __nand_ecc_rs_decoding()  (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_DECODING)
#define __nand_ecc_encode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_ENCF))
#define __nand_ecc_decode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_DECF))
#endif

#define __nand_ecc()		(REG_EMC_NFECC & 0x00fcffff)

#ifdef  CONFIG_SSFDC_HW_RS_ECC
#define JZSOC_ECC_BLOCK		512 /* 9-bytes RS ECC per 512-bytes data */
#define PAR_SIZE                9
#endif

#ifdef  CONFIG_SSFDC_HW_HM_ECC
#define JZSOC_ECC_BLOCK		256 /* 3-bytes HW ECC per 256-bytes data */
#endif

#define JZSOC_ECC_STEPS      (CONFIG_SSFDC_NAND_PAGE_SIZE / JZSOC_ECC_BLOCK)

int jz_nand_init(void);
void jz_nand_cleanup (void);

#ifdef CONFIG_SSFDC_HW_HM_ECC
int nand_correct_data_256(unsigned char *dat, unsigned char *read_ecc, unsigned char *calc_ecc);
#endif

#ifdef CONFIG_SSFDC_HW_RS_ECC
void rs_correct(unsigned char *buf, int idx, int mask);
#endif

int jz_nand_open (void *dev_id);
int jz_nand_release (void *dev_id);
int jz_nand_scan_id (void *dev_id, struct nand_chip_info_t *info);
int jz_nand_read_page_info (void *dev_id, int page, struct nand_page_info_t *info);
int jz_nand_read_page (void *dev_id, int page, unsigned char *data,struct nand_page_info_t *info);
int jz_nand_write_page (void *dev_id, int page, unsigned char *data, struct nand_page_info_t *info);
int jz_nand_write_page_info (void *dev_id, int page, struct nand_page_info_t *info);
int jz_nand_erase_block (void *dev_id, int block);


#endif /* __JZ_NAND_H__ */
