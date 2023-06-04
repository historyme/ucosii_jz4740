/*
 * Common CMOS camera sensor interface
 */
#include "camera.h"

#if (CONFIG_OV7660 || CONFIG_KSMOV7649)
#define DEV_ADDR 0x21
#endif
#if CONFIG_HV7131
#define DEV_ADDR 0x11
#endif

/* error code */
#define ETIMEDOUT	1
#define ENODEV		2

extern int i2c_read(unsigned char device, unsigned char *buf,
		    unsigned char offset, int count);

extern int i2c_write(unsigned char device, unsigned char *buf,
	      unsigned char offset, int count);

void sensor_write_reg(unsigned char reg, unsigned char val)
{ 
  if(-ENODEV == i2c_write(DEV_ADDR, &val, reg, 1)) {
    printf("No camera!\n");
    OSTaskDel(OS_PRIO_SELF);
  }
}

unsigned char sensor_read_reg(unsigned char reg)
{
  unsigned char buf;
  i2c_read(DEV_ADDR, &buf, reg, 1);
  return buf;
}
