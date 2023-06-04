#include <includes.h>
#include <jz4740.h>
#include "usb.h"
#include "udc.h"
#include "mmc_api.h"
#define dprintf(x...) 
//printf(x)

static u32 rx_buf[1024];
static u32 tx_buf[1024];
static u32 tx_size, rx_size, finished;
static u32 fifo, curep;
static OS_EVENT *udcEvent;
static OS_EVENT *DMA_Finish;
u8 is_connect;
u8 ep0state;
u8 USB_Version;
u8 err;
u16  IntrOutMask;
//u16 MAX_EP1_SIZE;

void (*tx_done)(void) = NULL;

u32 hwRxFifoCount(void) { return rx_size; }
u32 hwTxFifoCount(void) { return tx_size - finished; }

#define UDC_TASK_PRIO	5

// UDC FIFO
#define TXFIFOEP0 USB_FIFO_EP0

static u32 fifoaddr[] = 
{
	TXFIFOEP0, TXFIFOEP0+4 ,TXFIFOEP0+8
};

static u32 fifosize[] = {
	MAX_EP0_SIZE, MAX_EP1_SIZE
};


int Enable_DMA_Read(u32 start_addr,u32 length)
{
	start_addr= start_addr & 0x7fffffff;
	dprintf("\n Enable DMA read! %d",length);
	jz_readb(USB_REG_INTRUSB);
	jz_readw(USB_REG_INTRIN);
	jz_readw(USB_REG_INTROUT);

	jz_writeb(USB_REG_INDEX,1);
	//__dcache_writeback_all();	
	IntrOutMask = 0x0;
	usb_setw(USB_REG_OUTCSR,0xa000);
	jz_writew(USB_REG_INTROUTE,0x0);   //disable OUT intr
	jz_writel(USB_REG_ADDR2,start_addr);
	jz_writel(USB_REG_COUNT2,length);
	jz_writel(USB_REG_CNTL2,0x001d);
	__intc_unmask_irq(IRQ_UDC);
	OSSemPend(DMA_Finish, 1000, &err);    //wait for DMA finish!
	if (err==OS_TIMEOUT) 
	{
		dprintf("\n READ TIMEOUT!");
		return 0;
	}
	//__dcache_invalidate_all();	
	return 1;
}

void Disable_DMA_Read()
{
	u16 temp;
	dprintf("\n Disable DMA read!");
	jz_writeb(USB_REG_INDEX,1);
	usb_clearw(USB_REG_OUTCSR,0xa000);
	jz_writew(USB_REG_INTROUTE,0x2);  //Enable OUT intr
	IntrOutMask = 0x2;
	OSSemPost(DMA_Finish);
}

int Enable_DMA_Write(u32 start_addr,u32 length)
{
	dprintf("\n Enable DMA write! %d",length);
	__dcache_writeback_all();
	jz_readb(USB_REG_INTRUSB);
	jz_readw(USB_REG_INTRIN);
	jz_readw(USB_REG_INTROUT);

	jz_writeb(USB_REG_INDEX,1);
	usb_setw(USB_REG_INCSR,0x9400);
	jz_writel(USB_REG_ADDR1,(u8 *)(start_addr & 0x7fffffff));
	jz_writel(USB_REG_COUNT1,length);//fifosize[ep]);
	jz_writel(USB_REG_CNTL1,0x001f);
	finished = length - (length % fifosize[1]);
	__intc_unmask_irq(IRQ_UDC);
	OSSemPend(DMA_Finish, 1000, &err);
	if (err==OS_TIMEOUT) 
	{
		dprintf("\n TIMEOUT!");
		return 0;
	}

	dprintf("\n addr %x,count %x,cntl %x",
		jz_readl(USB_REG_ADDR1),jz_readl(USB_REG_COUNT1),jz_readl(USB_REG_CNTL1));
	
	if( tx_size-finished <fifosize[1] && tx_size != finished)
	{
		finished = tx_size;
		jz_writeb(USB_REG_INDEX, 1);
		usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
	}
	return 1;
}

void Disable_DMA_Write()
{
	dprintf("\n Disable DMA write!");
	jz_writeb(USB_REG_INDEX,1);
	jz_writel(USB_REG_CNTL1,0x001e);
	usb_clearw(USB_REG_INCSR,0x9400);
	OSSemPost(DMA_Finish);
}

static void udc_reset(void)
{
	u8 byte;
	//data init
	ep0state = USB_EP0_IDLE;
	IntrOutMask = 0x2;
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
	jz_writeb(USB_REG_INDEX,1);
	jz_writew(USB_REG_OUTMAXP,512);
	jz_writew(USB_REG_INTRINE,0x3);   //enable intr
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
	}
  
	jz_writel(USB_REG_ADDR1,0);
	jz_writel(USB_REG_COUNT1,0);//fifosize[ep]);
	jz_writel(USB_REG_CNTL1,0);
	jz_writel(USB_REG_ADDR2,0);
	jz_writel(USB_REG_COUNT2,0);//fifosize[ep]);
	jz_writel(USB_REG_CNTL2,0);
	DMA_Finish = OSSemCreate(0);
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
	} 

}

void HW_SendZeroPKT(int ep)
{
	jz_writeb(USB_REG_INDEX, ep);
	REG32(fifo)=0;
	usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
	if (ep==0)usb_setb(USB_REG_CSR0, USB_CSR0_DATAEND);
}

void HW_SendPKT(int ep, const u8 *buf, int size)
{
	dprintf("EP%d send pkt :%d\n", ep, size);
	memcpy((void *)tx_buf, buf, size);
	fifo = fifoaddr[ep];
	tx_size = size;
	finished = 0;

	if (ep!=0)
	{
		jz_writeb(USB_REG_INDEX, ep);
		if (tx_size - finished <= fifosize[ep]) 
		{
			udcWriteFifo((u8 *)((u32)tx_buf+finished),
				     tx_size - finished);
			usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
			finished = tx_size;
		} else 
		{
			udcWriteFifo((u8 *)((u32)tx_buf+finished),
				     fifosize[ep]);
			usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
			finished += fifosize[ep];
		}
	}
}

void HW_GetPKT(int ep, const u8 *buf, int size)
{
	dprintf("EP%d read pkt :%d\n", ep, size);
	__dcache_writeback_all();
	__dcache_invalidate_all();
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
	u8	byCSR0;

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

	//printf("\n SIZE %d FINISH %d",tx_size,finished);
	if (tx_size-finished==0) 
	{
		USB_HandleUFICmd();
		return;
	}

	if (tx_size - finished <= fifosize[EP]) 
	{
		udcWriteFifo((u8 *)((u32)tx_buf+finished),
			     tx_size - finished);
		usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
		finished = tx_size;
	} else 
	{
		udcWriteFifo((u8 *)((u32)tx_buf+finished),
			    fifosize[EP]);
		usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
		finished += fifosize[EP];
	}
}

void EPOUT_Handler(u8 EP)
{
	u32 size;
	jz_writeb(USB_REG_INDEX, EP);
	size = jz_readw(USB_REG_OUTCOUNT);
	fifo = fifoaddr[EP];
	udcReadFifo((u8 *)((u32)rx_buf+rx_size), size);
	usb_clearb(USB_REG_OUTCSR,USB_OUTCSR_OUTPKTRDY);
	rx_size += size;
	USB_HandleUFICmd();
	dprintf("\nEPOUT_handle return!");
}

void udc4740Proc ()
{
	u8	IntrUSB;
	u16	IntrIn;
	u16	IntrOut;
	//u16     IntrDMA;
/* Read interrupt registers */
	IntrUSB = jz_readb(USB_REG_INTRUSB);
	IntrIn  = jz_readw(USB_REG_INTRIN);
	IntrOut = jz_readw(USB_REG_INTROUT);
	//IntrDMA = jz_readb(USB_REG_INTR);
/* Check for resume from suspend mode */
	dprintf("\nIntrUSB %x",IntrUSB);
	dprintf(" IntrIn %x",IntrIn);
	dprintf(" IntrOut %x",IntrOut);
	dprintf(" IntrDMA %x",IntrDMA);

	if ((IntrOut & IntrOutMask ) & 2) 
	{
		dprintf("\nUDC EP1 OUT operation!");
		EPOUT_Handler(1);
	} else 
	if (IntrIn & 2) 
	{
		dprintf("\nUDC EP1 IN operation!");
		EPIN_Handler(1);	     
	}

	if (IntrUSB & USB_INTR_RESET) 
	{
		dprintf("\nUDC reset intrupt!");  
		udc_reset();
	}

/* Check for endpoint 0 interrupt */
	if (IntrIn & USB_INTR_EP0) 
	{
		dprintf("\nUDC EP0 operations!");
		EP0_Handler();
	}

	/* Check for suspend mode */
        /*Implement late!!*/
	dprintf("\n UDCProc finish!");
	return;
}

void GPIO_Handle(unsigned int arg)
{
	dprintf("\n INTR! is_connect: %d",is_connect);
	if (is_connect)     //has connected!
	{
		udelay(200);
		if (__gpio_get_pin(GPIO_UDC_DETE)==0)
		{
			printf("\n UDC_DISCONNECT!");
			//if (MMC_Initialize()) printf("\n SD card init error!");
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
			//if (MMC_Initialize()) printf("\n SD card init error!");
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
	__gpio_disable_pull(102);
	/*__intc_unmask_irq(IRQ_GPIO_UDC_DETE);*/
	REG_CPM_SCR &= ~CPM_SCR_USBPHY_ENABLE;  //disable UDC_PHY
	jz_writeb(USB_REG_POWER,0x40);   //High speed	
}

static void udcIntrHandler(unsigned int arg)
{
	u8 err;
	u16     IntrDMA;
	__intc_mask_irq(IRQ_UDC);
	IntrDMA = jz_readb(USB_REG_INTR);
	if (IntrDMA & 0x1)     //channel 1 :IN
		Disable_DMA_Write();
	else 
	if (IntrDMA & 0x2)     //channel 2 :OUT
		Disable_DMA_Read();
	else	OSSemPost(udcEvent);
}

void TEST_GPIO()
{
	__gpio_as_input(96);
	//while(1)
	//if (__gpio_get_pin(96)==0) printf("\n 0");;
}

static void udcTaskEntry(void *arg)
{
	USB_Version=USB_HS;
	is_connect=0;
	udcEvent = OSSemCreate(0);
	request_irq(IRQ_UDC, udcIntrHandler, 0);
	GPIO_IRQ_init();
	udc_reset();
	TEST_GPIO();
	if (MMC_Initialize()) printf("\n SD card init error!");
	__intc_unmask_irq(IRQ_UDC);
	__gpio_unmask_irq(GPIO_UDC_DETE);
	while (1) {
		OSSemPend(udcEvent, 0, &err);
		//printf("\nKKKKooo");
		udc4740Proc();
		__intc_unmask_irq(IRQ_UDC);
		__gpio_unmask_irq(GPIO_UDC_DETE);
	}
}

#define UDC_TASK_STK_SIZE	1024 * 5
static OS_STK udcTaskStack[UDC_TASK_STK_SIZE];

void udc_init(void)
{
	printf("\nCreate UDC task!\n");
	OSTaskCreate(udcTaskEntry, (void *)0,
	     (void *)&udcTaskStack[UDC_TASK_STK_SIZE - 1],
	     UDC_TASK_PRIO);

	//while(1);
}

