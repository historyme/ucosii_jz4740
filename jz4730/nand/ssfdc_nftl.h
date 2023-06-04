#ifndef __SSFDC_NFTL_H__
#define __SSFDC_NFTL_H__

//#include "jz_nand.h"
#include "nandconfig.h"

#define DIRECT_MAP    0     //1: direct map enable; 0: direct map disable     Suggestion: better set to 0, or when bad block comes out, the system will be crashed
#define SSFDC_SHIFT   2     //the number will effect the max partition number allowed, eg. set to 2, the max partition number is 4; set to 3, the max partition nubmer is 8

/*
 * Common hardware model for SmartMedia Flash Cards
 */
#define NAND_BLK_STATUS_EARLY_FAIL        0x00
#define NAND_BLK_STATUS_LATE_FAIL         0xF0
#define NAND_BLK_STATUS_NORMAL            0xFF

#define DISPLAY_SIZE        512
#define FAT_HARDSECT        512
#define HARDSECT            FAT_HARDSECT
#define SECTORS_PER_PAGE    (CONFIG_SSFDC_NAND_PAGE_SIZE / HARDSECT) 
#define NAND_OOB_SIZE                     (CONFIG_SSFDC_NAND_PAGE_SIZE / 32)

#define SSFDC_BIT_VALID_ENTRY          0x8000
#define SSFDC_BIT_BLOCK_ADDR           0x7FFF

#define SSFDC_BIT_FREE_BLOCK           0x01
#define SSFDC_BIT_BAD_BLOCK            0x02
#define SSFDC_BIT_CIS_BLOCK            0x04

#define Left(i)        ((i) << 1)        
#define Right(i)    (((i) << 1) + 1)

struct ssfdc_nftl_ops_t{
	int (*init)(void);
	int (*open)(int minor);
	int (*release)(int minor);
	int (*write)(int nr_page, int tmp, int zone, unsigned char *data_ptr);
	int (*read)(int nr_page, int tmp, int zone, unsigned char *data_ptr);
};

struct nand_page_info_t
{
	unsigned char   block_status;
	unsigned int    reserved;
	unsigned short  block_addr_field;
	unsigned int    lifetime;
	unsigned char   ecc_field[NAND_OOB_SIZE - 11];	
} __attribute__ ((packed));


struct page_cache_t {
	unsigned char data[SECTORS_PER_PAGE][HARDSECT];
	unsigned long page_num;
};//added by celion

struct ssfdc_zone_t {
	unsigned int free_phys_block;
	unsigned int free_virt_block;
	unsigned int used_block;
	unsigned int bad_block;
	unsigned short *block_lookup; //index: virt_block value:block_info's index->phy_block
	struct block_info *block_info;
	unsigned short *block_info_index; 	
};  

struct block_info{
	unsigned int lifetime;
	unsigned char tag;
};

struct block_cache_t
{
	int in_use;
	int zone;
	int virt_block; 
	int old_phys_block;
	int new_phys_block;
	unsigned short dirty_page[CONFIG_SSFDC_NAND_PAGE_PER_BLOCK];
	unsigned short dirty_sector[CONFIG_SSFDC_NAND_PAGE_PER_BLOCK][SECTORS_PER_PAGE];
	unsigned char data[CONFIG_SSFDC_NAND_PAGE_PER_BLOCK][CONFIG_SSFDC_NAND_PAGE_SIZE];
};

struct nand_chip_info_t
{
    unsigned char make;         // maker code
    unsigned char device;       // device code 
};

struct ssfdc_client_t
{
	int ref_count[(1<<SSFDC_SHIFT)+1];
	void *dev_id;
//	spinlock_t lock;
	int lock;
	int nr_zone;

  	struct nand_chip_info_t chip_info;	
	struct nand_io_t *io;
	struct ssfdc_zone_t *zone;
	struct nand_phy_zone_t *hw_zone;
	struct block_cache_t write_cache;
	struct page_cache_t read_page_cache;
};


struct nand_phy_zone_t
{
	unsigned short total_phys_block;
	unsigned short total_virt_block;
	unsigned short bad_block_num;
	unsigned short start_phys_block;
};

struct nand_io_t
{
    	char name[0x100];
    	int (*open) (void *dev_id);
    	int (*release) (void *dev_id);
    	int (*scan_id) (void *dev_id, struct nand_chip_info_t *info);
    	int (*read_page) (void *dev_id, int page, unsigned char *data, struct nand_page_info_t *info);
	int (*read_page_info) (void *dev_id, int page, struct nand_page_info_t *info);
	int (*write_page_info) (void *dev_id, int page, struct nand_page_info_t *info);
	int (*write_page) (void *dev_id, int page, unsigned char *data, struct nand_page_info_t *info);
	int (*erase_block) (void *dev_id, int block);
};

int nand_io_register (void *dev_id, struct nand_io_t *nand_io);
int nand_io_unregister (void *dev_id);
void ssfdc_dump_page (unsigned char *data);
void ssfdc_dump_page_info (struct nand_page_info_t *info);
int  ssfdc_flush_cache(void);



/* ssfdc nftl layer : alloc and free resources */
int ssfdc_nftl_init (void);
int ssfdc_nftl_open (int minor);
int ssfdc_nftl_release (int minor);
int ssfdc_client_has_inited (struct ssfdc_client_t *client);
struct nand_phy_zone_t * ssfdc_fill_zone_table(void);
int ssfdc_alloc_and_init_data_struct (struct ssfdc_client_t *client);
void ssfdc_free_and_destroy_data_struct (struct ssfdc_client_t *client);

/* ssfdc nftl layer : address mapping & bad-block managment */
int ssfdc_find_free_block (struct ssfdc_client_t *client, int zone, int *free_phys_block);
int ssfdc_block_info_map_bad_block (struct ssfdc_client_t *client, int zone, int phys_block);
int ssfdc_block_lookup_map_entry (struct ssfdc_client_t *client, int zone, int virt_block, int phys_block);
int ssfdc_block_lookup_unmap_entry (struct ssfdc_client_t *client, int zone, int virt_block);
int ssfdc_address_translate (struct ssfdc_client_t *client, int zone, int virt_block, int *phys_block);

/* ssfdc nftl layer :  wear-leving */
void ssfdc_heapsort(struct ssfdc_zone_t *zone, int heapsize);
void ssfdc_maxheapify(struct ssfdc_zone_t *zone, int i, int heapsize);
void ssfdc_swap(unsigned short *x, unsigned short *y);

/* ssfdc nftl layer :  read-page & ECC */
int ssfdc_read_page(int phys_page, unsigned char *data, struct nand_page_info_t *info);
extern int ssfdc_rs_ecc_init(void);
extern int ssfdc_verify_ecc (unsigned char *data, struct nand_page_info_t *info, unsigned short correct_enable);

/* ssfdc nftl layer :  read and write called by ssfdc block layer */
int ssfdc_nftl_read(int nr_page, int tmp, int zone, unsigned char *data_ptr);
int ssfdc_nftl_write(int nr_page, int tmp, int zone, unsigned char *data_ptr);

/* ssfdc nftl layer :  block-cache operation */
void inline ssfdc_init_cache (struct ssfdc_client_t *client, struct block_cache_t *cache);
void ssfdc_setup_cache (struct ssfdc_client_t *client, struct block_cache_t *cache, int zone, int virt_block, int new_phys_block, int old_phys_block);
void ssfdc_fill_block_cache(void);
int ssfdc_erase_block(int phys_block);
int ssfdc_program_block(int phys_block);
int ssfdc_mark_bad_block_to_nand(int phys_block);

/* ssfdc nftl layer :  Proc filesystem support */
//int ssfdc_proc_read (char *buf, char **start, off_t offset, int count, int *eof, void *unused);
int ssfdc_proc_read (void);
#endif /* __SSFDC_NTFL_H__ */
