#ifndef __UDC_H__
#define __UDC_H__

#include "usb.h"
#define MAX_EP0_SIZE	64
#define MAX_EP1_SIZE	512

#define USB_HS      0
#define USB_FS      1
#define USB_LS      2

//definitions of EP0
#define USB_EP0_IDLE	0
#define USB_EP0_RX	1
#define USB_EP0_TX	2
/* Define maximum packet size for endpoint 0 */
#define M_EP0_MAXP	64
/* Endpoint 0 status structure */

#define GPIO_UDC_DETE 102
#define IRQ_GPIO_UDC_DETE IRQ_GPIO_0+102


typedef struct _PARTENTRY {
        u8            Part_BootInd;           // If 80h means this is boot partition
        u8            Part_FirstHead;         // Partition starting head based 0
        u8            Part_FirstSector;       // Partition starting sector based 1
        u8            Part_FirstTrack;        // Partition starting track based 0
        u8            Part_FileSystem;        // Partition type signature field
        u8            Part_LastHead;          // Partition ending head based 0
        u8            Part_LastSector;        // Partition ending sector based 1
        u8            Part_LastTrack;         // Partition ending track based 0
        u32           Part_StartSector;       // Logical starting sector based 0
        u32           Part_TotalSectors;      // Total logical sectors in partition
} PARTENTRY;

static __inline__ void usb_setb(u32 port, u8 val)
{
	volatile u8 *ioport = (volatile u8 *)(port);
	*ioport = (*ioport) | val;
}

static __inline__ void usb_clearb(u32 port, u8 val)
{
	volatile u8 *ioport = (volatile u8 *)(port);
	*ioport = (*ioport) & ~val;
}

static __inline__ void usb_setw(u32 port, u16 val)
{
	volatile u16 *ioport = (volatile u16 *)(port);
	*ioport = (*ioport) | val;
}

static __inline__ void usb_clearw(u32 port, u16 val)
{
	volatile u16 *ioport = (volatile u16 *)(port);
	*ioport = (*ioport) & ~val;
}

#endif //__UDC_H__
