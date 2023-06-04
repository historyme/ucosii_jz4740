/********************************************
 *
 * File: drivers/ssfdc/ssfdc_nftl.c
 * 
 * Description: Generic SSFDC block device driver.
 *              - block address translation
 *              - bad block management 
 *              - write cache
 *
 * Author: Martin Siu, EmSoft Ltd
 * History: 26 May 2003 - new
 *          27 Mar 2007 - changed by Peter & Celion, Ingenic Ltd
 ********************************************/
#include "nandconfig.h"
#include "ssfdc_nftl.h"
#include "ssfdc_ecc.h"
#include "ssfdc_setup.h"

#define __author__              "Martin Siu, Peter, Celion"
#define __description__         "SSFDC core support"
#define __driver_name__         "ssfdc_nftl"
#define __license__             "GPL"

struct ssfdc_client_t ssfdc_client;
static unsigned short sw_ecc_enable, wear_enable, write_verify_enable;
static int nand_page_size, nand_page_per_block, nand_block_size;
static unsigned short sectors_per_page;

struct ssfdc_nftl_ops_t ssfdc_nftl_ops = {
	init:     ssfdc_nftl_init,
	open:     ssfdc_nftl_open,
	release:  ssfdc_nftl_release,
	read:     ssfdc_nftl_read,
	write:    ssfdc_nftl_write,
};

int ssfdc_nftl_open (int minor)
{
	struct ssfdc_client_t *client = &ssfdc_client;
	
//	if (!client->dev_id) {
//		printf("client not registerd\n");
//		client->ref_count[minor]++;
//		return -1;
//	}
        
//	if ( client->io->open(client->dev_id) < 0) {
//		printf("client not open\n");
//		client->ref_count[minor]++;
//		return -2;
//	}

	if (ssfdc_client_has_inited(client)) {
		printf("client already init, return normally\n");
		client->ref_count[minor]++;
		return 0;
	}
	client->ref_count[minor]++;
	if ( ssfdc_alloc_and_init_data_struct(client) ) {
		ssfdc_flush_cache();
		client->ref_count[minor]--;
		client->io->release(client->dev_id);
		return -ENOMEM;
	}
	return 0;
}

int ssfdc_nftl_release (int minor)
{
	struct ssfdc_client_t *client = &ssfdc_client;

	ssfdc_flush_cache();

	client->ref_count[minor]--;

	//free ssfdc_client's other items
	if( !ssfdc_client_has_inited(client) ){
		client->io->release(client->dev_id);
		ssfdc_free_and_destroy_data_struct(client);
	}
	return 0;
}

/* return 0 : ssfdc_client do not inited */
int ssfdc_client_has_inited (struct ssfdc_client_t *client)
{	
	int i;

	if (client->nr_zone > 0) {
		for (i = 0; i <= client->nr_zone; i++) {
			if (client->ref_count[i] > 0)
				return 1;
		}
		return 0;
	} else 
		return (client->ref_count[0] > 0);
    
}

void ssfdc_dump_page_info (struct nand_page_info_t *info)
{
	unsigned char *ptr = (unsigned char *) info;
	int i;
	
	printf("[ ");
	for (i = 0; i < sizeof(struct nand_page_info_t); i++)
		printf("%02x ", *ptr++);
	printf("]\n");
	
	printf("reserved: 0x%08x\n", info->reserved);
	printf("virt_block: %d\n", info->block_addr_field);
	printf("lifetime: %d\n", info->lifetime);
}

void ssfdc_dump_page (unsigned char *data)
{
	unsigned char *ptr = data;
	int col, row, value;
	
	printf("        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
	printf("------------------------------------------------------\n");    
	
//	for (row = 0; row < ( DISPLAY_SIZE / 16); row++) {
	for (row = 0; row < ( 2048 / 16); row++) {
		if (row < 0x10)
			printf("0x0%x0: ", row);
		else
			printf("0x%x0: ", row);
		
		for (col = 0; col < 16; col++) {
			value = 0x000000FF & ptr[row * 16 + col];
			if (value < 0x10)
				printf("0%x ", value);
			else
				printf("%x ", value);
		}
		printf("\n");
	}    
}

int ssfdc_block_info_map_bad_block (struct ssfdc_client_t *client, int zone, int phys_block)
{
	struct ssfdc_zone_t *zone_ptr;
	printf("%s\n",__FUNCTION__);
	
	zone_ptr = client->zone + zone;
	zone_ptr->block_info[phys_block].tag |= SSFDC_BIT_BAD_BLOCK;
	zone_ptr->bad_block++;
	zone_ptr->free_phys_block--;
	zone_ptr->free_virt_block--;
	
	if (zone_ptr->bad_block > client->hw_zone[zone].bad_block_num ) {
		printf("Warning: too many bad blocks: %d, nand flash is un-useable\n", zone_ptr->bad_block);
		return -EIO;
	}
	else {
		printf("bad_block=%d\n", zone_ptr->bad_block);
	}
	return 0;
}

int ssfdc_block_lookup_map_entry (struct ssfdc_client_t *client, int zone, int virt_block, int phys_block)
{
	struct ssfdc_zone_t *zone_ptr = client->zone + zone;
	
	if (!(zone_ptr->block_lookup[virt_block] & SSFDC_BIT_VALID_ENTRY)) {
	
		zone_ptr->block_lookup[virt_block] = phys_block;
		zone_ptr->block_lookup[virt_block] |= SSFDC_BIT_VALID_ENTRY;
		zone_ptr->block_info[phys_block].tag &= ~SSFDC_BIT_FREE_BLOCK;
		zone_ptr->free_phys_block--;
		zone_ptr->free_virt_block--;
		zone_ptr->used_block++;
		return 0;
	} else {
//		printf("Error: zone: 0x%x, map block address 0x%x -> (new) 0x%x, (old) 0x%x\n",zone, virt_block, phys_block, zone_ptr->block_lookup[virt_block] & SSFDC_BIT_BLOCK_ADDR);
		return -EINVAL;
	}
}

int ssfdc_block_lookup_unmap_entry (struct ssfdc_client_t *client, int zone, int virt_block)
{
	struct ssfdc_zone_t *zone_ptr = client->zone + zone;
	int phys_block;
        
	if (zone_ptr->block_lookup[virt_block] & SSFDC_BIT_VALID_ENTRY) {
		printf("unmap 0x%x -> 0x%x\n", virt_block, zone_ptr->block_lookup[virt_block] & SSFDC_BIT_BLOCK_ADDR);
		zone_ptr->block_lookup[virt_block] &= ~SSFDC_BIT_VALID_ENTRY;
		phys_block = zone_ptr->block_lookup[virt_block] & SSFDC_BIT_BLOCK_ADDR;
		zone_ptr->block_info[phys_block].tag |= SSFDC_BIT_FREE_BLOCK;
		zone_ptr->free_phys_block++;
		zone_ptr->free_virt_block++;
		zone_ptr->used_block--;

		return 0;
	} else {
		printf("Error: zone: 0x%x, unmap block address 0x%x -> NULL\n", zone, virt_block);
		return -EINVAL;
	}
}

int ssfdc_address_translate (struct ssfdc_client_t *client, int zone, int virt_block, int *phys_block)
{
	struct ssfdc_zone_t *zone_ptr;
	unsigned short entry;
	
	if (zone >= client->nr_zone) {
		printf("Error: trying to access beyond physical medium, nr_zone: %d, zone: %d\n", client->nr_zone, zone);
		return -EINVAL;   
	}
	
	zone_ptr = client->zone + zone;
	entry = zone_ptr->block_lookup[virt_block];
	
	if (!(entry & SSFDC_BIT_VALID_ENTRY))
		return -EINVAL;
	
	*phys_block = entry & SSFDC_BIT_BLOCK_ADDR;
	printf("found valid block mapping 0x%x --> 0x%x\n", virt_block, *phys_block);
	
	return 0;
}


struct nand_phy_zone_t * ssfdc_fill_zone_table(void)
{
	int i;	
	struct nand_phy_zone_t *zone;	
	struct ssfdc_client_t *client = &ssfdc_client;	
	int start_block, total_blocks, allow_bad_blocks;

	start_block = get_start_block();
	total_blocks = get_total_blocks();
	allow_bad_blocks = get_allow_bad_blocks();
	
//	printf("%s\n",__FUNCTION__);		
	zone = malloc( sizeof(struct nand_phy_zone_t) * client->nr_zone);
	
	if(!zone) printf("out of mem!\n");		
	for( i = 0; i < client->nr_zone ; i++){	
	    zone[i].total_phys_block = total_blocks;
	    zone[i].total_virt_block = total_blocks - allow_bad_blocks;
	    zone[i].start_phys_block = start_block;
	    zone[i].bad_block_num = allow_bad_blocks;
	}	
	return zone;
}	

int ssfdc_alloc_and_init_data_struct (struct ssfdc_client_t *client)
{
	struct ssfdc_zone_t *zone_ptr;
	int zone;
	struct nand_page_info_t info;
	int phys_page, phys_block, virt_block;
		
       /* if zone information are out of kmalloc limit,
        * we can use mutiple zones
        */
	client->nr_zone = 1;  
       
	client->hw_zone = ssfdc_fill_zone_table();
	client->zone = malloc(sizeof(struct ssfdc_zone_t) * client->nr_zone);
	if (!client->zone)  { printf("1 out of mem!\n"); return -ENOMEM; }
			
	//initialized each zone
	for (zone = 0; zone < client->nr_zone; zone++) {
		printf("reading zone: 0x%x\n", zone);
		zone_ptr = client->zone + zone;
		zone_ptr->free_phys_block = client->hw_zone[zone].total_phys_block;  //add by celion
		zone_ptr->free_virt_block = client->hw_zone[zone].total_virt_block;  //add by celion
		zone_ptr->used_block = 0;
		zone_ptr->bad_block = 0;

		zone_ptr->block_lookup = malloc(sizeof(unsigned short) * client->hw_zone[zone].total_virt_block);
		if (!zone_ptr->block_lookup) { printf("2 out of mem!\n"); return -ENOMEM;}


		for (virt_block = 0; virt_block < client->hw_zone[zone].total_virt_block; virt_block++) {
			zone_ptr->block_lookup[virt_block] = 0;
			zone_ptr->block_lookup[virt_block] &= ~SSFDC_BIT_VALID_ENTRY;
		}

		zone_ptr->block_info = malloc(sizeof(struct block_info) * client->hw_zone[zone].total_phys_block);
		zone_ptr->block_info_index= malloc(sizeof(unsigned short) * (client->hw_zone[zone].total_phys_block + 1)); //added by celion
		if (!zone_ptr->block_info || !zone_ptr->block_info_index) 
			{printf("3 out of mem!\n"); return -ENOMEM;}

		for (phys_block = 0; phys_block < client->hw_zone[zone].total_phys_block; phys_block++) {
			// phys_block is zone relative
			phys_page = (client->hw_zone[zone].start_phys_block + phys_block) * nand_page_per_block; 
			client->dev_id=1;
//			client->io->read_page_info(client->dev_id, phys_page, &info);
			jz_nand_read_page_info(client->dev_id, phys_page, &info);
			zone_ptr->block_info[phys_block].tag = SSFDC_BIT_FREE_BLOCK;
			if( info.lifetime == 0xFFFFFFFF )
				zone_ptr->block_info[phys_block].lifetime = 0;
			else
				zone_ptr->block_info[phys_block].lifetime = info.lifetime;
			
			zone_ptr->block_info_index[phys_block+1] = phys_block;
			
			virt_block = info.block_addr_field;
			
			if (info.block_status != NAND_BLK_STATUS_NORMAL) {
				ssfdc_block_info_map_bad_block(client, zone, phys_block);
				printf("found bad block 0x%x -> 0x%x, block status 0x%x\n", 
				       virt_block, phys_block, info.block_status);
				//ssfdc_dump_page_info(&info);
				continue;
			}
			ssfdc_block_lookup_map_entry(client, zone, virt_block, phys_block);
			
		}

		if( YES == wear_enable )
			ssfdc_heapsort(zone_ptr, client->hw_zone[zone].total_phys_block);
	}	

	int count_badblock=0, i;
	for(i=0; i<client->nr_zone; i++)
		count_badblock += client->zone[i].bad_block;

	printf("total detect badblocks : %d\n",count_badblock);
	// init RS ecc

	if (ssfdc_rs_ecc_init())
		return -ENOMEM;;
	return 0;
}


/* 
 * heapsort: contains two procedures, one is to build the max-heap, 
 * the other is to delete max to gain the sorted array. 
 */
void ssfdc_heapsort(struct ssfdc_zone_t *zone, int heapsize)
{
 	int i;
	
	for (i = heapsize / 2; i >= 1; i--)        /* build max-heap */
		ssfdc_maxheapify(zone, i, heapsize);        
	
	for (i = heapsize; i >= 2; i--){            /* delete max */   
		ssfdc_swap(&zone->block_info_index[1],&zone->block_info_index[i]);
		heapsize--;
		ssfdc_maxheapify(zone, 1, heapsize);
	}
	
}
/* 
 * ssfdc_maxheapify: used to let the value at a[i] "float down" in the 
 * max-heap so that the subtree rooted at index i becomes a max-heap. 
 */
void ssfdc_maxheapify(struct ssfdc_zone_t *zone, int i, int heapsize)
{
	int largest, left, right;
	
	left = Left(i);
	right = Right(i);
	
	if (left <= heapsize && 
	    zone->block_info[zone->block_info_index[left]].lifetime > 
	    zone->block_info[zone->block_info_index[i]].lifetime)
		largest = left;
	else
		largest = i;
	if (right <= heapsize && 
	    zone->block_info[zone->block_info_index[right]].lifetime >
	    zone->block_info[zone->block_info_index[largest]].lifetime)
		largest = right;
	
	if (largest != i){
		ssfdc_swap(&zone->block_info_index[i], &zone->block_info_index[largest]);
		ssfdc_maxheapify(zone, largest, heapsize);    /* recurrsion */
	}		
}

void ssfdc_swap(unsigned short *x, unsigned short *y)
{
	unsigned short temp;
	temp = *x;
	*x = *y;
	*y = temp;
}


void ssfdc_free_and_destroy_data_struct (struct ssfdc_client_t *client)
{
	int zone;
	struct ssfdc_zone_t *zone_ptr;	

	for (zone = 0; zone < client->nr_zone; zone++) {
		zone_ptr = client->zone + zone;
		free(zone_ptr->block_lookup);
		free(zone_ptr->block_info);
		free(zone_ptr->block_info_index);
		zone_ptr->block_lookup = NULL;
		zone_ptr->block_info = NULL;
		zone_ptr->block_info_index = NULL;
	}
	free(client->zone);
	free(client->hw_zone);
	client->zone = NULL;
	client->hw_zone = NULL;
	
	ssfdc_free_rs_ecc_init();
}



int ssfdc_read_page(int phys_page, unsigned char *data, struct nand_page_info_t *info)
{
	struct ssfdc_client_t *client = &ssfdc_client;
	int ret;
	
	ret = jz_nand_read_page(client->dev_id,phys_page,data,info);

	if ( YES == sw_ecc_enable ){
		if( ssfdc_verify_ecc((unsigned char *)data, info, 1) )
			//Error: ecc unrecoverable error on phys page
			return -ECC_ERROR;
		else
			return 0;
	}		
	return ret;
}


int ssfdc_nftl_read(int nr_page, int tmp, int zone, unsigned char *data_ptr)
{
	int i, virt_page, phys_page, virt_block, phys_block, page_offset; 
	struct ssfdc_client_t *client = &ssfdc_client;
	struct nand_page_info_t info;
	struct page_cache_t *page_cache = &client->read_page_cache;

	for (i = 0; i < nr_page; tmp++, i++) {
		virt_page = tmp / sectors_per_page;
		page_offset = tmp % sectors_per_page;

		virt_block = virt_page / nand_page_per_block; 
		data_ptr += (i * HARDSECT);
	
		if ( ssfdc_address_translate(client, zone, virt_block, &phys_block) < 0) {
			// In a Flash Memory device, there might be a logical block that is
			// not allcated to a physical block due to the block not being used.
			// All data returned should be set to 0xFF when accessing this logical 
			// block.
			printf("address translate fail\n");
			memset(data_ptr, 0xFF, HARDSECT);
		} else {
	
			phys_page = ((client->hw_zone[zone].start_phys_block + phys_block) * 
				     nand_page_per_block) + (virt_page % nand_page_per_block);

			if(phys_page != page_cache->page_num ) {
				ssfdc_read_page(phys_page, (unsigned char *)page_cache->data, &info);
				page_cache->page_num = phys_page;
			}
			memcpy(data_ptr,page_cache->data[page_offset],HARDSECT);
		}
	}
	return 0;
}


int ssfdc_find_free_block (struct ssfdc_client_t *client, int zone, int *free_phys_block)
{
	unsigned short i,phys_block;

	struct block_info *block_info_table;
	struct ssfdc_zone_t *zone_ptr;
	unsigned short *block_info_table_index;
	
	zone_ptr = client->zone + zone;    
	block_info_table = zone_ptr->block_info;
	block_info_table_index = zone_ptr->block_info_index;
	
	for (i = 1; i < client->hw_zone[zone].total_phys_block + 1; i++) {
		phys_block = block_info_table_index[i];
		if (block_info_table[phys_block].tag & SSFDC_BIT_BAD_BLOCK)
			continue;
		
		if (block_info_table[phys_block].tag & SSFDC_BIT_FREE_BLOCK) {
			*free_phys_block = phys_block;
			printf("zone: %d, find free phys block: %d,free blocks: %d\n", zone, 
				phys_block,zone_ptr->free_phys_block);
			return 0;
		}
	}
	
	return -ENOMEM;
}

void inline ssfdc_init_cache (struct ssfdc_client_t *client, struct block_cache_t *cache)
{
	cache->in_use = 0; 
	memset(cache->dirty_page, 0, nand_page_per_block*sizeof(unsigned short));
	memset(cache->dirty_sector, 0, nand_page_per_block * sectors_per_page *sizeof(unsigned short));
}

void ssfdc_setup_cache (struct ssfdc_client_t *client, struct block_cache_t *cache, int zone, int virt_block, int new_phys_block, int old_phys_block)
{
	cache->zone = zone;
	cache->old_phys_block = old_phys_block;
	cache->new_phys_block = new_phys_block;
	cache->virt_block = virt_block;
	cache->in_use = 1;
	memset(cache->dirty_page, 0, nand_page_per_block*sizeof(unsigned short));	
	memset(cache->dirty_sector, 0, nand_page_per_block * sectors_per_page *sizeof(unsigned short));
}



int ssfdc_nftl_write(int nr_page, int tmp, int zone, unsigned char *data_ptr)
{
	int i, virt_page;
	int virt_block, new_phys_block, old_phys_block;
	unsigned long page_offset;
	int page_num_in_block;
	struct ssfdc_client_t *client = &ssfdc_client;
	struct block_cache_t *block_cache = &client->write_cache;
	
	for (i = 0; i < nr_page; tmp++, i++) {
		virt_page = tmp / sectors_per_page;
		page_offset = tmp % sectors_per_page;
		
		zone = 0;
		virt_block = virt_page / nand_page_per_block; 
		page_num_in_block = virt_page % nand_page_per_block;
		
		if (ssfdc_address_translate(client, zone, virt_block, &old_phys_block) < 0) {
		       printf("zone: 0x%x, virtual block 0x%x not mapped\n", zone, virt_block);
			
			if (block_cache->in_use){
				ssfdc_flush_cache();
			}
			if ((client->nr_zone > 0) && (DIRECT_MAP))
				new_phys_block = virt_block;
			else {
				if (ssfdc_find_free_block(client, zone, &new_phys_block))
					printf("Bug: find_free_block!!, zone: 0x%x\n", zone);
			}				
			ssfdc_setup_cache(client, block_cache, zone, 
					  virt_block, new_phys_block, new_phys_block);
			ssfdc_block_lookup_map_entry(client, zone, virt_block, new_phys_block);
		} else {
			if ( block_cache->in_use) {
				if ( block_cache->virt_block != virt_block) {
					// Commit before we start a new cache.
					ssfdc_flush_cache();
					ssfdc_block_lookup_unmap_entry(client, zone, virt_block);
					if ((client->nr_zone > 0) && (DIRECT_MAP)) {
						new_phys_block = virt_block;						   
					} else {
						if (ssfdc_find_free_block(client, zone, 
									  &new_phys_block))
							printf("Bug: find_free_block!!, zone: 0x%x\n", zone);
					}				
					ssfdc_setup_cache(client, block_cache, 
							  zone, virt_block, new_phys_block,
							  old_phys_block);					
					ssfdc_block_lookup_map_entry(client, zone,
								     virt_block, new_phys_block);                        
				} else {
					printf("cache hit: 0x%x\n", virt_page);
				}
			} else {
				printf("in else:zone: 0x%x, with existing mapping: 0x%x -> 0x%x\n", zone, virt_block, old_phys_block);
				ssfdc_block_lookup_unmap_entry(client, zone, virt_block);
				if ((client->nr_zone > 0) && (DIRECT_MAP)) {
					new_phys_block = virt_block;
				} else {
					if (ssfdc_find_free_block(client, zone, &new_phys_block))
						printf("Bug: find_free_block!!, zone: 0x%x\n",
						       zone);
				}
				ssfdc_setup_cache(client, block_cache, 
						  zone, virt_block, new_phys_block,
						  old_phys_block);				
				ssfdc_block_lookup_map_entry(client, zone, virt_block,
							     new_phys_block);
			}                        
		}		
		data_ptr += (i * HARDSECT);
		block_cache->dirty_page[page_num_in_block] = 1;
		block_cache->dirty_sector[page_num_in_block][page_offset] = 1;
		memcpy(block_cache->data[page_num_in_block] + page_offset * 512,
		       data_ptr,HARDSECT);//hardsects 512
	} // for            
	return 0;
}

void ssfdc_fill_block_cache(void)
{
	struct ssfdc_client_t *client = &ssfdc_client;
	struct block_cache_t *cache = &client->write_cache;
	struct page_cache_t page_cache;
	struct nand_page_info_t info;
	int phys_block, page, sector, phys_page;
    
	phys_block = client->hw_zone[cache->zone].start_phys_block + cache->old_phys_block;

	for (page = 0; page < nand_page_per_block; page++) {

		if ( 0 == cache->dirty_page[page]) {
			phys_page = (phys_block * nand_page_per_block) + page;
			ssfdc_read_page(phys_page, cache->data[page], &info);
		}else{
			for(sector = 0; sector < sectors_per_page; sector++)
				if( 0==cache->dirty_sector[page][sector] )
					break;

			if(sector != sectors_per_page){
				phys_page = (phys_block * nand_page_per_block) + page;
				ssfdc_read_page(phys_page, (unsigned char *)page_cache.data, &info);
				for(sector = 0; sector < sectors_per_page; sector++)
					if( 0==cache->dirty_sector[page][sector] )
						memcpy(cache->data[page] + sector * 512, page_cache.data[sector], HARDSECT);						
			}
		}
	} 
}

int ssfdc_erase_block(int phys_block)
{
	struct ssfdc_client_t *client = &ssfdc_client;
	struct block_cache_t *cache = &client->write_cache;
        struct ssfdc_zone_t *zone_ptr = &client->zone[cache->zone];
        struct block_info *block_info = zone_ptr->block_info;
	struct nand_page_info_t info;
	int ret;

	ret = jz_nand_erase_block(client->dev_id, phys_block);
	if( YES == wear_enable ){
		memset(&info, 0xff, sizeof(info));
		block_info[cache->new_phys_block].lifetime++;
		info.lifetime = block_info[cache->new_phys_block].lifetime;
		jz_nand_write_page_info(client->dev_id, 
					    phys_block*nand_page_per_block,&info);
		ssfdc_heapsort(zone_ptr, client->hw_zone[cache->zone].total_phys_block);

	}

	return ret;
}

int ssfdc_mark_bad_block_to_nand(int phys_block)
{
	struct ssfdc_client_t *client = &ssfdc_client;
      	struct block_cache_t *cache = &client->write_cache;
	struct nand_page_info_t info;
	int phys_page;
 
	ssfdc_block_lookup_unmap_entry(client,cache->zone,cache->virt_block);
	ssfdc_block_info_map_bad_block(client,cache->zone,phys_block);
	
     	memset(&info, 0xff,sizeof(info));	
	info.block_status = 0x00;
	phys_page = phys_block * nand_page_per_block;

	jz_nand_write_page_info(client->dev_id, phys_page, &info);

	if (ssfdc_find_free_block(client, cache->zone, &cache->new_phys_block))
		printf("Bug: find_free_block!!, zone: 0x%x\n", cache->zone);
	phys_block = client->hw_zone[cache->zone].start_phys_block + 
		cache->new_phys_block;	
	ssfdc_block_lookup_map_entry(client, cache->zone, cache->virt_block, 
				     phys_block);
	
	return phys_block;
}

int ssfdc_program_block(int phys_block)
{
	struct ssfdc_client_t *client = &ssfdc_client;
      	struct block_cache_t *cache = &client->write_cache;
	struct nand_page_info_t info;
	int page, phys_page, ret;
	unsigned char *data_ptr = NULL;
	unsigned char *ecc_ptr = info.ecc_field;
	
	memset(&info, 0xff, sizeof(info));
	info.block_addr_field = cache->virt_block;

	// Finally calculate ecc and program the pages.
	for (page = 0; page < nand_page_per_block; page++) {
		phys_page = (phys_block * nand_page_per_block) + page;
		data_ptr = (unsigned char *)&cache->data[page];
		if ( YES == sw_ecc_enable )
			ssfdc_verify_ecc(data_ptr, &info, 0);
		else
			memset(ecc_ptr,0xff,sizeof(info.ecc_field));
								
		ret = jz_nand_write_page(client->dev_id, phys_page, cache->data[page],&info);
	
		if( ret < 0 )
			break;
	} 	
	if( page != nand_page_per_block ){
		ssfdc_mark_bad_block_to_nand(phys_block);
		return -1;
	}
	return 0;
}
	
int ssfdc_flush_cache (void)
{
	struct ssfdc_client_t *client = &ssfdc_client;
	struct block_cache_t *cache = &client->write_cache;
	struct page_cache_t page_cache;
	struct nand_page_info_t info;
	int phys_block,page, phys_page, ret;
	
	if (!cache->in_use)
		return 0;

	/* un-dirty data read from old block */
	ssfdc_fill_block_cache();
	
	/* erase a free block */
	phys_block = client->hw_zone[cache->zone].start_phys_block + cache->new_phys_block;	
	
	restart:
	do {

		ret = ssfdc_erase_block(phys_block);

	       /* if erase process error, tagged to be bad block, 
	        * and find a new free phys_block to program
	        */
		if( ret < 0 ) {
			phys_block = ssfdc_mark_bad_block_to_nand(phys_block);
		}
			
	}while(ret < 0);
	ret = ssfdc_program_block(phys_block);

	/* if write process error, tagged to be bad block, 
	 * and find a new free phys_block to program
	 */
	if(ret < 0){
		phys_block = ssfdc_mark_bad_block_to_nand(phys_block);
		goto restart;
	}

	/* Now, program new block done correctly */

	/* if write_verify_enable, read the block back with ECC check, 
	 * if Ecc check fail, do Re-programming( back to restart )
	 */
	if( YES == write_verify_enable ){
		for (page = 0; page < nand_page_per_block; page++) {
			phys_page = (phys_block * nand_page_per_block) + page;
			ret = ssfdc_read_page(phys_page, (unsigned char *)page_cache.data, &info);
			if ( ret != 0 ){
				goto restart;
			}
		}	
	}

	if (cache->old_phys_block != cache->new_phys_block) {
			phys_block = client->hw_zone[cache->zone].start_phys_block + cache->old_phys_block;	
			ssfdc_erase_block(phys_block);
	}
	
	ssfdc_init_cache(client,cache);

	// We are done!
	return 0;
}
	
int nand_io_register(void *dev_id, struct nand_io_t *nand_io)
{
	unsigned long flags;
	struct ssfdc_client_t *client = &ssfdc_client;
//	if (client->dev_id) {
//		printf("Error: ssfdc already has a registered client\n");
//		return -EBUSY;
//	}
	if (!dev_id) { printf("Error: missing dev_id\n"); return -EINVAL; }
	if (!nand_io) { printf("Error: missing client info\n"); return -EINVAL; }
	if (!nand_io->open) { printf("Error: missing open op\n"); return -EINVAL; }
	if (!nand_io->release) { printf("Error: missing release op\n"); return -EINVAL; }
	if (!nand_io->scan_id) { printf("Error: missing io_scan op\n"); return -EINVAL; }
	if (!nand_io->read_page) { printf("Error: missing read_page op\n"); return -EINVAL; }
	if (!nand_io->write_page) { printf("Error: missing write_page op\n"); return -EINVAL; }
	if (!nand_io->erase_block) { printf("Error: missing erase_block op\n"); return -EINVAL; }   

	flags=spin_lock_irqsave();
	client->dev_id = dev_id;
	client->io = nand_io;
	ssfdc_init_cache(client, &client->write_cache);
	spin_unlock_irqrestore(flags);

	return 0;
}

int nand_io_unregister (void *dev_id)
{
	unsigned long flags;
	struct ssfdc_client_t *client = &ssfdc_client;
    
//	if (!client->dev_id) {
//		printf("Error: ssfdc has no registered client\n");
//		return -ENODEV;
//	}
	
	if (!dev_id) { printf("Error: missing dev_id\n"); return -EINVAL; }
	
	flags = spin_lock_irqsave();
	client->dev_id = NULL;
	client->io = NULL; //be careful this, it will affect order
	spin_unlock_irqrestore(flags);    
	
	return 0;
}

//int ssfdc_proc_read (char *buf, char **start, off_t offset, int count, int *eof, void *unused)
int ssfdc_proc_read (void)
{	
	int len = 0;
	//unsigned short i, 
	unsigned int capacity = 0;
	struct ssfdc_client_t *client = &ssfdc_client;
	
	len += printf("[Nand Flash]\n");
	
	if (client->dev_id) {
		jz_nand_scan_id(client->dev_id, &client->chip_info);

		len += printf("             Maker:      0x%02x  \n", client->chip_info.make);

		len += printf("            Device:      0x%02x  \n\n", client->chip_info.device);
//		for(i=0; i<client->nr_zone; i++)  //added by celion
		capacity = ( get_total_blocks() - get_allow_bad_blocks() ) * nand_block_size / 1024;

		len += printf("          Capacity:      %d KB\n\n",capacity); //changed by celion

		len += printf("  Total_virt_block:      %d\n", get_total_blocks() - get_allow_bad_blocks());
		len += printf("  Total_phys_block:      %d\n", get_total_blocks());
		len += printf("  Max badblock allowed:  %d\n\n",get_allow_bad_blocks());
		//len += sprintf(buf + len, "  block.in_use:  %d\n\n", client->write_cache.in_use);
	}		
//	*start = NULL;
//	*eof = 1;
        return len;
}

int ssfdc_nftl_init (void)
{
	memset(&ssfdc_client, 0, sizeof(struct ssfdc_client_t));
//	spin_lock_init(&ssfdc_client.lock);
#ifdef CONFIG_PROC_FS    
	create_proc_read_entry(__driver_name__, 0, NULL, ssfdc_proc_read, NULL);
#endif
	printf("%s installed\n", __description__);

	//init global variables
	nand_page_size = get_nand_page_size();
	nand_block_size = get_nand_block_size();
	nand_page_per_block = get_nand_page_per_block();
	sectors_per_page = get_sectors_per_page();

//	hw_ecc_enable = get_hw_ecc_enable();
	sw_ecc_enable = get_sw_ecc_enable();
	wear_enable = get_wear_enable();
	write_verify_enable = get_write_verify_enable();	

//	test();
	return 0;
}




