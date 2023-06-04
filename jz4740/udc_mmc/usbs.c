#include "usb.h"

#define dprintf(x...)
extern u8 ep0state;

static const USB_DeviceRequest UDReq[] = {		//for what???
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

void usbEncodeDevReq(u8 *buf, int index)		//for what??
{
	memcpy(buf, &UDReq[index], sizeof(USB_DeviceRequest));
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
		ep0state=2;
		break;
	case 2: /* Vendor request */
		break;
	}
}

