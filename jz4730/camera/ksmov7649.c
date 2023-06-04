/*
 * KSMOV7649 CMOS camera sensor initialization
 */

#include "camera.h"

static void set_hv(int h, int v)
{
	sensor_write_reg(0x17, 0x1a);
	sensor_write_reg(0x19, 0x03);
	sensor_write_reg(0x18, 0x1a + h / 4);
	sensor_write_reg(0x1a, 0x03 + v / 2);
}

void init_ksmov7649(void)
{
	sensor_write_reg(0x12, 0x80);   // register reset
	udelay(5000);
	sensor_write_reg(0x12, 0x0c);   // AWB=1, RGB mode
	sensor_write_reg(0x11, 0xc0);   // HSYNC and VSYNC both POS
	sensor_write_reg(0x14, 0x04);   // res: VGA 640x480
	sensor_write_reg(0x15, 0x40);   // data on PCLK rising edge
	sensor_write_reg(0x1f, 0x11);   // RGB565
	sensor_write_reg(0x28, 0x20);   // progressive scan
	sensor_write_reg(0x71, 0x40);   // PCLK gated by HREF
	set_hv(IMG_WIDTH, IMG_HEIGHT);
	//printf("init ov7649 OK!\n");
	//printf("PID&version =%x %x\n",sensor_read_reg(0x0a),sensor_read_reg(0x0b)); 

}
