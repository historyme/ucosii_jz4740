/*
**********************************************************************
*                          Micrium, Inc.
*                      949 Crestview Circle
*                     Weston,  FL 33327-1848
*
*                            uC/FS
*
*             (c) Copyright 2001 - 2003, Micrium, Inc.
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : mmc_drv.c
Purpose     : File system generic MMC/SD driver
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "fs_port.h"
#include "fs_dev.h" 
#include "fs_lbl.h" 
#include "fs_conf.h"

#if FS_USE_MMC_DRIVER
#include "fs_api.h"
#include "mmc_api.h"
/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static FS_u32   _FS_mmc_logicalstart[FS_MMC_MAXUNIT];     /* start of partition */
static char     _FS_mmc_mbrbuffer[0x200];                 /* buffer for reading MBR */
#define         MULTI_BLOCK_NUM   4  // MULTI_BLOCK_NUM = 2 ^ n 
static unsigned long _FS_mmc_multibuffer[0x200 / 4 * (MULTI_BLOCK_NUM)];                 /* buffer for mulitblock buf */
static unsigned int  multiblockId = -1;
static char     _FS_mmc_diskchange[FS_MMC_MAXUNIT];       /* signal flag for driver */


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/
static inline int FS_MMC_Update()
{
	return 0;
}
static inline int FS__MMC_Init(FS_u32 Unit)
{
	return MMC_Initialize();
}

static int FS__MMC_ReadSector(FS_u32 Unit,unsigned long Sector,unsigned char *pBuffer)
{
	int x = 0;
	if(Sector / MULTI_BLOCK_NUM !=  multiblockId / MULTI_BLOCK_NUM)
	{
		multiblockId = Sector / 4 * 4;
		x = MMC_ReadMultiBlock(multiblockId,MULTI_BLOCK_NUM,(unsigned char *)_FS_mmc_multibuffer);

	}
	FS__CLIB_memcpy(pBuffer,(void *)&(_FS_mmc_multibuffer [ (Sector % 4) * (0x200 / 4)]) , 0x200);


	return x;
}

static int FS__MMC_WriteSector(FS_u32 Unit,unsigned long Sector,unsigned char *pBuffer)
{
	int x = 0;
	//printf("MMC Write Sector %d ,%d\r\n",Sector,multiblockId);
	if(Sector / MULTI_BLOCK_NUM ==  multiblockId / MULTI_BLOCK_NUM)
		FS__CLIB_memcpy((void *)&(_FS_mmc_multibuffer [ (Sector % 4) * (0x200 / 4)]) ,pBuffer, 0x200);
	
	x = MMC_WriteBlock(Sector, pBuffer);
	return x;
        
}
static inline int FS__MMC_ReadMultiSector(FS_u32 Unit,unsigned long Sector,unsigned long Count,unsigned char *pBuffer)
{
	return MMC_ReadMultiBlock(Sector,Count,pBuffer);
}

static inline int FS__MMC_WriteMultiSector(FS_u32 Unit,unsigned long Sector,unsigned long Count,unsigned char *pBuffer)
{
	return MMC_WriteMultiBlock(Sector,Count,pBuffer);
}

static inline int FS__MMC_DetectStatus(FS_u32 Unit)
{
	return MMC_DetectStatus();
}

/*********************************************************************
*
*             _FS_MMC_DevStatus
*
  Description:
  FS driver function. Get status of the media.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  ==1 (FS_LBL_MEDIACHANGED) - The media of the device has changed.
  ==0                       - Device okay and ready for operation.
  <0                        - An error has occured.
*/

static int _FS_MMC_DevStatus(FS_u32 Unit) {
  static int init = 0;
  int x;
  char a;
  //printf("mmc init\r\n");
  if (!init) {
    /* 
       The function is called the first time. For each unit,
       the flag for 'diskchange' is set. That makes sure, that
       FS_LBL_MEDIACHANGED is returned, if there is already a
       media in the reader.
    */
    for (init = 0; init < FS_MMC_MAXUNIT; init++) {
      _FS_mmc_diskchange[init] = 1;
    }
    init = 1;
  }
  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  a = FS__MMC_DetectStatus(Unit);  /* Check if a card is present */
  if (a) {
	_FS_mmc_diskchange[Unit] = 1;  
	printf("sd detect error!\r\n");  
    return -1;  /* No card in reader */
  }
  /* When you get here, then there is a card in the reader */
  a = _FS_mmc_diskchange[Unit];  /* Check if the media has changed */
  if (a) {
    /* 
       A diskchange took place. The following code reads the MBR of the
       card to get its partition information.
    */
    _FS_mmc_diskchange[Unit] = 0;  /* Reset 'diskchange' flag */
    FS__MMC_Init(Unit);            /* Initialize MMC/SD hw */
    x = FS__MMC_ReadSector(Unit, 0, (unsigned char*)&_FS_mmc_mbrbuffer[0]);
    if (x != 0) {
        printf("read sector error!\r\n");
		
		return -1;
    }
	if(FS__CLIB_strncmp(&_FS_mmc_mbrbuffer[54],"FAT",3) == 0)
		_FS_mmc_logicalstart[Unit] = 0;
  	else if(FS__CLIB_strncmp(&_FS_mmc_mbrbuffer[82],"FAT",3) == 0)	
	{
		_FS_mmc_logicalstart[Unit] = 0;
	}
	else
	{
		
    /* Calculate start sector of the first partition */
		_FS_mmc_logicalstart[Unit]  = _FS_mmc_mbrbuffer[0x1c6];
		_FS_mmc_logicalstart[Unit] += (0x100UL * _FS_mmc_mbrbuffer[0x1c7]);
		_FS_mmc_logicalstart[Unit] += (0x10000UL * _FS_mmc_mbrbuffer[0x1c8]);
		_FS_mmc_logicalstart[Unit] += (0x1000000UL * _FS_mmc_mbrbuffer[0x1c9]);
	}
	
	printf("_FS_mmc_logicalstart = %d\r\n",_FS_mmc_logicalstart[0]);
	
	return FS_LBL_MEDIACHANGED;
	
	
  }
  return 0;
}

/*********************************************************************
*
*             _FS_MMC_DevRead
*
  Description:
  FS driver function. Read a sector from the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

static int _FS_MMC_DevRead(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int x = 0;
   
  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  //printf("_FS_MMC_DevRead %d %d\r\n",Sector,_FS_mmc_logicalstart[Unit]);
  x = FS__MMC_ReadSector(Unit, Sector + _FS_mmc_logicalstart[Unit], (unsigned char*)pBuffer);  
  	  
  
  if (x != 0) {
    x = -1;
  }
  return x;
}


/*********************************************************************
*
*             _FS_MMC_DevWrite
*
  Description:
  FS driver function. Write sector to the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be written to the device.
  pBuffer     - Pointer to data to be stored.
 
  Return value:
  ==0         - Sector has been written to the device.
  <0          - An error has occured.
*/

static int _FS_MMC_DevWrite(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int x;

  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  //printf("FS__MMC_WriteSector = %d %d\r\n",Sector,_FS_mmc_logicalstart[Unit]);
  x = FS__MMC_WriteSector(Unit, Sector + _FS_mmc_logicalstart[Unit], (unsigned char*)pBuffer);
  if (x != 0) {
    x = -1;
  }
  return x;
}

/*********************************************************************
*
*             _FS_MMC_DevMultiRead
*
  Description:
  FS driver function. Read a sector from the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  Count       - Sector Number to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

static int _FS_MMC_DevMultiRead(FS_u32 Unit, FS_u32 Sector,FS_u32 Count,void *pBuffer) {
  int x;

  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  x = FS__MMC_ReadMultiSector(Unit, Sector + _FS_mmc_logicalstart[Unit],Count, (unsigned char*)pBuffer);
  if (x != 0) {
    x = -1;
  }
  return x;
}


/*********************************************************************
*
*             _FS_MMC_DevWrite
*
  Description:
  FS driver function. Write sector to the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be written to the device.
  Count       - Sector Number to be written to the device.
  pBuffer     - Pointer to data to be stored.
 
  Return value:
  ==0         - Sector has been written to the device.
  <0          - An error has occured.
*/

static int _FS_MMC_DevMultiWrite(FS_u32 Unit, FS_u32 Sector, FS_u32 Count,void *pBuffer) {
  int x;

  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  x = FS__MMC_WriteMultiSector(Unit, Sector + _FS_mmc_logicalstart[Unit],Count, (unsigned char*)pBuffer);
  //printf("_FS_MMC_DevWrite = %d \r\n",Sector + _FS_mmc_logicalstart[Unit]);
  if (x != 0) {
    x = -1;
  }
  return x;
}


/*********************************************************************
*
*             _FS_MMC_DevIoCtl
*
  Description:
  FS driver function. Execute device command.

  Parameters:
  Unit        - Unit number.
  Cmd         - Command to be executed.
  Aux         - Parameter depending on command.
  pBuffer     - Pointer to a buffer used for the command.
 
  Return value:
  Command specific. In general a negative value means an error.
*/

static int _FS_MMC_DevIoCtl(FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer) {
  FS_u32 *info;
  int x;
  char a;

  Aux = Aux;  /* Get rid of compiler warning */
  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  //printf("MMC IOCTRL = %x %x\r\n",FS_CMD_FLUSH_CACHE,Cmd);
  switch (Cmd) {
    case FS_CMD_CHK_DSKCHANGE:
      break;
    case FS_CMD_GET_DEVINFO:
      if (!pBuffer) {
        return -1;
      }
      info = pBuffer;
      FS__MMC_Init(Unit);
      x = FS__MMC_ReadSector(Unit, 0, (unsigned char*)&_FS_mmc_mbrbuffer[0]);
      if (x != 0) {
        return -1;
      }
      /* hidden */
      *info = _FS_mmc_mbrbuffer[0x1c6];
      *info += (0x100UL * _FS_mmc_mbrbuffer[0x1c7]);
      *info += (0x10000UL * _FS_mmc_mbrbuffer[0x1c8]);
      *info += (0x1000000UL * _FS_mmc_mbrbuffer[0x1c9]);
      info++;
      /* head */
      *info = _FS_mmc_mbrbuffer[0x1c3]; 
      info++;
      /* sec per track */
      *info = _FS_mmc_mbrbuffer[0x1c4]; 
      info++;
      /* size */
      *info = _FS_mmc_mbrbuffer[0x1ca];
      *info += (0x100UL * _FS_mmc_mbrbuffer[0x1cb]);
      *info += (0x10000UL * _FS_mmc_mbrbuffer[0x1cc]);
      *info += (0x1000000UL * _FS_mmc_mbrbuffer[0x1cd]);
      break;
  case FS_CMD_FLUSH_CACHE:
	  FS_MMC_Update();
	  
	  break;
    default:
      break;
  }
  return 0;
}


/*********************************************************************
*
*             Global variables
*
**********************************************************************
*/

const FS__device_type FS__mmcdevice_driver = {
  "MMC device",
  _FS_MMC_DevStatus,
  _FS_MMC_DevRead,
  _FS_MMC_DevWrite,
  _FS_MMC_DevIoCtl
};

#endif /* FS_USE_MMC_DRIVER */
