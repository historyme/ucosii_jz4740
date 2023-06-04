#if (NAND == 0)
#error "No Add NAND defined in makefile!"
#endif
#include "jz_nand.h"
#include "ssfdc_setup.h"
void NandTest()
{
	/*
	unsigned char d[9];
    int i,j;
	for(j = 1;j < 9;j++)
	{
		printf("%d char test\r\n",j);
		
	 	memset(&d[0],0xff,j);
		for(i = 0;i < j; i++)
			if(d[i] != 0xff)
				printf("0xff error  %d->0x%02x\r\n",i,d[i]);
		memset(&d[0],0,j);
		for(i = 0;i < j; i++)
			if(d[i] != 0)
				printf("0x0  error   %d->0x%02x\r\n",i,d[i]);
	}
	
    */
      unsigned int mybuffer[0x200 / 4];

      unsigned char *data_ptr;
	  unsigned int i,j;
	  
	  data_ptr = (unsigned char *)&mybuffer;
      
	  jz_nand_init();
	  ssfdc_nftl_init();
	  ssfdc_nftl_open(1);
      printf("nand write data\r\n");
	  
	  for(i = 0;i < 0x200 / 4;i++)
		  mybuffer[i] = i;

	  for(i = 0; i < 16; i++) 
	  	  ssfdc_nftl_write(1,i,0,data_ptr);

	  ssfdc_flush_cache();
	  printf("nand read data\r\n");
	  
	  for(i = 0; i < 16; i++) 
	  {
           for(j = 0;j < 0x200 / 4;j++)
			   mybuffer[j] = -1;
     
		  ssfdc_nftl_read(1,i,0,data_ptr);
		  for(j = 0;j < 0x200 / 4;j++)
			  if(mybuffer[j] != j)
				  printf("read sector data error %d sector %d word->0x%x\r\n",i,j,mybuffer[j]);
		  
	  }
	  

	  
}
