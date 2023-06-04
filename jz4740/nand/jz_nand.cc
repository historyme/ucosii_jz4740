/********************************************
 *
 * File: drivers/ssfdc/jz_nand.c
 * 
 * Description: Actual hardware IO routies for the nand flash device on JZ 
 *
 * History: 27 May 2003 - new
 *          27 Mar 2007 - by Peter & Celion, Ingenic Ltd    
 ********************************************/
#include <bsp.h>
#include <jz4740.h>
#include "jz_nand.h"
#include "ssfdc_nftl.h"
#define __author__              "Peter, Celion"
#define __description__         "JZ nand I/O driver"
#define __driver_name__         "jz_nand"
#define __license__             "GPL"

//#define JZ_DEBUG
//#define JZ_HW_TEST

/* global mutex between yaffs2 and ssfdc */
#define DMA_ENABLE 1

#if DMA_ENABLE
#define DMA_CHANNEL 5
#define PHYSADDR(x) ((x) & 0x1fffffff)
#endif


void rs_correct(unsigned char *buf, int idx, int mask);

#define NAND_INTERRUPT 1
#if NAND_INTERRUPT
#include "ucos_ii.h"

static OS_EVENT * rb_sem;

#define RB_GPIO_GROUP 2
#define RB_GPIO_BIT   30
#define RB_GPIO_PIN (32 * RB_GPIO_GROUP + RB_GPIO_BIT)  
#define NAND_CLEAR_RB() (REG_GPIO_PXFLGC(RB_GPIO_GROUP) = (1 << RB_GPIO_BIT))
#define nand_wait_ready()                     \
do{                                           \
    unsigned short timeout = 1000;             \
	while((!(REG_GPIO_PXFLG((RB_GPIO_GROUP) & (1 << RB_GPIO_BIT))))& timeout--);\
    REG_GPIO_PXFLGC(RB_GPIO_GROUP) = (1 << RB_GPIO_BIT); \
}while(0)
#define READDATA_TIMEOUT 100
#define WRITEDATA_TIMEOUT 800

#else
static inline void nand_wait_ready(void)
{
	unsigned int timeout = 100;
	while ((REG_GPIO_PXPIN(2) & 0x40000000) && timeout--);
	while (!(REG_GPIO_PXPIN(2) & 0x40000000));
}
#endif
static unsigned long NANDFLASH_BASE = 0xB8000000;
#define REG_NAND_DATA  *((volatile unsigned char *) NANDFLASH_BASE)
#define REG_NAND_CMD  *((volatile unsigned char *) (NANDFLASH_BASE + NANDFLASH_CLE))
#define REG_NAND_ADDR *((volatile unsigned char *) (NANDFLASH_BASE + NANDFLASH_ALE))


static inline int nand_send_readaddr(unsigned int pageaddr,unsigned int offset)
{
    unsigned char err;
//	printf("pageaddr = 0x%x %d\r\n",pageaddr,offset);
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x00;//NAND_CMD_READ1_00;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((offset & 0x000000FF) >> 0);
	NAND_IO_ADDR = (unsigned char)((offset & 0x0000FF00) >> 8);
	
	NAND_IO_ADDR = (unsigned char)((pageaddr & 0x000000FF) >> 0);
	NAND_IO_ADDR = (unsigned char)((pageaddr & 0x0000FF00) >> 8);
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		NAND_IO_ADDR = (unsigned char)((pageaddr & 0x00FF0000) >> 16);
	JZ_NAND_CLR_ALE;
#if NAND_INTERRUPT	
	NAND_CLEAR_RB();
	__gpio_unmask_irq(RB_GPIO_PIN);
#endif
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x30;
	JZ_NAND_CLR_CLE;

#if NAND_INTERRUPT	
	OSSemPend(rb_sem,READDATA_TIMEOUT,&err);
	if(err)
	{
		printf("time\r\n");
		
		return -1;
		
		//	goto write_error;
		
	}
#else
	nand_wait_ready();
#endif
	
	return 0;
	
//	nand_wait_ready();
	
}

static void inline nand_send_readcacheaddr(unsigned short offset)
{
  	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0x05;//NAND_CMD_READ1_00;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((offset & 0x000000FF) >> 0);
	NAND_IO_ADDR = (unsigned char)((offset & 0x0000FF00) >> 8);
	JZ_NAND_CLR_ALE;

	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = 0xe0;
	JZ_NAND_CLR_CLE;
	
}


#if DMA_ENABLE
static inline void nand_write_memcpy(void *target,void *source,unsigned int len)
{
    int ch = DMA_CHANNEL;
	//printf("target = 0x%08x source = 0x%08x\r\n",target,source);
	
	if((unsigned int)source < 0xa0000000)
		 dma_cache_wback_inv((unsigned long)source, len);
	
	if((unsigned int)target < 0xa0000000)
		dma_cache_wback_inv((unsigned long)target, len);
	

	REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)source);       
	REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target);       
	REG_DMAC_DTCR(ch) = len / 16;	            	    
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;	        	
	REG_DMAC_DCMD(ch) = DMAC_DCMD_SAI| DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_8|DMAC_DCMD_DS_16BYTE;
	
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
	while (	REG_DMAC_DTCR(ch) );
	
	
}
static inline void nand_read_memcpy(void *target,void *source,unsigned int len)
{
    int ch = DMA_CHANNEL;
//	printf("target = 0x%08x source = 0x%08x\r\n",target,source);
	
	if((unsigned int)source < 0xa0000000)
	{
       
		dma_cache_wback_inv((unsigned long)source, len);
		
		
	}
	
	if((unsigned int)target < 0xa0000000)
	{
		dma_cache_wback_inv((unsigned long)target, len);
		
    }
	
	REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)source);       
	REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target);       
	REG_DMAC_DTCR(ch) = len / 4;	            	    
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;	        	
	REG_DMAC_DCMD(ch) = DMAC_DCMD_SAI| DMAC_DCMD_DAI | DMAC_DCMD_SWDH_8 | DMAC_DCMD_DWDH_32|DMAC_DCMD_DS_32BIT;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
	while (	REG_DMAC_DTCR(ch) );
	
	
}
#endif
static inline int nand_read_oob(int page,struct nand_page_info_t *info)
{
   	unsigned char *ptr;
    unsigned short i;
	if(nand_send_readaddr(page,CONFIG_SSFDC_NAND_PAGE_SIZE))
    	    return -1;
	ptr = (unsigned char *)info;	
  	for ( i = 0; i < NAND_OOB_SIZE; i++) 
    	*ptr++ = NAND_IO_ADDR;
    return 0;
	
}
static inline int nand_rs_correct(unsigned char *tmpbuf)
{
	unsigned int stat = REG_EMC_NFINTS;
	//printf("stat = 0x%x\n", stat);
	if (stat & EMC_NFINTS_ERR) {
		if (stat & EMC_NFINTS_UNCOR) {
			printf("Uncorrectable error occurred\n");
			printf("stat = 0x%x\n", stat);
			return -EIO;
		}
		else {
			printf("Correctable error occurred\n");
			printf("stat = 0x%x\n", stat);
			unsigned int errcnt = (stat & EMC_NFINTS_ERRCNT_MASK) >> EMC_NFINTS_ERRCNT_BIT;
			switch (errcnt) {
			case 4:
				rs_correct(tmpbuf, (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				break;
				
			case 3:
				rs_correct(tmpbuf, (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				break;
				
			case 2:
				rs_correct(tmpbuf, (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				break;
				
			case 1:
				rs_correct(tmpbuf, (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				break;
			default:
				
				break;
			}
		}
		
	}
	return 0;	
}


int jz_nand_scan_id (void *dev_id, struct nand_chip_info_t *info)
{
#ifdef JZ_DEBUG
	printf("%s()\n", __FUNCTION__);
#endif

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
	

	return 0;
}

int jz_nand_read_page_info (void *dev_id, int page, struct nand_page_info_t *info)
{
	int ret
#ifdef JZ_DEBUG    
	printf("%s() page: 0x%x\n", __FUNCTION__, page);
#endif

	JZ_NAND_SELECT;
	ret = nand_read_oob(page,info)	
	JZ_NAND_DESELECT;
	return ret;
}
int jz_nand_read__pagewithmask(void *dev_id, int page,int mask,unsigned char *data,struct nand_page_info_t *info)
{
	int i,ret = 0;
	unsigned long addr;
	unsigned char *ptr,*tmpbuf;
	
#ifdef JZ_DEBUG
	printf("%s() page: 0x%x\n", __FUNCTION__, page);
#endif

	JZ_NAND_SELECT;
    if(nand_read_oob(page,info))
    {
		JZ_NAND_DESELECT;
		return -EIO;    
    }
	addr = 0;
	ptr = (unsigned char *) data;
	tmpbuf = (unsigned char *)data;
    unsigned char *par = (unsigned char *)&info->ecc_field;
			
    for(i = 0; (i < JZSOC_ECC_STEPS) && (ret == 0);i++)
	{
		if(mask & (1 << i))
		{
			REG_EMC_NFINTS = 0x0;
			__nand_ecc_rs_decoding();

			nand_send_readcacheaddr(addr);
#if DMA_ENABLE
			nand_read_memcpy(ptr,(void *)&NAND_IO_ADDR,JZSOC_ECC_BLOCK);
			ptr += JZSOC_ECC_BLOCK;
		
#else
		
			for(i = 0; i < JZSOC_ECC_BLOCK; i++)
				*ptr++ = NAND_IO_ADDR;
#endif
			volatile unsigned char *paraddr = (volatile unsigned char*)EMC_NFPAR0;
            
			for(i = 0; i < PAR_SIZE; i++)
				*paraddr++ = *(par+i);
			REG_EMC_NFECR |= EMC_NFECR_PRDY;
			__nand_ecc_decode_sync();
			__nand_ecc_disable();
			if((info->block_addr_field == 0xffff)&&(info->lifetime == 0xffffffff))
				continue;
            ret = nand_rs_correct(tmpbuf);
			
		}
		par += PAR_SIZE;
		addr += JZSOC_ECC_BLOCK;
        tmpbuf += JZSOC_ECC_BLOCK;
	}
		
	JZ_NAND_DESELECT;
	return ret;	
}

int jz_nand_read_page (void *dev_id, int page, unsigned char *data,struct nand_page_info_t *info)
{
	int i,j,ret = 0;
	
	unsigned char *ptr;
    volatile unsigned char *paraddr = (volatile unsigned char*)EMC_NFPAR0;
	unsigned char *par;
	unsigned char *tmpbuf;
	unsigned int stat;
	
#ifdef JZ_DEBUG
	printf("%s() page: 0x%x\n", __FUNCTION__, page);
#endif
//    printf("%s() page: 0x%x\n", __FUNCTION__, page);

	JZ_NAND_SELECT;
   
    if(nand_read_oob(page,info))
    {
		JZ_NAND_DESELECT;
		return -EIO;    
    }
	nand_send_readcacheaddr(0);
	ptr = (unsigned char *) data;
	tmpbuf = (unsigned char *)data;

	

	par = (unsigned char *)&info->ecc_field;
	
	for(j = 0; (j < JZSOC_ECC_STEPS) &&(ret == 0); j++){
		    
		REG_EMC_NFINTS = 0x0;
		__nand_ecc_rs_decoding();
#if DMA_ENABLE
		nand_read_memcpy(ptr,(void *)&NAND_IO_ADDR,JZSOC_ECC_BLOCK);
		ptr += JZSOC_ECC_BLOCK;
		
#else		
		for(i = 0; i < JZSOC_ECC_BLOCK; i++)
		{
			unsigned int dat;
			dat = NAND_IO_ADDR;
			*ptr++ = dat;
			
			//if((i % 16) == 0)
			//printf("\r\n");
			//printf("0x%02x ",dat);		
		}
		
		
#endif
		//printf("\r\nread par:\r\n");
		 volatile unsigned char *paraddr = (volatile unsigned char*)EMC_NFPAR0;
		for(i = 0; i < PAR_SIZE; i++)
		{
			//printf("0x%02x ",*par);
			*paraddr++ = *par++;
		}
		//	printf("\r\n");
		REG_EMC_NFECR |= EMC_NFECR_PRDY;
		
		__nand_ecc_decode_sync();
       	__nand_ecc_disable();
		
		
		if((info->block_addr_field == 0xffff)&&(info->lifetime == 0xffffffff))
			continue;
		ret =  nand_rs_correct(tmpbuf);
		tmpbuf += JZSOC_ECC_BLOCK;
		
	}
	JZ_NAND_DESELECT;
	return ret;
}
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
    if(i > 512 / 2)
	{
		buf[i - 1] = d & 0xff;
		buf[i] = (d >> 8) & 0xff;
	}
	
}
static void inline nand_send_writecacheaddr(unsigned short offset)
{
	
}
static void nand_send_writeaddr(unsigned int pageaddr,unsigned int offset)
{
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_PAGE_PROGRAM_START;
	JZ_NAND_CLR_CLE;
	
	JZ_NAND_SET_ALE;
	NAND_IO_ADDR = (unsigned char)((offset >> 0) & 0x0FF);
	NAND_IO_ADDR = (unsigned char)((offset >> 8) & 0x0FF);
	NAND_IO_ADDR = (unsigned char)((pageaddr >> 0) & 0x0FF);
	NAND_IO_ADDR = (unsigned char)((pageaddr >> 8) & 0x0FF);    
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		NAND_IO_ADDR = (unsigned char)((pageaddr >> 16) & 0x0FF);
	JZ_NAND_CLR_ALE;	
}
int jz_nand_write_page (void *dev_id, int page, unsigned char *data, struct nand_page_info_t *info)
{
	int i;
	unsigned char *ptr, status;
	
#ifdef JZ_DEBUG
	printf("!!!!!!!gt!!! %s() page: 0x%x 0x%x 0x%x\n", __FUNCTION__, page, *data, *(data+1));
#endif
//	printf("!!!!!!!gt!!! %s() page: 0x%x 0x%08x 0x%x\n", __FUNCTION__, page, data, *(data+1));
	JZ_NAND_SELECT;
	nand_send_writeaddr(page,0);
	ptr = (unsigned char *) data;

	int j;
	unsigned char *par = (unsigned char *)&(info->ecc_field);
//	printf("Write Nand:\r\n");
	
	for(j = 0;j<JZSOC_ECC_STEPS; j++){
		REG_EMC_NFINTS = 0;
		__nand_ecc_rs_encoding();
#if DMA_ENABLE
        nand_write_memcpy((void *)&NAND_IO_ADDR,ptr,JZSOC_ECC_BLOCK);
		ptr += JZSOC_ECC_BLOCK;	
#else
		for(i =0; i < JZSOC_ECC_BLOCK; i++)	
			NAND_IO_ADDR = *ptr++;
#endif   
		__nand_ecc_encode_sync();
        
		//nand_wait_ready();
		//ndelay(100);
		
		//	printf("par:\n[");
        volatile unsigned char *paraddr = (unsigned char *)EMC_NFPAR0; 
		for (i = 0; i < PAR_SIZE; i++){
			//		printf(" 0x%x", *paraddr);
			*par++ = *paraddr++;
		}
		//	printf("]\n");
		__nand_ecc_disable();
	}
//	printf("The info write to nand\n");
//	ssfdc_dump_page_info(info);
//	ssfdc_dump_page(data);
	
	ptr = (unsigned char *) info;
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		//	if((i % 16) == 0)
		//	printf("\r\n");
		//printf("0x%02x ",*ptr);		
		NAND_IO_ADDR = *ptr++;
        

	}
#if NAND_INTERRUPT	
	NAND_CLEAR_RB();
	__gpio_unmask_irq(RB_GPIO_PIN);
#endif
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_PAGE_PROGRAM_STOP;
	JZ_NAND_CLR_CLE;
	
	//nand_wait_ready();
#if NAND_INTERRUPT
	unsigned char err;
	OSSemPend(rb_sem,100,&err);
	if(err)
	{
		printf("!!!!!!!error!!! %s() page: 0x%x \n", __FUNCTION__, page);
		JZ_NAND_DESELECT;
		goto write_error;
		
	}
#else
	nand_wait_ready();
#endif
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_READ_STATUS;
	JZ_NAND_CLR_CLE;

    i = NAND_OOB_SIZE * JZ_WAIT_STAUTS_DELAY * 1000;
	while((NAND_IO_ADDR & 0x01) &&(i--)) ;
	
	JZ_NAND_DESELECT;
	if(i > 0)
		return 0;
 write_error:
	printf("write page: 0x%x failed\n", page);
	return -EIO;
}


int jz_nand_write_page_info (void *dev_id, int page, struct nand_page_info_t *info)
{
	int i;
	unsigned long addr = page << NAND_PAGE_SHIFT;
	unsigned char *ptr, status;
	

	JZ_NAND_SELECT;

	nand_send_writeaddr(page,CONFIG_SSFDC_NAND_PAGE_SIZE);	
	ptr = (unsigned char *) info;
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		NAND_IO_ADDR = *ptr++;
	}

	//NAND_CLEAR_RB();
#if NAND_INTERRUPT	
	NAND_CLEAR_RB();
	__gpio_unmask_irq(RB_GPIO_PIN);
#endif
    
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_PAGE_PROGRAM_STOP;
	JZ_NAND_CLR_CLE;
	
#if NAND_INTERRUPT
	unsigned char err;
	OSSemPend(rb_sem,100,&err);
	if(err)
	{
		printf("!!!!!!!error!!! %s() page: 0x%x \n", __FUNCTION__, page);
		JZ_NAND_DESELECT;
		goto write_error;
	}
#else
	nand_wait_ready();
#endif
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_READ_STATUS;
	JZ_NAND_CLR_CLE;

	i = NAND_OOB_SIZE * JZ_WAIT_STAUTS_DELAY * 1000;
	while((NAND_IO_ADDR & 0x01) &&(i--)) ;
	
	JZ_NAND_DESELECT;
	if(i > 0)
		return 0;
 write_error:

	printf("write page : 0x%x 's info failed\n", page);
	return -EIO;
}

#define JZ_NAND_ERASE_TIME (30 * 1000)

int jz_nand_erase_block (void *dev_id, int block)
{
	int i;
	unsigned char status;
	unsigned long addr = block * CONFIG_SSFDC_NAND_PAGE_PER_BLOCK;
#ifdef JZ_DEBUG
	printf("$$$$$$$$$$$ %s() block: 0x%x addr=0x%x\n", __FUNCTION__, block, addr);
#endif
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
#if NAND_INTERRUPT	
	NAND_CLEAR_RB();
	__gpio_mask_irq(RB_GPIO_PIN);
#endif
	
	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_BLOCK_ERASE_CONFIRM;
	JZ_NAND_CLR_CLE;

	nand_wait_ready();



	JZ_NAND_SET_CLE;
	NAND_IO_ADDR = NAND_CMD_READ_STATUS;
	JZ_NAND_CLR_CLE;

	i = JZ_NAND_ERASE_TIME / 2 ;
	while((NAND_IO_ADDR & 0x01) && (i--)) ;
	JZ_NAND_DESELECT;
	if(i > 0)
		return 0;
	
 erase_error:
	printf("erase block: 0x%x failed\n", block);
	return -EIO;
}
#if NAND_INTERRUPT
static void jz_rb_handler(unsigned int arg)
{
    __gpio_mask_irq(arg);
	OSSemPost(rb_sem);	
}
#endif
void jz_nand_hw_init (void)
{
    unsigned int dat;
	
	REG_EMC_NFCSR |= EMC_NFCSR_NFE1;// | EMC_NFCSR_NFCE1;
    dat = REG_EMC_SMCR1;
	dat &= ~0x0fffff00;
	REG_EMC_SMCR1 = dat | 0x0a221200;
	printf("REG_EMC_SMCR1 = 0x%08x\r\n",REG_EMC_SMCR1);
#if DMA_ENABLE
	__cpm_start_dmac();
	__dmac_enable_module();
#endif
#if NAND_INTERRUPT	
	//__gpio_mask_irq(RB_GPIO_PIN);
	__gpio_disable_pull(RB_GPIO_PIN);
	
    NAND_CLEAR_RB();
	__gpio_as_irq_rise_edge(RB_GPIO_PIN);
	rb_sem = OSSemCreate(0);
	request_irq(IRQ_GPIO_0 + RB_GPIO_PIN, jz_rb_handler,RB_GPIO_PIN);
#endif
}

int jz_nand_init (void)
{
	int ret;	
	jz_nand_hw_init();
	return 0;
}

