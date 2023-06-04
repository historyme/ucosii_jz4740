#if UDC_MMC

#include <mass_storage.h>
#include <mmc.h>
#define UCFS_MMC_NAME "mmc:"
static UDC_LUN udcLun;
unsigned int InitDev(unsigned int handle)
{
	printf("sd Disk Mount\r\n");
#if UCFS
	
#endif	
	disk = (unsigned char *)((unsigned int)disk_emu | 0xa0000000);
}
unsigned int ReadDevSector(unsigned int handle,unsigned char *buf,unsigned int startsect,unsigned int sectcount)
{
//	printf("read dev = ia:%x,oa:%x s:%d c:%d\r\n",buf,disk,startsect,sectcount);
	
	memcpy(buf,(void *)(disk + startsect * SECTSIZE),sectcount*SECTSIZE);
}
unsigned int WriteDevSector(unsigned int handle,unsigned char *buf,unsigned int startsect,unsigned int sectcount)
{
	memcpy((void *)(disk + startsect * SECTSIZE),buf,sectcount*SECTSIZE);
}
unsigned int CheckDevState(unsigned int handle)
{
	return 1;
}
unsigned int GetDevInfo(unsigned int handle,PDEVINFO pinfo)
{
	pinfo->hiddennum = 0;
	pinfo->headnum = 2;
	pinfo->sectnum = 4;
	pinfo->partsize = BLOCKNUM;
	pinfo->sectorsize = 512;
}
unsigned int DeinitDev(unsigned handle)
{
	disk = 0;
}
void InitUdcMMC()
{

	
	Init_Mass_Storage();

	udcLun.InitDev = InitDev;
	udcLun.ReadDevSector = ReadDevSector;
	udcLun.WriteDevSector = WriteDevSector;
	udcLun.CheckDevState = CheckDevState;
	udcLun.GetDevInfo = GetDevInfo;
	udcLun.DeinitDev = DeinitDev;
	
	udcLun.DevName = (unsigned int)'RAM1';

	if(RegisterDev(&udcLun))
		printf("UDC Register Fail!!!\r\n");
	
}
#endif
