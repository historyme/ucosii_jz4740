#include "usb.h"
#include "udc.h"
#if 1
#define dprintf(x...) 
//printf(x)
#else
#define dprintf(x...)
#endif
#ifndef NULL
#define NULL
#endif

/*
 * USB mass storage class subclasses
 */
enum UMASS_SUBCLASS
{
	MASS_SUBCLASS_RBC = 1,	// Flash
	MASS_SUBCLASS_8020,	// CD-ROM
	MASS_SUBCLASS_QIC,	// Tape
	MASS_SUBCLASS_UFI,	// Floppy disk device
	MASS_SUBCLASS_8070,	// Floppy disk device
	MASS_SUBCLASS_SCSI	// Any device with a SCSI-defined command set
};


/*
 * USB mass storage class protocols
 */
enum UMASS_PROTOCOL
{
	MASS_PROTOCOL_CBI,	// Command/Bulk/Interrupt
	MASS_PROTOCOL_CBI_CCI,	// Command/Bulk/Interrupt
	MASS_PROTOCOL_BULK = 0x50	// Bulk-Only transport
};

static u32 epout, epin;

static USB_DeviceDescriptor devDesc = 
{
	sizeof(USB_DeviceDescriptor),
	DEVICE_DESCRIPTOR,	//1
	0x0200,     //Version 2.0
	0x08,
	0x01,
	0x50,
	64,	/* Ep0 FIFO size */
	0x07c4,
	0xa4a5,
	0x0000,  //device release number in bin
	0x00,
	0x00,
	0x00,
	0x01
};

#define	CONFIG_DESCRIPTOR_LEN	(sizeof(USB_ConfigDescriptor) + \
				 sizeof(USB_InterfaceDescriptor) + \
				 sizeof(USB_EndPointDescriptor) * 2)

static struct {
	USB_ConfigDescriptor    configuration_descriptor;
	USB_InterfaceDescriptor interface_descritor;
	USB_EndPointDescriptor  endpoint_descriptor[2];
} __attribute__ ((packed)) confDesc = {
	{
		sizeof(USB_ConfigDescriptor),
		CONFIGURATION_DESCRIPTOR,
		CONFIG_DESCRIPTOR_LEN,
		0x01,
		0x01,
		0x00,
		0xc0,	// Self Powered, no remote wakeup
		0x00	// Maximum power consumption 2000 mA
	},
	{
		sizeof(USB_InterfaceDescriptor),
		INTERFACE_DESCRIPTOR,
		0x00,
		0x00,
		0x02,	/* ep number */
		CLASS_MASS_STORAGE,
		MASS_SUBCLASS_SCSI,
		MASS_PROTOCOL_BULK,
		0x04
	},
	{
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(1 << 7) | 1,// endpoint 2 is IN endpoint
			2, /* bulk */
			/* Transfer Type: Bulk;
			 * Synchronization Type: No Synchronization;
			 * Usage Type: Data endpoint
			 */
			//512, /* IN EP FIFO size */
			512,
			16
		},
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(0 << 7) | 1,// endpoint 5 is OUT endpoint
			2, /* bulk */
			/* Transfer Type: Bulk;
			 * Synchronization Type: No Synchronization;
			 * Usage Type: Data endpoint
			 */
			512, /* OUT EP FIFO size */
			16
		}
	}
};

void sendDevDescString(int size)
{
	u16 str_ret[13] = {
		   0x031a,//0x1a=26 byte
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030
		  };
	dprintf("sendDevDescString size = %d\r\n",size);
	if(size >= 26)
		size = 26;
	str_ret[0] = (0x0300 | size);
	HW_SendPKT(0, str_ret,size);
	
}
void setMassStorage(int speed) // speed = 1 is Full speed & 0 is High speed
{

	if(speed == 0)
		devDesc.bcdUSB = 200;
	else
		devDesc.bcdUSB = 100;
}

void sendDevDesc(int size)
{
       switch (size) {
	case 18:
		devDesc.iSerialNumber = GetMassDevNum();
		if(devDesc.iSerialNumber > 0)
			devDesc.iSerialNumber--;
		HW_SendPKT(0, &devDesc, sizeof(devDesc));
		break;
	default:
		if(size < sizeof(devDesc))
			HW_SendPKT(0, &devDesc, size);
		else
			HW_SendPKT(0, &devDesc, sizeof(devDesc));
		break;
	}
}

void sendConfDesc(int size)
{
	switch (size) {
	case 9:
		HW_SendPKT(0, &confDesc, 9);
		break;
	case 8:
		HW_SendPKT(0, &confDesc, 8);
		break;
	default:
		HW_SendPKT(0, &confDesc, sizeof(confDesc));
		break;
	}
}



void usbHandleClassDevReq(u8 *buf)
{
	u8 scsiLUN = 0;	
	switch (buf[1]) {
	case 0xfe:
		scsiLUN = GetMassDevNum();
		if(scsiLUN > 0)
			scsiLUN --;
		dprintf("Get max lun\n");
		
		if (buf[0] == 0xa1)
			HW_SendPKT(0, &scsiLUN, 1);
		break;
	case 0xff:
		dprintf("Mass storage reset\n");
		break;
	}
}


/*
 * Command Block Wrapper (CBW)
 */
#define	CBWSIGNATURE	0x43425355		// "CBSU"
#define	CBWFLAGS_OUT	0x00			// HOST-to-DEVICE
#define	CBWFLAGS_IN	0x80			// DEVICE-to-HOST
#define	CBWCDBLENGTH	16

typedef struct
{
	u32 dCBWSignature;
	u32 dCBWTag;
	s32 dCBWDataXferLength;
	u8 bmCBWFlags;
	/* The bits of this field are defined as follows:
	 *     Bit 7 Direction - the device shall ignore this bit if the
	 *                       dCBWDataTransferLength zero, otherwise:
	 *         0 = Data-Out from host to the device,
	 *         1 = Data-In from the device to the host.
	 *     Bit 6 Obsolete. The host shall set this bit to zero.
	 *     Bits 5..0 Reserved - the host shall set these bits to zero.
	 */
	u8 bCBWLUN : 4,
	   reserved0 : 4;

	u8 bCBWCBLength : 5,
	   reserved1    : 3;
	u8 CBWCB[CBWCDBLENGTH];
} __attribute__ ((packed)) CBW;


/*
 * Command Status Wrapper (CSW)
 */
#define	CSWSIGNATURE			0x53425355	// "SBSU"
#define	CSWSIGNATURE_IMAGINATION_DBX1	0x43425355	// "CBSU"
#define	CSWSIGNATURE_OLYMPUS_C1		0x55425355	// "UBSU"

#define	CSWSTATUS_GOOD			0x0
#define	CSWSTATUS_FAILED		0x1
#define	CSWSTATUS_PHASE_ERR		0x2

typedef struct
{
	u32 dCSWSignature;
	u32 dCSWTag;
	u32 dCSWDataResidue;
	u8 bCSWStatus;
	/* 00h Command Passed ("good status")
	 * 01h Command Failed
	 * 02h Phase Error
	 * 03h and 04h Reserved (Obsolete)
	 * 05h to FFh Reserved
	 */
} __attribute__ ((packed)) CSW;


/*
 * Required UFI Commands
 */
#define	UFI_FORMAT_UNIT			0x04	// output
#define	UFI_INQUIRY			0x12	// input
#define	UFI_MODE_SELECT			0x55	// output
#define	UFI_MODE_SENSE_6		0x1A	// input
#define	UFI_MODE_SENSE_10		0x5A	// input
#define	UFI_PREVENT_MEDIUM_REMOVAL	0x1E
#define	UFI_READ_10			0x28	// input
#define	UFI_READ_12			0xA8	// input
#define	UFI_READ_CAPACITY		0x25	// input
#define	UFI_READ_FORMAT_CAPACITY	0x23	// input
#define	UFI_REQUEST_SENSE		0x03	// input
#define	UFI_REZERO_UNIT			0x01
#define	UFI_SEEK_10			0x2B
#define	UFI_SEND_DIAGNOSTIC		0x1D
#define	UFI_START_UNIT			0x1B
#define	UFI_TEST_UNIT_READY		0x00
#define	UFI_VERIFY			0x2F
#define	UFI_WRITE_10			0x2A	// output
#define	UFI_WRITE_12			0xAA	// output
#define	UFI_WRITE_AND_VERIFY		0x2E	// output
#define	UFI_ALLOW_MEDIUM_REMOVAL	UFI_PREVENT_MEDIUM_REMOVAL
#define	UFI_STOP_UNIT			UFI_START_UNIT



#define	S_CBW		0
#define S_DATA_OUT	1
#define	S_DATA_IN	2
#define S_CSW		3
#define S_NULL          4
//static
 u32 massStat = S_CBW;
static u32 massBuf[1024];
static u32 start_sector;
static u16 nr_sectors;
static CSW csw;
static CBW cbw;

void massReset(void)
{
	massStat = S_CBW;
	start_sector = 0;
	nr_sectors = 0;
}

static u32 swap32(u32 n)
{
	return (((n & 0x000000ff) >> 0) << 24) |
	       (((n & 0x0000ff00) >> 8) << 16) |
	       (((n & 0x00ff0000) >> 16) << 8) |
	       (((n & 0xff000000) >> 24) << 0);
}

typedef struct _CAPACITY_DATA {
	u32 Blocks;
	u32 BlockLen;   
	
}CAPACITY_DATA;
typedef struct _READ_FORMAT_CAPACITY_DATA {
	u8 Reserve1[3];
	u8 CapacityListLen;
	CAPACITY_DATA CurMaxCapacity;
	CAPACITY_DATA CapacityData[30];
	
}READ_FORMAT_CAPACITY_DATA;

static void ProcessScsiReadFormatCapacity(u8 cbwLUN,u32 size)
{
	struct {
		u32 hiddennum;
		u32 headnum;
		u32 secnum;
		u32 partsize;
	} devinfo;
	READ_FORMAT_CAPACITY_DATA readfcd;
	memset(&readfcd,0,sizeof(readfcd));
	readfcd.CapacityListLen = 1;
	massDevInfo(cbwLUN,&devinfo);
	readfcd.CapacityData[0].Blocks = devinfo.partsize-1;
	readfcd.CapacityData[0].BlockLen = 512;
	readfcd.CurMaxCapacity.Blocks = devinfo.partsize-1;
		
	readfcd.CurMaxCapacity.BlockLen = 512;
	readfcd.CurMaxCapacity.BlockLen = (readfcd.CurMaxCapacity.BlockLen << 8) | 0x2;
	if(size > sizeof(READ_FORMAT_CAPACITY_DATA))
		size = sizeof(READ_FORMAT_CAPACITY_DATA);
	csw.dCSWDataResidue = 0;
	csw.bCSWStatus = CSWSTATUS_GOOD;
	csw.dCSWTag = cbw.dCBWTag;	
	HW_SendPKT(epin, (u8 *) &readfcd, size);

}
int USB_HandleUFICmd(void)
{
	u32 tmp;

	//printf("massStat:%d\n", massStat);
	//printf("\nStart USB_HandleUFICmd!");
	switch (massStat) {
	case S_CBW:
		HW_GetPKT(epout, &cbw, sizeof(CBW));
		HW_StopOutTransfer();
		if (cbw.dCBWSignature != CBWSIGNATURE)
			return 0;
		csw.dCSWSignature = CSWSIGNATURE;
		csw.bCSWStatus = CSWSTATUS_GOOD;
		csw.dCSWTag = cbw.dCBWTag;
		csw.dCSWDataResidue = 0;
		dprintf("cbw.Signature:%08x\n", cbw.dCBWSignature);
		dprintf("cbw.dCBWTag:%08x\n", cbw.dCBWTag);
		dprintf("cbw.dCBWDataXferLength:%x\n", cbw.dCBWDataXferLength);
		dprintf("cbw.bmCBWFlags:%08x\n", cbw.bmCBWFlags);
		dprintf("cbw.bCBWLUN:%d\n", cbw.bCBWLUN);
		dprintf("cbw.bCBWCBLength:%d\n", cbw.bCBWCBLength);
		dprintf("cbw.CBWCB[0]:%02x\n", cbw.CBWCB[0]);
		switch (cbw.CBWCB[0]) {
			//case UFI_TEST_UNIT_READY:
			//break;
			
		case UFI_INQUIRY:
		{
			static const u8 inquiry[] = {
				0x00,	// Direct-access device (floppy)
				0x80,	// 0x80	// Removable Media
				0x00,
				0x01,	// UFI device
				0x5B,
				0x00, 0x00, 0x00,
				'I', 'n', 'g', 'e', 'n', 'i', 'c', ' ',
				'J', 'z', 'S', 'O', 'C', ' ', 'U', 'S',
				'B', '-', 'D', 'I', 'S', 'K', ' ', ' ',
				'0', '1', '0', '0'
			};
			u32 size = sizeof(inquiry);
			if(cbw.dCBWDataXferLength < sizeof(inquiry))
				size = cbw.dCBWDataXferLength;
			HW_SendPKT(epin, inquiry, sizeof(inquiry));
			massStat = S_NULL;
			break;
		}

		case UFI_REQUEST_SENSE:
		{
			static const u8 sense[] = {
				0x70,
				0x00,
				0x05,
				0x00, 0x00, 0x00, 0x00,
				0x0c,
				0x00, 0x00, 0x00, 0x00,
				0x20,
				0x00,
				0x00,
				0x00, 0x00, 0x00
			};
			HW_SendPKT(epin, sense, sizeof(sense));
			massStat = S_DATA_OUT;
			break;
		}

		case UFI_READ_CAPACITY:
		{
			struct {
				u32 hiddennum;
				u32 headnum;
				u32 secnum;
				u32 partsize;
			} devinfo;
			u32 resp[2];

//			FS_IoCtl(USR_DISK, FS_CMD_GET_DEVINFO, 0, (void*)&devinfo);
//			resp[0] = cpu_to_be32(devinfo.partsize - 1);
//			resp[1] = cpu_to_be32(FS_FAT_SEC_SIZE);
//			USB_EndpOut(1, (const u8*)resp, sizeof(resp), false);
			massDevInfo(cbw.bCBWLUN,&devinfo);
			resp[0] = swap32(devinfo.partsize-1); /* last sector */
			resp[1] = swap32(512);		/* sector size */
			HW_SendPKT(epin, (u8 *)resp, sizeof(resp));
			massStat = S_DATA_OUT;
			break;
		}

		case UFI_READ_10:
		case UFI_WRITE_10:
		case UFI_WRITE_AND_VERIFY:
		{
			
			start_sector =
				((u32)cbw.CBWCB[2] << 24) |
				((u32)cbw.CBWCB[3] << 16) |
				((u32)cbw.CBWCB[4] << 8) |
				(u32)cbw.CBWCB[5];
			nr_sectors   =
				((u16)cbw.CBWCB[7] << 8) | (u16)cbw.CBWCB[8];

			dprintf("s:%d n:%d\n", start_sector, nr_sectors);

			if (cbw.CBWCB[0] == UFI_READ_10) {
				//HW_StopOutTransfer();
				if (nr_sectors > 8) {
					massDevRead(cbw.bCBWLUN ,massBuf, start_sector, 8);
					HW_SendPKT(epin, massBuf, 8*512);
					start_sector += 8;
					nr_sectors -= 8;
				} else {
					massDevRead(cbw.bCBWLUN,massBuf, start_sector,
						    nr_sectors);
					HW_SendPKT(epin, massBuf,
						   nr_sectors * 512);
					start_sector += nr_sectors;
					nr_sectors = 0;
					
				}
				
				massStat = S_DATA_OUT;
				
			} else
			{
					massStat = S_DATA_IN;
					HW_StartOutTransfer();
			}
			
			break;
		}

		case UFI_VERIFY:
//		FS_IoCtl(USR_DISK, FS_CMD_FLUSH_CACHE, 0, 0);
			massStat = S_CSW;
			break;

		case UFI_READ_FORMAT_CAPACITY:
			ProcessScsiReadFormatCapacity(cbw.bCBWLUN,cbw.dCBWDataXferLength);
			
			massStat = S_NULL;
			
			break;
		case UFI_MODE_SENSE_10:
		case UFI_MODE_SENSE_6:
	    {
			static const unsigned char sensedata[] = {
				0x00,0x06, // lenght
				0x00,      // default media
				0x00,      // bit 7 is write protect
				0x00,0x00,0x00,0x00 //reserved
			};

			HW_SendPKT(epin, sensedata,sizeof(sensedata));
			
			//csw.dCSWTag = cbw.dCBWTag;
			//csw.dCSWDataResidue = 0;
			//csw.bCSWStatus = CSWSTATUS_GOOD;	/* failed ? */
			massStat = S_NULL;
		}
		
			break;
		default:
			massStat = S_CSW;
			break;
		}

		break;
	case S_DATA_OUT:
		
		if (!hwTxFifoCount()) {
			if (!nr_sectors) {
				massStat = S_CSW;
				
				break;
			}
			
			if (nr_sectors > 8) {
				massDevRead(cbw.bCBWLUN,massBuf, start_sector, 8);
				HW_SendPKT(epin, massBuf, 8*512);
				start_sector += 8;
				nr_sectors -= 8;
			} else {
				massDevRead(cbw.bCBWLUN,massBuf, start_sector, nr_sectors);
				HW_SendPKT(epin, massBuf, nr_sectors * 512);
				start_sector += nr_sectors;
				nr_sectors = 0;
				
			}
			return;
		}
		break;
	case S_DATA_IN:
		
		tmp = hwRxFifoCount();
		if ((nr_sectors >= 8) && (tmp >= 4096)) {
			HW_GetPKT(epout, massBuf, 4096);
			massDevWrite(cbw.bCBWLUN,massBuf, start_sector, 8);
			start_sector += 8;
			nr_sectors -= 8;
		} else if ((tmp == nr_sectors * 512) && (nr_sectors < 8)) {
			HW_GetPKT(epout, massBuf, tmp);
			massDevWrite(cbw.bCBWLUN,massBuf, start_sector, nr_sectors);
			start_sector += nr_sectors;
			nr_sectors = 0;
		}
		if (nr_sectors == 0)
		{
				massStat = S_CSW;
				
		}
		break;
	case S_CSW:
		if (hwTxFifoCount())
		{
			//printf("start trans\r\n");
			return;
		}
		
		massStat = S_CBW;
		HW_StartOutTransfer();
	
		
		break;
	case S_NULL:
		
		if (!hwTxFifoCount())
		{
			
			//csw.dCSWTag = CSWSTATUS_FAILED;
			//csw.dCSWDataResidue = cbw.dCBWDataXferLength;
			//csw.bCSWStatus = 0;	/* failed ? */
			massStat = S_CSW;
		}
		
		break;
	}

	if (massStat == S_CSW)
	{
		//printf("HW_SendPK %d  ep %d\r\n",cbw.CBWCB[0],epin);
		HW_SendPKT(epin, &csw, sizeof(CSW));
		
	}
	return 0;
}

void mass_storage_assignep(u32 out, u32 out_size, u32 in, u32 in_size)
{
	epout = out;
	epin = in;
	confDesc.endpoint_descriptor[0].bEndpointAddress = (1<<7) | epin;
	confDesc.endpoint_descriptor[0].wMaxPacketSize = in_size;
	confDesc.endpoint_descriptor[1].bEndpointAddress = (0<<7) | epout;
	confDesc.endpoint_descriptor[1].wMaxPacketSize = out_size;
}

void init_mass_storage(u16 out_size,u16 in_size)
{
	massDevInit();
	massReset();
	mass_storage_assignep(1, out_size, 1, in_size);
}
