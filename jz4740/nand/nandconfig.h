#ifndef __NANDCONFIG_H__
#define __NANDCONFIG_H__

#define malloc(size) alloc(size)
#define free(addr)  deAlloc(addr)
//#define CONFIG_MIPS_JZ4740 1
#define CONFIG_SSFDC_HW_RS_ECC 
//#define CONFIG_SSFDC_HW_HM_ECC 1
//#define CONFIG_SSFDC_SW_RS_ECC 0
//#define CONFIG_SSFDC_SW_HM_ECC 1
//#define JZ_HW_TEST //1

#define CONFIG_SSFDC_NAND_PAGE_SIZE 2048
#define CONFIG_SSFDC_NAND_PAGE_PER_BLOCK 128
#define CONFIG_SSFDC_NAND_ROW_CYCLE 3
#define CONFIG_SSFDC_WRITE_VERIFY 0
#define CONFIG_SSFDC_WEAR_ENABLE 1

#define off_t long

#define EIO 1
#define NULL -1
#define ENOMEM 2
#define ENODEV 3
#define EBUSY 4
#define EINVAL 1
#define CONFIG_SSFDC_START_BLOCK 512
#define CONFIG_SSFDC_TOTAL_BLOCKS 512
#define CONFIG_SSFDC_MAX_BAD_BLOCK_ALLOWD 10

#endif /* __GPIO_H__ */
