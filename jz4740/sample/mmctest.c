//#include "mmc_protocol.h"
//struct mmc_csd csd;
void MMCTest()
{
     unsigned short mmcbuf[2048];
     int i;
     unsigned int t[2];
     unsigned int t1[2];
     unsigned int size;

     JZ_StartTimer();
     JZ_GetTimer(t);

     MMC_Initialize();	 
     size=MMC_GetSize();
     printf("Card Size: %d\n",size);	
     for(i = 0; i < 2048;i++)
	     mmcbuf[i] = i;
     printf("write mmc 4 8 sector\r\n");
     MMC_WriteMultiBlock(4,8,(unsigned char *)mmcbuf);
     printf("write mmc 12 8 sector\r\n");
     MMC_WriteMultiBlock(12,8,(unsigned char *)mmcbuf);
     for(i = 0; i < 2048;i++)
	     mmcbuf[i] = 0;
     printf("read mmc 4 8 sector\r\n");    
     MMC_ReadMultiBlock(4,8,(unsigned char *)mmcbuf);
     for(i = 0; i < 2048;i++)
	     if(mmcbuf[i] != i)
		     printf("error %d, %d\r\n", mmcbuf[i],i);
     printf("read mmc 12 8 sector\r\n");    
     for(i = 0; i < 2048;i++)
	     mmcbuf[i] = 0;
     MMC_ReadMultiBlock(12,8,(unsigned char *)mmcbuf);
     for(i = 0; i < 2048;i++)
	     if(mmcbuf[i] != i)
		     printf("error %d, %d\r\n", mmcbuf[i],i);
     printf("Test mmc OK!\n");

     JZ_GetTimer(t1);
     JZ_DiffTimer(t1,t1,t);
     printf("write file use timer %d.%06d s\r\n",t1[1],t1[0]);
     JZ_StopTimer();

}
