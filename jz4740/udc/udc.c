#include <includes.h>
#include "jz4740.h"
#include "usb.h"
#include "udc.h"
#define GPIO_UDC_DETE_PIN (32 * 3 + 6)
#define GPIO_UDC_DETE GPIO_UDC_DETE_PIN
#define IRQ_GPIO_UDC_DETE (IRQ_GPIO_0 + GPIO_UDC_DETE) 
#define dprintf(x...) //printf(x)

static u32 rx_buf[1024];
static u32 tx_buf[1024];
static u32 tx_size = 0, rx_size = 0, finished = 0;
static u32 fifo, curep;
static OS_EVENT *udcEvent;

u8 ep0state;
u8 USB_Version;
u8 is_connect;
//u16 MAX_EP1_SIZE;
u8 in_mask = 0;
u8 out_mask = 0;

void (*tx_done)(void) = NULL;

u32 hwRxFifoCount(void) { return rx_size; }
u32 hwTxFifoCount(void) { return tx_size - finished; }

#define UDC_TASK_PRIO	2

// UDC FIFO
#define TXFIFOEP0 USB_FIFO_EP0

static u32 fifoaddr[] = 
{
	TXFIFOEP0, TXFIFOEP0+4 ,TXFIFOEP0+8
};

static u32 fifosize[] = {
	MAX_EP0_SIZE, MAX_EP1_SIZE
};

static void udc_reset(void)
{
	u8 byte;
	//data init
	ep0state = USB_EP0_IDLE;
	//__cpm_stop_udc();
	/* Enable the USB PHY */
	//REG_CPM_SCR |= CPM_SCR_USBPHY_ENABLE;
	/* Disable interrupts */
	jz_writew(USB_REG_INTRINE, 0);
	jz_writew(USB_REG_INTROUTE, 0);
	jz_writeb(USB_REG_INTRUSBE, 0);
	jz_writeb(USB_REG_FADDR,0);
	//jz_writeb(USB_REG_POWER,0x60);   //High speed
	jz_writeb(USB_REG_INDEX,0);
	jz_writeb(USB_REG_CSR0,0xc0);
	jz_writeb(USB_REG_INDEX,1);
	jz_writew(USB_REG_INMAXP,512);
	jz_writew(USB_REG_INCSR,0x2048);
	jz_writeb(USB_REG_INDEX,1);
	jz_writew(USB_REG_OUTMAXP,512);
	jz_writew(USB_REG_OUTCSR,0x0090);
	
	//jz_writew(USB_REG_INTRINE,0x3);   //enable intr
	

	jz_writew(USB_REG_INTRINE,0x1);   //enable intr ep0
	
	jz_writew(USB_REG_INTROUTE,0x2);
	jz_writeb(USB_REG_INTRUSBE,0x4);

	byte=jz_readb(USB_REG_POWER);
	dprintf("\nREG_POWER: %02x",byte);
	
	if ((byte&0x10)==0) 
	{
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_INMAXP,64);
		jz_writew(USB_REG_INCSR,0x2048);
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_OUTMAXP,64);
		jz_writew(USB_REG_OUTCSR,0x0090);
		USB_Version=USB_FS;
		fifosize[1]=64;
		init_mass_storage(64,64);
		printf("usb1.1 reset\r\n");
	
	}
	else
	{
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_INMAXP,512);
		jz_writew(USB_REG_INCSR,0x2048);
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_OUTMAXP,512);
		jz_writew(USB_REG_OUTCSR,0x0090);
		USB_Version=USB_HS;
		fifosize[1]=512;
		init_mass_storage(512,512);
		printf("usb2.0 reset\r\n");
	}
		out_mask = 0;
		in_mask = 0;
	dprintf("\n return!");
}

static void udcReadFifo(u8 *ptr, int size)
{
	u32 *d = (u32 *)ptr;
	int s = size / 4;
	u32 x;
	while(s--)
	{
		x = REG32(fifo);
		*ptr++ = (x >> 0)& 0x0ff;
		*ptr++ = (x >> 8)  & 0x0ff;
		*ptr++ = (x >> 16) & 0x0ff;
		*ptr++ = (x >> 24) & 0xff;
	}
	s = size % 4;
	while(s--)
		*ptr++ = REG8(fifo);
       
#if 0
	ptr -= size;
	
	printf("recv:(%d)", size);
	for (s=0;s<size;s++) {
		if (s % 16 == 0)
			printf("\n");
		printf(" %02x", *(ptr+s));
	}
	printf("\n");
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
	} 
}

void HW_SendZeroPKT(int ep)
{
	jz_writeb(USB_REG_INDEX, ep); 
	REG32(fifo)=0;
	usb_setw(USB_REG_INCSR, USB_INCSR_INPKTRDY);
	usb_setw(USB_REG_CSR0, USB_CSR0_DATAEND);
}

void HW_SendPKT(int ep, const u8 *buf, int size)
{
	u16 csr;
	
	//printf("EP%d send pkt :%d\n", ep, size);
	memcpy((void *)tx_buf, buf, size);
	fifo = fifoaddr[ep];
	tx_size = size;
	finished = 0;

	if (ep!=0)
	{
		//jz_readw(USB_REG_INTRIN);
		usb_setw(USB_REG_INTRINE, 2); // enable ep1 in interrupt
		jz_writeb(USB_REG_INDEX, ep);
		csr = jz_readw(USB_REG_INCSR);
		in_mask = 0;
		if((csr & 2) == 0)
		{
			
			if (tx_size - finished <= fifosize[ep]) 
			{
				udcWriteFifo((u8 *)((u32)tx_buf+finished),
							 tx_size - finished);
				usb_setw(USB_REG_INCSR, USB_INCSR_INPKTRDY);
				finished = tx_size;
			} else 
			{
				
				udcWriteFifo((u8 *)((u32)tx_buf+finished),
							 fifosize[ep]);
				usb_setw(USB_REG_INCSR, USB_INCSR_INPKTRDY);
				finished += fifosize[ep];
				

			}
		}
		
	}
}

void HW_GetPKT(int ep, const u8 *buf, int size)
{
	dprintf("EP%d read pkt :%d\n", ep, size);
	memcpy((void *)buf, (u8 *)rx_buf, size);
	fifo = fifoaddr[ep];
	if (rx_size > size)
		rx_size -= size;
	else {
		size = rx_size;
		rx_size = 0;
	}
	memcpy((u8 *)rx_buf, (u8 *)((u32)rx_buf+size), rx_size);
}

void HW_StartOutTransfer()
{
	usb_setw(USB_REG_INTROUTE,2);// enable ep1 out interrupt for cbw and data
	//printf("S ->");
	out_mask = 0;
}
void HW_StopOutTransfer()
{
	usb_clearw(USB_REG_INTROUTE, 2);// enable ep1 out interrupt for cbw and data
	out_mask = 1;
	//printf("E\n");
}

void usbHandleStandDevReq(u8 *buf)
{
	USB_DeviceRequest *dreq = (USB_DeviceRequest *)buf;
	switch (dreq->bRequest) {
	case GET_DESCRIPTOR:
		if (dreq->bmRequestType == 0x80)	/* Dev2Host */
			switch(dreq->wValue >> 8) 
			{
			case DEVICE_DESCRIPTOR:
				dprintf("get device\n");
				sendDevDesc(dreq->wLength);
				break;
			case CONFIGURATION_DESCRIPTOR:
				dprintf("get config\n");
				sendConfDesc(dreq->wLength);
				break;
			case STRING_DESCRIPTOR:
				if (dreq->wLength == 0x02)
					HW_SendPKT(0, "\x04\x03", 2);
				else
					sendDevDescString(dreq->wLength);
				//HW_SendPKT(0, "\x04\x03\x09\x04", 2);
				break;
			}
		dprintf("\nSet ep0state=TX!");
		ep0state=USB_EP0_TX;
		
		break;
	case SET_ADDRESS:
		dprintf("\nSET_ADDRESS!");
		jz_writeb(USB_REG_FADDR,dreq->wValue);
		break;
	case GET_STATUS:
		switch (dreq->bmRequestType) {
		case 80:	/* device */
			HW_SendPKT(0, "\x01\x00", 2);
			break;
		case 81:	/* interface */
		case 82:	/* ep */
			HW_SendPKT(0, "\x00\x00", 2);
			break;
		}
		ep0state=USB_EP0_TX;
		break;
	case CLEAR_FEATURE:
	case SET_CONFIGURATION:
	case SET_INTERFACE:
	case SET_FEATURE:
#if 0
		dprintf("Send ZERO packet at 0.\n");
		HW_SendZeroPKT(0);
#endif
		break;
	}
}

void EP0_Handler ()
{
	u8			byCSR0;

	/* Read CSR0 */
	jz_writeb(USB_REG_INDEX, 0);
	byCSR0 = jz_readb(USB_REG_CSR0);

	/* Check for SentStall 
	if sendtall is set ,clear the sendstall bit*/
	if (byCSR0 & USB_CSR0_SENTSTALL) 
	{
		jz_writeb(USB_REG_CSR0, (byCSR0 & ~USB_CSR0_SENDSTALL));
		ep0state = USB_EP0_IDLE;
		dprintf("\nSentstall!");
		return;
	}

	/* Check for SetupEnd */
	if (byCSR0 & USB_CSR0_SETUPEND) 
	{
		jz_writeb(USB_REG_CSR0, (byCSR0 | USB_CSR0_SVDSETUPEND));
		ep0state = USB_EP0_IDLE;
		dprintf("\nSetupend!");
		return;
	}
	/* Call relevant routines for endpoint 0 state */
	if (ep0state == USB_EP0_IDLE) 
	{
		if (byCSR0 & USB_CSR0_OUTPKTRDY)   //There are datas in fifo
		{
			USB_DeviceRequest *dreq;
			fifo=fifoaddr[0];
			udcReadFifo((u8 *)rx_buf, sizeof(USB_DeviceRequest));
			usb_setb(USB_REG_CSR0, 0x40);//clear OUTRD bit
			dreq = (USB_DeviceRequest *)rx_buf;
			dprintf("\nbmRequestType:%02x\nbRequest:%02x\n"
				"wValue:%04x\nwIndex:%04x\n"
				"wLength:%04x\n",
				dreq->bmRequestType,
				dreq->bRequest,
				dreq->wValue,
				dreq->wIndex,
				dreq->wLength);
			usbHandleDevReq(rx_buf);
		} else 
		{
			dprintf("0:R DATA\n");
		}
		rx_size = 0;
	}

	if (ep0state == USB_EP0_TX) 
	{
		fifo=fifoaddr[0];
		if (tx_size - finished <= 64) 
		{
			udcWriteFifo((u8 *)((u32)tx_buf+finished),
					 tx_size - finished);
			finished = tx_size;
			usb_setb(USB_REG_CSR0, USB_CSR0_INPKTRDY);
			usb_setb(USB_REG_CSR0, USB_CSR0_DATAEND); //Set dataend!
			ep0state=USB_EP0_IDLE;
		} else 
		{
			udcWriteFifo((u8 *)((u32)tx_buf+finished), 64);
			usb_setb(USB_REG_CSR0, USB_CSR0_INPKTRDY);
			finished += 64;
		}
	}
	return;
}

void EPIN_Handler(u8 EP)
{
	jz_writeb(USB_REG_INDEX, EP);
	fifo = fifoaddr[EP];

	if (tx_size-finished==0) 
	{
		usb_clearw(USB_REG_INTRINE, 2); // disable ep1 in interrupt		
		in_mask = 1;
		USB_HandleUFICmd();
		return;
	}

	if (tx_size - finished <= fifosize[EP]) 
	{
		udcWriteFifo((u8 *)((u32)tx_buf+finished),
			     tx_size - finished);
		usb_setw(USB_REG_INCSR, USB_INCSR_INPKTRDY);
		finished = tx_size;
		
		
	} else 
	{
		udcWriteFifo((u8 *)((u32)tx_buf+finished),
			    fifosize[EP]);
		usb_setw(USB_REG_INCSR, USB_INCSR_INPKTRDY);
		finished += fifosize[EP];
	}
}

void EPOUT_Handler(u8 EP)
{
	u32 size;
	
	jz_writeb(USB_REG_INDEX, EP);
	size = jz_readw(USB_REG_OUTCOUNT);
	//printf("\n size :%d",size);
	if(size > 0)
	{
		
		fifo = fifoaddr[EP];
		
		udcReadFifo((u8 *)((u32)rx_buf+rx_size), size);
		usb_clearw(USB_REG_OUTCSR,USB_OUTCSR_OUTPKTRDY);
		//printf("out intr = %x\n",jz_readw(USB_REG_INTROUT));
		
		rx_size += size;
		//printf("\n size :%d %x\n",rx_size,fifo);
		USB_HandleUFICmd();
		dprintf("\nEPOUT_handle return!");
	}
	
}

void udc4740Proc ()
{
	u8	IntrUSB;
	u16	IntrIn;
	u16	IntrOut;
	IntrOut = jz_readw(USB_REG_INTROUT);		
	IntrIn  = jz_readw(USB_REG_INTRIN);	
/* Read interrupt registers */
	if(in_mask == 0)
	{
		
	
	/* Check for resume from suspend mode */
		if (IntrIn & 2) 
		{
			
			EPIN_Handler(1);
			//printf("\nUDC EP1 IN operation!");
			//return;
			
		}
	}	
	if(out_mask == 0)
	{	
		if (IntrOut & 2) 
		{
			
			EPOUT_Handler(1);
			//printf("\nUDC EP1 OUT operation!");
			extern u32 massStat;
			
			//printf("IntrIn = %x,IntrOut = %x massStat = %d\r\n",IntrIn,IntrOut,massStat);	
			 //return;
			
		}
	} 
	
	
	IntrUSB = jz_readb(USB_REG_INTRUSB);
	if (IntrUSB & USB_INTR_RESET) 
	{
		dprintf("\nUDC reset intrupt!");  
		udc_reset();
	}
	if(IntrUSB & USB_INTR_SUSPEND)
	{
		dprintf("\nUDC Suspend interrupt!");
		
	}
	

/* Check for endpoint 0 interrupt */
	if (IntrIn & USB_INTR_EP0) 
	{
		dprintf("\nUDC EP0 operations!");
		EP0_Handler();
	}
	/* Check for suspend mode */
/*Implement late!!*/
	//IntrIn  = jz_readw(USB_REG_INTRIN);

	return;
}

static void udcIntrHandler(unsigned int arg)
{
	u8 err;
	__intc_mask_irq(IRQ_UDC);
	__gpio_mask_irq(IRQ_GPIO_UDC_DETE);
	OSSemPost(udcEvent);
}

void GPIO_Handle(unsigned int arg)
{
	printf("\n INTR! is_connect: %d",is_connect);
	printf("\n GPIO IRQ!!");
	if (is_connect)     //has connected!
	{
		udelay(200);
		if (__gpio_get_pin(GPIO_UDC_DETE)==0)
		{
			printf("\n UDC_DISCONNECT!");
			is_connect=0;
			//disable udc phy!
			__gpio_as_irq_rise_edge(GPIO_UDC_DETE);
			REG_CPM_SCR &= ~CPM_SCR_USBPHY_ENABLE;
			jz_writeb(USB_REG_POWER,0x40);      //disable sofeconnet!
		}
		else printf("\n UDC FALSE DISCONNECT!");
		
	}
	else                //has not connected!
	{
		udelay(200);
		if (__gpio_get_pin(GPIO_UDC_DETE)==1)
		{
			printf("\n UDC_CONNECT!");
			is_connect=1;
			//enable udc phy!
			__gpio_as_irq_fall_edge(GPIO_UDC_DETE);
			REG_CPM_SCR |= CPM_SCR_USBPHY_ENABLE;
			jz_writeb(USB_REG_POWER,0x60);     //enable sofeconnect
		}
		else printf("\n UDC FALSE CONNECT!");
	}

}

void GPIO_IRQ_init()
{
	int err=0;
	__gpio_as_irq_rise_edge(GPIO_UDC_DETE);
	request_irq(IRQ_GPIO_UDC_DETE, GPIO_Handle, 0);
	REG_CPM_SCR &= ~CPM_SCR_USBPHY_ENABLE;  //disable UDC_PHY
	jz_writeb(USB_REG_POWER,0x40);   //High speed	
}


static void udcTaskEntry(void *arg)
{
	u8 err;

	dprintf("Init UDC\n");
	USB_Version=USB_HS;
	is_connect=0;             //has not connected!
	udcEvent = OSSemCreate(0);
	request_irq(IRQ_UDC, udcIntrHandler, 0);
	GPIO_IRQ_init();
	udc_reset();
	dprintf("\nUDC reset!!");
	__intc_unmask_irq(IRQ_UDC);
	__gpio_unmask_irq(GPIO_UDC_DETE);
	__gpio_disable_pull(102);
	while (1) {
		OSSemPend(udcEvent, 0, &err);
		udc4740Proc();
		__intc_unmask_irq(IRQ_UDC);
		__gpio_unmask_irq(GPIO_UDC_DETE);
	}
}

#define UDC_TASK_STK_SIZE	1024 * 5
static OS_STK udcTaskStack[UDC_TASK_STK_SIZE];

void udc_init(void)
{
	printf("\nCreate UDC task!");
	OSTaskCreate(udcTaskEntry, (void *)0,
		     (void *)&udcTaskStack[UDC_TASK_STK_SIZE - 1],
		     UDC_TASK_PRIO);
}

