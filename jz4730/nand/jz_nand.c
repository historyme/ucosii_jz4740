/********************************************
 *
 * File: drivers/ssfdc/jz_nand.c
 * 
 * Description: Actual hardware IO routies for the nand flash device on JZ 
 *
 * History: 27 May 2003 - new
 *          27 Mar 2007 - by Peter & Celion, Ingenic Ltd    
 ********************************************/

#include <regs.h>
#include <ops.h>
//#include "ssfdc_nftl.h"
#include "jz_nand.h"


#define __author__              "Peter, Celion"
#define __description__         "JZ nand I/O driver"
#define __driver_name__         "jz_nand"
#define __license__             "GPL"

//#define JZ_DEBUG
//#define JZ_HW_TEST

/* global mutex between yaffs2 and ssfdc */
//DECLARE_MUTEX(nand_flash_sem);



#ifdef CONFIG_MIPS_JZ4730
static void inline nand_wait_ready(void)
{
	unsigned int timeout = 100;
	while ((REG_EMC_NFCSR & 0x80) && timeout--);
	while (!(REG_EMC_NFCSR & 0x80));
}
static unsigned long NANDFLASH_BASE = 0xB4000000;
#endif

#ifdef CONFIG_MIPS_JZ4740
static inline void nand_wait_ready(void)
{
//#if 1
	unsigned int timeout = 100000;
	while ((REG_GPIO_PXPIN(2) & 0x40000000) && timeout--);
	while (!(REG_GPIO_PXPIN(2) & 0x40000000));
//#else
//	unsigned int timeout = 0x7ffffff;
//	while(timeout--);
//#endif
}
static unsigned long NANDFLASH_BASE = 0xB8000000;
#endif

struct jz_nand_t
{
	struct nand_io_t io;
};

static struct jz_nand_t jz_nand;

int jz_nand_open (void *dev_id)
{

//	MOD_INC_USE_COUNT;
	return 0;
}

int jz_nand_release (void *dev_id)
{
//	MOD_DEC_USE_COUNT;
    
	return 0;
}

int jz_nand_scan_id (void *dev_id, struct nand_chip_info_t *info)
{

	NANDFLASH_SPIN_LOCK();

	JZ_NAND_SELECT;

	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_READ_ID1;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = NAND_CMD_READ1_00;
	JZ_NAND_CLR_ALE;
        
	info->make = NAND_IO_ADDR;
	info->device = NAND_IO_ADDR;
	
	JZ_NAND_DESELECT;
	
	NANDFLASH_SPIN_UNLOCK();

	return 0;
}

int jz_nand_read_page_info (void *dev_id, int page, struct nand_page_info_t *info)
{
	unsigned long addr = page << NAND_PAGE_SHIFT;
	unsigned char *ptr;
	int i;
	
	NANDFLASH_SPIN_LOCK();

	JZ_NAND_SELECT;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x00;//NAND_CMD_READ2;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((CONFIG_SSFDC_NAND_PAGE_SIZE & 0x000000FF) >>  0);
	NAND_IO_ADDR = (unsigned char)((CONFIG_SSFDC_NAND_PAGE_SIZE & 0x00000F00) >>  8);
	NAND_IO_ADDR = (unsigned char)((addr & 0x000FF000) >> 12);
	NAND_IO_ADDR = (unsigned char)((addr & 0x0FF00000) >> 20);
	if( CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		NAND_IO_ADDR = (unsigned char)((addr & 0xF0000000) >> 28);
	JZ_NAND_CLR_ALE;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x30;
	JZ_NAND_CLR_CLE;
	
	nand_wait_ready();	    
	
	ptr = (unsigned char *) info;
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		*ptr++ = NAND_IO_ADDR;
	}
	JZ_NAND_DESELECT;
	
	NANDFLASH_SPIN_UNLOCK();
	
	return 0;
}

int jz_nand_read_page (void *dev_id, int page, unsigned char *data,struct nand_page_info_t *info)
{
	int i,j;
	unsigned long addr = page << NAND_PAGE_SHIFT;
	unsigned char *ptr;
	//unsigned char oob_buf[NAND_OOB_SIZE];
	
	//unsigned char *tmp;
#ifdef CONFIG_SSFDC_HW_RS_ECC
	//struct nand_page_info_t oob_buf;
	//oob_buf = malloc(sizeof(struct nand_page_info_t));
	//jz_nand_read_page_info(dev_id,page, &oob_buf);
	jz_nand_read_page_info(dev_id,page, info);

/*	unsigned char oob_buf[NAND_OOB_SIZE];
	NANDFLASH_SPIN_LOCK();
	JZ_NAND_SELECT;
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x00;//NAND_CMD_READ2;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((CONFIG_SSFDC_NAND_PAGE_SIZE & 0x000000FF) >>  0);
	NAND_IO_ADDR = (unsigned char)((CONFIG_SSFDC_NAND_PAGE_SIZE & 0x00000F00) >>  8);
	NAND_IO_ADDR = (unsigned char)((addr & 0x000FF000) >> 12);
	NAND_IO_ADDR = (unsigned char)((addr & 0x0FF00000) >> 20);
	if( CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		NAND_IO_ADDR = (unsigned char)((addr & 0xF0000000) >> 28);
	JZ_NAND_CLR_ALE;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x30;
	JZ_NAND_CLR_CLE;
	
	nand_wait_ready();	    
	
	ptr = (unsigned char *) oob_buf;
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		*ptr++ = NAND_IO_ADDR;
	}
	JZ_NAND_DESELECT;
	
	NANDFLASH_SPIN_UNLOCK();*/
		
#endif
	NANDFLASH_SPIN_LOCK();
	JZ_NAND_SELECT;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x00;//NAND_CMD_READ1_00;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((addr & 0x000000FF) >> 0);
	NAND_IO_ADDR = (unsigned char)((addr & 0x00000F00) >> 8);
	NAND_IO_ADDR = (unsigned char)((addr & 0x000FF000) >> 12);
	NAND_IO_ADDR = (unsigned char)((addr & 0x0FF00000) >> 20);
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		NAND_IO_ADDR = (unsigned char)((addr & 0xF0000000) >> 28);
	JZ_NAND_CLR_ALE;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x30;
	JZ_NAND_CLR_CLE;
	
	nand_wait_ready();
	
	ptr = (unsigned char *) data;

//#ifdef CONFIG_MIPS_JZ4740
#ifdef CONFIG_SSFDC_HW_RS_ECC
	unsigned int stat;
	//unsigned char *
	unsigned char *par = (unsigned char *)&info->ecc_field;
//unsigned char *par = (unsigned char *)oob_buf;
	//printk("The page_info for creating the error index and mask.\n");
	//ssfdc_dump_page_info(info);

	for(j = 0; j < JZSOC_ECC_STEPS; j++){

		volatile unsigned char *paraddr = (volatile unsigned char *)EMC_NFPAR0;
		REG_EMC_NFINTS = 0x0;
		__nand_ecc_rs_decoding();
		for(i = 0; i < JZSOC_ECC_BLOCK; i++)
			*ptr++ = NAND_IO_ADDR;
		printf("The values of par_register!\n[");
		for(i=0;i<PAR_SIZE;i++){
			printf(" %02x ",*par);
			*paraddr++ = *par++;
			printf(" %02x ",*(paraddr-1));
		}
		printf("]\n");
		REG_EMC_NFECR |= EMC_NFECR_PRDY;
		__nand_ecc_decode_sync();
		__nand_ecc_disable();
		stat = REG_EMC_NFINTS;
		printf("stat = 0x%x\n", stat);
		if (stat & EMC_NFINTS_ERR) {
			printf("Error occurred\n");
			if (stat & EMC_NFINTS_UNCOR) {
				printf("Uncorrectable error occurred\n");
			}
			else {
				u32 errcnt = (stat & EMC_NFINTS_ERRCNT_MASK) >> EMC_NFINTS_ERRCNT_BIT;
				printf("errcnt=%d\n", errcnt);
				printf("ERR0=0x%08x INDEX=%d MASK=0x%x\n", REG_EMC_NFERR0, (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				printf("ERR1=0x%08x INDEX=%d MASK=0x%x\n", REG_EMC_NFERR1, (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				printf("ERR2=0x%08x INDEX=%d MASK=0x%x\n", REG_EMC_NFERR2, (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				printf("ERR3=0x%08x INDEX=%d MASK=0x%x\n", REG_EMC_NFERR3, (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);

				switch (errcnt) {
				case 4:
					rs_correct(data, (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 3:
					rs_correct(data, (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 2:
					rs_correct(data, (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 1:
					rs_correct(data, (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
					break;
				default:
					break;
				}
			}
		}
		
		/* increment pointer */
		data += JZSOC_ECC_BLOCK;
	}
	printf("The info and data after verifying!\n");
	ssfdc_dump_page_info(info);
	ssfdc_dump_page(data);

//#endif
//#endif	
#else //of rs
#ifdef CONFIG_SSFDC_HW_HM_ECC
//	int j;
	unsigned int calc_ecc[JZSOC_ECC_STEPS];

	for(j = 0; j < JZSOC_ECC_STEPS; j++){
		__nand_ecc_enable();
		for(i = 0; i < JZSOC_ECC_BLOCK; i++)
			*ptr++ = NAND_IO_ADDR;
		__nand_ecc_disable();
		calc_ecc[j] = __nand_ecc();
	}
#else //else of hm
	for (i = 0; i < CONFIG_SSFDC_NAND_PAGE_SIZE; i++) {
		*ptr++ = NAND_IO_ADDR;
	}
#endif //end of define hm
	
	ptr = (unsigned char *) info;
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		*ptr++ = NAND_IO_ADDR;
	}
	
#ifdef CONFIG_SSFDC_HW_HM_ECC
	//tmp = (unsigned char *)data;
	ptr = (unsigned char *)&info->ecc_field;
	for(i = 0; i < JZSOC_ECC_STEPS; i++){
		if( nand_correct_data_256(data, ptr, (unsigned char *)&calc_ecc[i]) ) {
			JZ_NAND_DESELECT;
			NANDFLASH_SPIN_UNLOCK();
			return -1;
		}
		ptr += 3;
		data += JZSOC_ECC_BLOCK;
	}
#endif //end of hm
#endif //end of rs
	JZ_NAND_DESELECT;

	NANDFLASH_SPIN_UNLOCK();

	return 0;
}

#ifdef CONFIG_SSFDC_HW_RS_ECC
void rs_correct(unsigned char *buf, int idx, int mask)
{
	int i, j;
	unsigned short d, d1, dm;

	i = (idx * 9) >> 3;
	j = (idx * 9) & 0x7;

	i = (j == 0) ? (i - 1) : i;
	j = (j == 0) ? 7 : (j - 1);

	d = (buf[i] << 8) | buf[i - 1];

	d1 = (d >> j) & 0x1ff;
	d1 ^= mask;

	dm = ~(0x1ff << j);
	d = (d & dm) | (d1 << j);

	buf[i - 1] = d & 0xff;
	buf[i] = (d >> 8) & 0xff;
}
#endif
#ifdef CONFIG_SSFDC_HW_HM_ECC

static unsigned char  CountNumberOfOnes(unsigned int  num)
{
	unsigned char  count = 0;

	while(num)
	{
		num=num&(num-1);
		count++;
	}

	return count;
}

static unsigned int  ECC_IsDataValid(unsigned char *  newECC,unsigned char *  oldECC)
{	    
	int i;
	for(i=0; i<3; i++)
	{
		if(newECC[i] !=oldECC[i] )
		{
			return 0;
		}
	}

	return 1;	
}

int  nand_correct_data_256(unsigned char *  pData, unsigned char *  poldECC, unsigned char *  pnewECC)
{
	unsigned int  oldECC;
	unsigned int  newECC; 
	//unsigned char numOnes = 0;
	unsigned int  byteLocation = 0;
	unsigned int  bitLocation  = 0;
	unsigned char *  p;
	unsigned char  i;
	oldECC=0;
	p=(unsigned char * )&oldECC;
	
	if(ECC_IsDataValid(poldECC,pnewECC))
		return 0;
	if((*poldECC == 0xff)&&(*(poldECC+1) == 0xff)&&(*(poldECC+2) == 0xff))
		return 0;

	for(i = 0;i < 3;i++)
		*(p+i)=*(poldECC+i);
	newECC=0;
	p=(unsigned char * )&newECC;
	for(i = 0;i < 3;i++)
		*(p+i)=*(pnewECC+i);
	newECC=(oldECC^newECC);
	newECC&=0x0fcffff;

	if(CountNumberOfOnes(newECC)!=11)
	{
		printf("Ecc uncorrectable error!\r\n");
		return -1;
	}
	byteLocation=newECC&0x0ffff;
	byteLocation=((byteLocation&0x000002)>>1 ) |
		((byteLocation&0x000008)>>2 ) |
		((byteLocation&0x000020)>>3 ) |
		((byteLocation&0x000080)>>4 ) |
		((byteLocation&0x000200)>>5 ) |
		((byteLocation&0x000800)>>6 ) |
		((byteLocation&0x002000)>>7 ) |
		((byteLocation&0x008000)>>8 ) ;
	
	bitLocation=(newECC>>18);
	bitLocation=((bitLocation&0x02)>>1)|
                ((bitLocation&0x08)>>2)|
                ((bitLocation&0x20)>>3);               

	if(pData[byteLocation]&(1<<bitLocation))
           	pData[byteLocation]&=(~(1<<bitLocation));
	else
		pData[byteLocation] |= ( 1<<bitLocation ); 	        
  
	return 0;
}
#endif	

int jz_nand_write_page (void *dev_id, int page, unsigned char *data, struct nand_page_info_t *info)
{
	int i,j;
	unsigned long addr = page << NAND_PAGE_SHIFT;
	unsigned char *ptr, status;

	NANDFLASH_SPIN_LOCK();
	
	JZ_NAND_SELECT;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_PAGE_PROGRAM_START;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((addr & 0x000000FF) >> 0);
	NAND_IO_ADDR = (unsigned char)((addr & 0x00000F00) >> 8);
	NAND_IO_ADDR = (unsigned char)((addr & 0x000FF000) >> 12);
	NAND_IO_ADDR = (unsigned char)((addr & 0x0FF00000) >> 20);    
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		NAND_IO_ADDR = (unsigned char)((addr & 0xF0000000) >> 28);
	JZ_NAND_CLR_ALE;
	
	nand_wait_ready();

	ptr = (unsigned char *) data;
//#ifdef CONFIG_MIPS_JZ4740
#ifdef CONFIG_SSFDC_HW_RS_ECC
	//unsigned char ecc_buf[NAND_OOB_SIZE], *par = ecc_buf;

	unsigned char *par = (unsigned char *)&info->ecc_field;
	for(j = 0; j < JZSOC_ECC_STEPS; j++){
		volatile unsigned char *paraddr = (volatile unsigned char *)EMC_NFPAR0;
		__nand_ecc_rs_encoding();
		for(i = 0; i < JZSOC_ECC_BLOCK; i++)
			NAND_IO_ADDR = *ptr++;
		__nand_ecc_encode_sync();
		__nand_ecc_disable();
		for (i = 0; i < PAR_SIZE; i++)
			*par++ = *paraddr++;
	}

	//printk("The info and datas for writing to nand!\n");//par = ecc_buf;
	//ssfdc_dump_page_info(data);
	printf("The info and data before verifying\n");
	ssfdc_dump_page_info(info);
	ssfdc_dump_page(data);
	//ptr = (unsigned char *)&info->ecc_field;
	//for(i = 0; i < JZSOC_ECC_STEPS*PAR_SIZE; i++)
	//*ptr++ = *par++;
#else //else of rs
//#endif
//#endif
#ifdef CONFIG_SSFDC_HW_HM_ECC
//	int j;
	unsigned int calc_ecc[JZSOC_ECC_STEPS];
	unsigned char *store_ecc=NULL;

	for(j = 0; j < JZSOC_ECC_STEPS; j++) {
		__nand_ecc_enable(); //// ????????????????
		for(i =0; i < JZSOC_ECC_BLOCK; i++)
			NAND_IO_ADDR = *ptr++;
		__nand_ecc_disable();
		calc_ecc[j] = __nand_ecc();
	}
	ptr = (unsigned char *)&info->ecc_field;
	for(i = 0; i < JZSOC_ECC_STEPS; i++){
		store_ecc =(unsigned char *)&calc_ecc[i];
		for(j = 0; j < 3; j++)
			*ptr++ = store_ecc[j];
	}
#else //else of hm
	for (i = 0; i < CONFIG_SSFDC_NAND_PAGE_SIZE; i++) {
		NAND_IO_ADDR = *ptr++;
	}
#endif //end of hm
#endif //end of rs	
	ptr = (unsigned char *) info;
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		NAND_IO_ADDR = *ptr++;
	}
	
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_PAGE_PROGRAM_STOP;
	JZ_NAND_CLR_CLE;
	
	nand_wait_ready();
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_READ_STATUS;
	JZ_NAND_CLR_CLE;
	
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		status = NAND_IO_ADDR;
		// check I/O pin 6
		if (status & 0x40) {
			JZ_NAND_DESELECT;
			// check I/O pin 0
			if (status & 0x01)
				goto write_error;
			else {
				NANDFLASH_SPIN_UNLOCK();
				return 0;
			}
			break;
		}
		udelay(JZ_WAIT_STAUTS_DELAY);
	}
	JZ_NAND_DESELECT;

 write_error:
	NANDFLASH_SPIN_UNLOCK();
	printf("write page: 0x%x failed\n", page);
	return -EIO;
}


int jz_nand_write_page_info (void *dev_id, int page, struct nand_page_info_t *info)
{
	int i;
	unsigned long addr = page << NAND_PAGE_SHIFT;
	unsigned char *ptr, status;
	
	NANDFLASH_SPIN_LOCK();

	JZ_NAND_SELECT;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_PAGE_PROGRAM_START;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((CONFIG_SSFDC_NAND_PAGE_SIZE & 0x000000FF) >>  0);
	NAND_IO_ADDR = (unsigned char)((CONFIG_SSFDC_NAND_PAGE_SIZE & 0x00000F00) >>  8);
	NAND_IO_ADDR = (unsigned char)((addr & 0x000FF000) >> 12);
	NAND_IO_ADDR = (unsigned char)((addr & 0x0FF00000) >> 20);    
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		NAND_IO_ADDR = (unsigned char)((addr & 0xF0000000) >> 28);
	JZ_NAND_CLR_ALE;
    
	nand_wait_ready();
	
	ptr = (unsigned char *) info;
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		NAND_IO_ADDR = *ptr++;
	}

	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_PAGE_PROGRAM_STOP;
	JZ_NAND_CLR_CLE;
	
	nand_wait_ready();
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_READ_STATUS;
	JZ_NAND_CLR_CLE;
	
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		status = NAND_IO_ADDR;
		// check I/O pin 6
		if (status & 0x40) {
			JZ_NAND_DESELECT;
			// check I/O pin 0
			if (status & 0x01)
				goto write_error;
			else {
				NANDFLASH_SPIN_UNLOCK();
				return 0;
			}
			break;
		}
		udelay(JZ_WAIT_STAUTS_DELAY);
	}
	JZ_NAND_DESELECT;
	
 write_error:
	NANDFLASH_SPIN_UNLOCK();

	printf("write page : 0x%x 's info failed\n", page);
	return -EIO;
}


int jz_nand_erase_block (void *dev_id, int block)
{
	int i;
	unsigned char status;
	unsigned long addr = block * CONFIG_SSFDC_NAND_PAGE_PER_BLOCK;// << NAND_BLOCK_SHIFT;

	NANDFLASH_SPIN_LOCK();
	JZ_NAND_SELECT;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_BLOCK_ERASE_START;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((addr & 0xFF));
	NAND_IO_ADDR = (unsigned char)((addr >> 8) & 0xFF);
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		NAND_IO_ADDR = (unsigned char)((addr >> 16) & 0xFF);
	JZ_NAND_CLR_ALE;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_BLOCK_ERASE_CONFIRM;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_READ_STATUS;
	JZ_NAND_CLR_CLE;
	
	// wait a maximum of 3ms
	for (i = 0; i < CONFIG_SSFDC_NAND_PAGE_PER_BLOCK; i++) {
		status = NAND_IO_ADDR;
		// check I/O pin 6
		if (status & 0x40) {
			JZ_NAND_DESELECT;
			// check I/O pin 0
			if (status & 0x01)
				goto erase_error;
			else {
				NANDFLASH_SPIN_UNLOCK();
				return 0;
			}
		}
		udelay(JZ_WAIT_STAUTS_DELAY);
	}
	
	JZ_NAND_DESELECT;

 erase_error:
	NANDFLASH_SPIN_UNLOCK();
	printf("erase block: 0x%x failed\n", block);
	return -EIO;
}

#ifdef JZ_HW_TEST
static void jz_nand_hw_test (void)
{
    unsigned char data[CONFIG_SSFDC_NAND_PAGE_SIZE];
    struct nand_page_info_t page_info;
    int i;
    printf("make bad block begin......\n");
    memset(&page_info, 0x0, sizeof(page_info));
    page_info.block_status = 0xff;
    for(i=512; i<512+5; i++){
    	jz_nand_erase_block(NULL, i);
        jz_nand_write_page_info(NULL, i*64, &page_info);
        jz_nand_read_page_info(NULL, i*64, &page_info);
   	ssfdc_dump_page_info(&page_info);
    }
    
}
#endif

static void jz_nand_hw_init (void)
{
	/* Enable NAND flash controller */
#ifdef CONFIG_MIPS_JZ4730
	REG_EMC_NFCSR |= EMC_NFCSR_NFE;
	REG_EMC_SMCR3 = 0x04444400;
#endif

#ifdef CONFIG_MIPS_JZ4740
	REG_EMC_NFCSR |= EMC_NFCSR_NFE1;// | EMC_NFCSR_NFCE1;
	REG_EMC_SMCR1 = 0x04444400;
#endif
}

int jz_nand_init (void)
{
	int ret;
//	init_MUTEX(&nand_flash_sem);
	
	jz_nand_hw_init();
#ifdef JZ_HW_TEST
	jz_nand_hw_test();
#endif
	strcpy(jz_nand.io.name, "JZ nand I/O\n");
	jz_nand.io.open = jz_nand_open;
	jz_nand.io.release = jz_nand_release;
	jz_nand.io.scan_id = jz_nand_scan_id;
	jz_nand.io.read_page_info = jz_nand_read_page_info;
	jz_nand.io.read_page = jz_nand_read_page;
	jz_nand.io.write_page_info = jz_nand_write_page_info;
	jz_nand.io.write_page = jz_nand_write_page;
	jz_nand.io.erase_block = jz_nand_erase_block;
	ret = nand_io_register(&jz_nand, &jz_nand.io);

	if ((ret = nand_io_register(&jz_nand, &jz_nand.io)) < 0)
		return ret;
	printf("%s installed\n", __description__);
	
	return 0;
}

void jz_nand_cleanup (void)
{
	nand_io_unregister(&jz_nand);
	printf("%s removed\n", __description__);
}

