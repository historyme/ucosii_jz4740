#include "usb.h"

#define BLOCKNUM	16384*7
#define SECTSIZE	512

static u8 disk_emu[BLOCKNUM * SECTSIZE];
//static u8 disk_emu1[BLOCKNUM * SECTSIZE];
static u8 disk_emu1[BLOCKNUM * 0];

u32 get_disk_buf_addr(u8 L)
{
	if (L==0)return (u32)&disk_emu;
	else return (u32)&disk_emu1;
}

u32 get_sector_size()
{
	return SECTSIZE;
}

int massDevWrite(u8 scsiLun,u8 *buf, u32 start_sect, u32 num_sect)
{
	if(scsiLun == 0)
		memcpy(disk_emu + start_sect * SECTSIZE, buf, num_sect * SECTSIZE);
	else
		memcpy(disk_emu1 + start_sect * SECTSIZE, buf, num_sect * SECTSIZE);
	//udelay(num_sect * 1000);
	return num_sect * SECTSIZE;
}

int massDevRead(u8 scsiLun,u8 *buf, u32 start_sect, u32 num_sect)
{
	__dcache_writeback_all();
	__dcache_invalidate_all();

	if(scsiLun == 0)
		memcpy(buf, disk_emu + start_sect * SECTSIZE, num_sect * SECTSIZE);
	else
		memcpy(buf, disk_emu1 + start_sect * SECTSIZE, num_sect * SECTSIZE);
	//udelay(num_sect * 1000);
	return num_sect * SECTSIZE;
}

int massDevInfo(u8 scsiLun,u32 *info)
{
	info[0] = 0;		/* hidden */
	info[1] = 2;		/* head */
	info[2] = 4;		/* sect per track */
	info[3] = BLOCKNUM;	/* block number */
}

int massDevInit()
{
	;
}
int GetMassDevNum()
{
	return 1;
}
