/*
 *  File: drivers/ssfdc/ssfdc_ecc.c
 * 
 *  Description: set things that depend on your special condition
 *  
 *  Author: Nancy, Ingenic Ltd
 *
 *  History: 6 June, 2007  -new
 */
#include <nandconfig.h>
#include "ssfdc_hm_ecc.h"
#include "ssfdc_rs_ecc.h"
#include "ssfdc_ecc.h"
#include "ssfdc_nftl.h"

//#define SSFDC_DEBUG
#ifdef SSFDC_DEBUG
#define dprintk(a...)   printf(a)
#else
#define dprintk(a...)   while(0){}
#endif

#ifdef CONFIG_SSFDC_SW_RS_ECC
static data_t inData[512];
static data_t outData[512];
struct rs *init_out=NULL;
#endif


#ifdef CONFIG_SSFDC_SW_HM_ECC
int ssfdc_hm_ecc (unsigned char *data, struct nand_page_info_t *info, unsigned short correct_enable)
{
	unsigned char *data_ptr=NULL;
	unsigned char *ecc_ptr=NULL;
	int ret=0,i;   
	int ecc_steps,datanums_per_ecc_step;

	data_ptr = data;
	ecc_ptr = info->ecc_field;
		
	unsigned char ecc_calc[3];
	datanums_per_ecc_step = 256;
	ecc_steps= CONFIG_SSFDC_NAND_PAGE_SIZE / datanums_per_ecc_step;

	//dprintk("%s: hm_ecc_enable  ecc_steps=%d\n",__FUNCTION__, ecc_steps);
	for(i=0; i< ecc_steps; i++){
		/*if need ecc_correction*/
		if(correct_enable){
			ssfdc_hm_ecc_calculate(data_ptr, ecc_calc); 
			switch (ssfdc_hm_ecc_correct_data(data_ptr, ecc_ptr, ecc_calc)) {
       			case 0: ret = 0; break; // ECC ok
        		case 1: printf("ECC 1 bit correction\n"); ret = 0; break;
        		case 2: printf("ECC 2 bit correction\n"); ret = 0; break;
        		default: ret = -ECC_ERROR; break; // unrecoverable error
			}
		}else
			ssfdc_hm_ecc_calculate(data_ptr, ecc_ptr);
			
		data_ptr += datanums_per_ecc_step;
		ecc_ptr += sizeof(ecc_calc);
	}
	
	return ret;
}
#endif


#ifdef CONFIG_SSFDC_SW_RS_ECC
static int ssfdc_rs_ecc (unsigned char *data, struct nand_page_info_t *info, unsigned short correct_enable)
{
	unsigned char *data_ptr=NULL;
	unsigned char *ecc_ptr=NULL;
	int ret=0,i,j;   
	int ecc_steps,datanums_per_ecc_step;
				
	data_ptr = data;
	ecc_ptr = info->ecc_field;

	data_t parity[8];
	datanums_per_ecc_step = 512;
	ecc_steps= CONFIG_SSFDC_NAND_PAGE_SIZE / datanums_per_ecc_step;

	if(correct_enable){
		for(j=0; j<36; j++)
			if(ecc_ptr[j] != 0xff) 
				break;
		if(j==36)
			return 0;
	}

	for(i=0; i< ecc_steps; i++){
		// init inData
		for(j=0; j< 512; j++)
			inData[j]=(data_t)data_ptr[j];
		// inData(8bits) to outData(9bits)
		Data2Sym(inData, outData);
		/*if need ecc_correction*/
		if(correct_enable){
			eccStore2Parity(ecc_ptr,parity);
			// store ecc code in the tail of outData for DECODE_RS used
			for(j=0; j<8; j++)
				outData[503+j] = parity[j];
			switch ( decode_rs(init_out, outData, NULL, 0) ) {
 			case 0: ret = 0; break; // ECC ok
       			case 1: printf("ECC 1 symbol correction\n"); ret = 0; break;
       			case 2: printf("ECC 2 symbol correction\n"); ret = 0; break;
			case 3: printf("ECC 3 symbol correction\n"); ret = 0; break;
			case 4: printf("ECC 4 symbol correction\n"); ret = 0; break;
      			default: ret = -ECC_ERROR; break; // unrecoverable error
              		}
		}
		else{
			// create Ecc code (parity)
			encode_rs(init_out, outData, parity);
			parity2EccStore(parity,ecc_ptr);
		}
		data_ptr += datanums_per_ecc_step;
		ecc_ptr += 9;
	}
	return ret;
}
#endif

int ssfdc_verify_ecc (unsigned char *data, struct nand_page_info_t *info, unsigned short correct_enable)
{
#ifdef CONFIG_SSFDC_SW_HM_ECC
	return ssfdc_hm_ecc(data,info,correct_enable);
#endif

#ifdef CONFIG_SSFDC_SW_RS_ECC
	return ssfdc_rs_ecc(data,info,correct_enable);
#endif
	return 0;
}


int ssfdc_rs_ecc_init(void)
{
#ifdef CONFIG_SSFDC_SW_RS_ECC
	init_out = init_rs_int(9, 0x211, 1, 1, 8, 0);
	if( NULL == init_out){
		printf("rs_init failed\n");
		return -ENOMEM;
	}
#endif
	return 0;
}

void ssfdc_free_rs_ecc_init(void)
{
#ifdef CONFIG_SSFDC_SW_RS_ECC
	free_rs_int(init_out);
#endif
}

