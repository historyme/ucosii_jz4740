#include <includes.h>
#include <regs.h>
#include <ops.h>
#include "usb.h"
#include "udc.h"
#define dprintf(x...)

static u32 rx_buf[1024];
static u32 tx_buf[1024];
static u32 tx_size, rx_size, finished;
static u32 fifo, curep;
static OS_EVENT *udcEvent;
void (*tx_done)(void) = NULL;

u32 hwRxFifoCount(void) { return rx_size; }
u32 hwTxFifoCount(void) { return tx_size - finished; }

#define UDC_TASK_PRIO	1

// UDC FIFO
#define RXFIFO    		(UDC_RXFIFO)    /* EP0 OUT, EP5-7 OUT */
#define TXFIFOEP0 		(UDC_TXFIFOEP0) /* EP0 IN */
#define TXFIFOEP1 		(TXFIFOEP0 + MAX_EP0_SIZE) /* EP1 IN */
#define TXFIFOEP2 		(TXFIFOEP1 + MAX_EP1_SIZE) /* EP2 IN */
#define TXFIFOEP3 		(TXFIFOEP2 + MAX_EP2_SIZE) /* EP3 IN */
#define TXFIFOEP4 		(TXFIFOEP3 + MAX_EP3_SIZE) /* EP4 IN */
static u32 fifoaddr[] = {
	TXFIFOEP0, TXFIFOEP1, TXFIFOEP2, TXFIFOEP3,
	TXFIFOEP4, RXFIFO, RXFIFO, RXFIFO
};

static u32 fifosize[] = {
	MAX_EP0_SIZE, MAX_EP1_SIZE, MAX_EP2_SIZE, MAX_EP3_SIZE,
	MAX_EP4_SIZE, MAX_EP5_SIZE, MAX_EP6_SIZE, MAX_EP7_SIZE,
};


static void udc_reset(void)
{
	REG_UDC_DevCFGR = 0x17;
	REG_UDC_DevCR = 0;
	REG_UDC_DevIntMR = 0x30; /* just allows UR, SI, SC */
//	REG_UDC_DevIntMR = 0x1f;
	REG_UDC_EPIntMR  = 0;

	REG_UDC_EP0InCR = (0 << 4) | (1 << 1);
	REG_UDC_EP0InCR = (0 << 4);
	REG_UDC_EP1InCR = (3 << 4) | (1 << 1);
	REG_UDC_EP1InCR = (3 << 4);
	REG_UDC_EP2InCR = (2 << 4) | (1 << 1);
	REG_UDC_EP2InCR = (2 << 4);
	REG_UDC_EP3InCR = (2 << 4) | (1 << 1);
	REG_UDC_EP3InCR = (2 << 4);
	REG_UDC_EP4InCR = (1 << 4) | (1 << 1);
	REG_UDC_EP4InCR = (1 << 4);

	REG_UDC_EP0OutCR = (0 << 4);
	REG_UDC_EP5OutCR = (2 << 4);
	REG_UDC_EP6OutCR = (2 << 4);
	REG_UDC_EP7OutCR = (1 << 4);

	REG_UDC_EP0InBSR = MAX_EP0_SIZE/4;
	REG_UDC_EP1InBSR = MAX_EP1_SIZE/4;
	REG_UDC_EP2InBSR = MAX_EP2_SIZE/4;
	REG_UDC_EP3InBSR = MAX_EP3_SIZE/4;
	REG_UDC_EP4InBSR = MAX_EP4_SIZE/4;

	REG_UDC_EP0InMPSR = MAX_EP0_SIZE;
	REG_UDC_EP1InMPSR = MAX_EP1_SIZE;
	REG_UDC_EP2InMPSR = MAX_EP2_SIZE;
	REG_UDC_EP3InMPSR = MAX_EP3_SIZE;
	REG_UDC_EP4InMPSR = MAX_EP4_SIZE;

	REG_UDC_EP0OutMPSR = MAX_EP0_SIZE;
	REG_UDC_EP5OutMPSR = MAX_EP5_SIZE;
	REG_UDC_EP6OutMPSR = MAX_EP6_SIZE;
	REG_UDC_EP7OutMPSR = MAX_EP7_SIZE;

	REG_UDC_STCMAR = 0xffff;

	REG_UDC_EP0InfR = (MAX_EP0_SIZE << 19) | (0 << 15) | (0 << 11) | (0x1 << 7) | (0 << 5) | (0 << 4) | (0 << 0);
	REG_UDC_EP1InfR = (MAX_EP1_SIZE << 19) | (0 << 15) | (0 << 11) | (0x1 << 7) | (3 << 5) | (1 << 4) | (1 << 0);
	REG_UDC_EP2InfR = (MAX_EP2_SIZE << 19) | (0 << 15) | (0 << 11) | (0x1 << 7) | (2 << 5) | (1 << 4) | (2 << 0);
	REG_UDC_EP3InfR = (MAX_EP3_SIZE << 19) | (0 << 15) | (0 << 11) | (0x1 << 7) | (2 << 5) | (1 << 4) | (3 << 0);
	REG_UDC_EP4InfR = (MAX_EP4_SIZE << 19) | (0 << 15) | (0 << 11) | (0x1 << 7) | (1 << 5) | (1 << 4) | (4 << 0);
	REG_UDC_EP5InfR = (MAX_EP5_SIZE << 19) | (0 << 15) | (0 << 11) | (0x1 << 7) | (2 << 5) | (0 << 4) | (5 << 0);
	REG_UDC_EP6InfR = (MAX_EP6_SIZE << 19) | (0 << 15) | (0 << 11) | (0x1 << 7) | (2 << 5) | (0 << 4) | (6 << 0);
	REG_UDC_EP7InfR = (MAX_EP7_SIZE << 19) | (0 << 15) | (0 << 11) | (0x1 << 7) | (1 << 5) | (0 << 4) | (7 << 0);	
}

static void udcReadFifo(u8 *ptr, int size)
{
	u32 *d = (u32 *)ptr;
	int s;
	s = (size + 3) >> 2;
	while (s--)
		*d++ = REG32(UDC_RXFIFO);
	REG32(UDC_RXCONFIRM);

#if 0
	dprintf("recv:(%d)", size);
	for (s=0;s<size;s++) {
		if (s % 16 == 0)
			dprintf("\n");
		dprintf(" %02x", *(ptr+s));
	}
	dprintf("\n");
#endif
}

static void udcWriteFifo(u8 *ptr, int size)
{
	u32 *d = (u32 *)ptr;
	u8 *c;
	int s, q;

#if 0
	dprintf("send:(%d)", size);
	for (s=0;s<size;s++) {
		if (s % 16 == 0)
			dprintf("\n");
		dprintf(" %02x", ptr[s]);
	}
	dprintf("\n");
#endif
	if (size > 0) {
		s = size >> 2;
		while (s--)
			REG32(fifo) = *d++;
		q = size & 3;
		if (q) {
			c = (u8 *)d;
			while (q--)
				REG8(fifo) = *c++;
		}
	} else {
		REG32(UDC_TXZLP) = 0;
		REG32(fifo) = 0x12345678;
	}
	REG32(UDC_TXCONFIRM) = 0;
}

void HW_SendZeroPKT(int ep)
{
	REG32(UDC_TXZLP) = 0;
	REG32(fifoaddr[ep]) = 0x12345678;
	REG32(UDC_TXCONFIRM) = 0;
}

void HW_SendPKT(int ep, const u8 *buf, int size)
{
	dprintf("EP%d send pkt :%d\n", ep, size);
	memcpy((void *)tx_buf, buf, size);
	fifo = fifoaddr[ep];
	tx_size = size;
	finished = 0;
}

void HW_GetPKT(int ep, const u8 *buf, int size)
{
	dprintf("EP%d read pkt :%d\n", ep, size);
	memcpy((void *)buf, (u8 *)rx_buf, size);
	if (rx_size > size)
		rx_size -= size;
	else {
		size = rx_size;
		rx_size = 0;
	}
	memcpy((u8 *)rx_buf, (u8 *)((u32)rx_buf+size), rx_size);
}

static int udcDevIntrPacket(u32 devintr)
{
	switch (devintr & ((1 << 7)-1)) {
	case UDC_DevIntR_SOF:
		dprintf("SOF\n");
		return -1;
	case UDC_DevIntR_US:
		dprintf("STALL\n");
		return -1;
	case UDC_DevIntR_UR:
		finished = 0;
		dprintf("RESET\n");
		return -1;
	case UDC_DevIntR_SI:
		dprintf("SET IF\n");
		usbEncodeDevReq(rx_buf, SET_INTERFACE);
		rx_size = 8;
		break;
	case UDC_DevIntR_SC:
		dprintf("SET CONF\n");
		usbEncodeDevReq(rx_buf, SET_CONFIGURATION);
		rx_size = 8;
		break;
	default:
		dprintf("devintr:%08x\n", devintr);
		break;
	}
	return 0;
}

static void udcProc(void)
{
	u32 devintr, epintr, epsr;

	devintr = REG_UDC_DevIntR;
	epintr = REG_UDC_EPIntR;

	if (devintr) {
		REG_UDC_DevIntR = devintr;
		if (!udcDevIntrPacket(devintr)) {
			usbHandleDevReq(rx_buf);
		}
		rx_size = 0;
	}

	if (!epintr) {
		REG_UDC_EPIntR = epintr;
		return;
	}

	switch (epintr & UDC_EPIntR_OUTEP_MASK) {
	case UDC_EPIntR_OUTEP0:
		epsr = REG_UDC_EP0OutSR;
		REG_UDC_EPIntR = UDC_EPIntR_OUTEP0;
		REG_UDC_EP0OutSR &= ~UDC_EPSR_OUT_MASK;

		if (epsr & UDC_EPSR_OUT_RCVSETUP) {
			USB_DeviceRequest *dreq;
			dprintf("0:R ST\n");
			udcReadFifo((u8 *)rx_buf, sizeof(USB_DeviceRequest));
			dreq = (USB_DeviceRequest *)rx_buf;
			dprintf("bmRequestType:%02x\nbRequest:%02x\n"
				"wValue:%04x\nwIndex:%04x\n"
				"wLength:%04x\n",
				dreq->bmRequestType,
				dreq->bRequest,
				dreq->wValue,
				dreq->wIndex,
				dreq->wLength);
			usbHandleDevReq(rx_buf);
		} else {
			dprintf("0:R DATA\n");
			if (__udc_ep0out_packet_size() == 0)
				/* ack zero pack */
				REG32(UDC_RXCONFIRM);
			else
				;// EP0 OUT Data
		}
		rx_size = 0;
		break;
	case UDC_EPIntR_OUTEP5:
		epsr = REG_UDC_EP5OutSR;
		REG_UDC_EPIntR = UDC_EPIntR_OUTEP5;
		REG_UDC_EP5OutSR &= ~UDC_EPSR_OUT_MASK;
		if (epsr & UDC_EPSR_OUT_RCVDATA) {
			u32 size;
			size = __udc_ep5out_packet_size();
			udcReadFifo((u8 *)((u32)rx_buf+rx_size), size);
			rx_size += size;
		}
		USB_HandleUFICmd();
		break;
	case 0:
		break;
	default:
		dprintf("epintr:%08x\n", epintr);
		REG_UDC_EPIntR = UDC_EPIntR_OUTEP_MASK;
		break;
	}

	switch (epintr & UDC_EPIntR_INEP_MASK) {
	case UDC_EPIntR_INEP0:
		epsr = REG_UDC_EP0InSR;
		if (epsr & UDC_EPSR_IN) {
			if (tx_size - finished <= 8) {
				udcWriteFifo((u8 *)((u32)tx_buf+finished),
					     tx_size - finished);
				finished = tx_size;
				
			} else {
				udcWriteFifo((u8 *)((u32)tx_buf+finished), 8);
				finished += 8;
			}
			REG_UDC_EP0InSR &= ~UDC_EPSR_IN;
		}
		REG_UDC_EPIntR = UDC_EPIntR_INEP0;
		break;
	case UDC_EPIntR_INEP2:
		epsr = REG_UDC_EP2InSR;
		if (epsr & UDC_EPSR_IN) {
			if (tx_size - finished <= MAX_EP2_SIZE) {
				udcWriteFifo((u8 *)((u32)tx_buf+finished),
					     tx_size - finished);
				finished = tx_size;
				USB_HandleUFICmd();
			} else {
				udcWriteFifo((u8 *)((u32)tx_buf+finished),
					     MAX_EP2_SIZE);
				finished += MAX_EP2_SIZE;
			}
			
			REG_UDC_EP2InSR &= ~UDC_EPSR_IN;
		}
		REG_UDC_EPIntR = UDC_EPIntR_INEP2;
		break;
	case 0:
		break;
	default:
		REG_UDC_EPIntR = UDC_EPIntR_INEP_MASK;
		dprintf("epintr:%08x\n", epintr);
		break;
	}
}

static void udcIntrHandler(unsigned int arg)
{
	u8 err;
	__intc_mask_irq(IRQ_UDC);
	OSSemPost(udcEvent);
}

static void udcTaskEntry(void *arg)
{
	u8 err;

	dprintf("Init UDC\n");
	udcEvent = OSSemCreate(0);

	request_irq(IRQ_UDC, udcIntrHandler, 0);
	init_mass_storage();
	
	__harb_usb0_udc();

	udc_reset();

	while (1) {
		OSSemPend(udcEvent, 0, &err);
		udcProc();
		__intc_unmask_irq(IRQ_UDC);
	}
}


#define UDC_TASK_STK_SIZE	1024 * 5
static OS_STK udcTaskStack[UDC_TASK_STK_SIZE];

void udc_init(void)
{
	
	OSTaskCreate(udcTaskEntry, (void *)0,
		     (void *)&udcTaskStack[UDC_TASK_STK_SIZE - 1],
		     UDC_TASK_PRIO);
}

