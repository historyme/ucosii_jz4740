#include "usb.h"

#define dprintf(x...)

static const USB_DeviceRequest UDReq[] = {
	{}, // get_status
	{}, // clear_feature
	{}, // RFU
	{}, // set_feature
	{}, // RFU
	{}, // set_address
	{}, // get_descriptor
	{}, // set_descriptor
	{}, // get_configuration
	{0, SET_CONFIGURATION, 1, 0, 8},
	{}, // get_interface
	{0, SET_INTERFACE, 1, 0, 8},
	{}, // SOF
};

void usbEncodeDevReq(u8 *buf, int index)
{
	memcpy(buf, &UDReq[index], sizeof(USB_DeviceRequest));
}

void usbHandleStandDevReq(u8 *buf)
{
	USB_DeviceRequest *dreq = (USB_DeviceRequest *)buf;
	switch (dreq->bRequest) {
	case GET_DESCRIPTOR:
		if (dreq->bmRequestType == 0x80)	/* Dev2Host */
			switch(dreq->wValue >> 8) {
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
		break;
	case CLEAR_FEATURE:
	case SET_CONFIGURATION:
	case SET_INTERFACE:
	case SET_FEATURE:
#if 0
		printf("Send ZERO packet at 0.\n");
		HW_SendZeroPKT(0);
#endif
		break;
	}
}

void usbHandleDevReq(u8 *buf)
{
	dprintf("dev req:%d\n", (buf[0] & (3 << 5)) >> 5);
	switch ((buf[0] & (3 << 5)) >> 5) {
	case 0: /* Standard request */
		usbHandleStandDevReq(buf);
		break;
	case 1: /* Class request */
		usbHandleClassDevReq(buf);
		break;
	case 2: /* Vendor request */
		break;
	}
}

