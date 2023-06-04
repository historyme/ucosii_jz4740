/*
 * Camera utilities interface
 */

#include "camera.h"
#include "fs_api.h"

#ifndef NULL
#define NULL	0
#endif

void camera_open(void)
{
       cim_init();

#ifdef CONFIG_OV7660
	init_ov7660();
#endif
#ifdef CONFIG_KSMOV7649
	init_ksmov7649();
#endif
#ifdef CONFIG_HV7131
	init_hv7131();
	/* enable sensor*/
	unsigned int val ;
	val = sensor_read_reg( 0x02);
	val &= ~(0x08);
	sensor_write_reg(0x02, val);
#endif
}

void camera_close(void)
{

	/* disable sensor */
#ifdef CONFIG_HV7131
	unsigned int val;
	val = sensor_read_reg( 0x02);
	val |= (0x08);
	sensor_write_reg(0x02, val);
#endif /* CONFIG_HV7131 */

	cim_close();
}


void camera_read(unsigned char *buf, int size)
{
 	cim_read(buf);
}

/*
 * Common utils
 */

void save_frame(char *filename, void *buf, int size)
{
  	FS_FILE *ffd;
	int count;

	if ((ffd = FS_FOpen(filename,"wb")) == NULL) {
		printf("Open file %s failed\n", filename);
	}

	if ((count = FS_FWrite(buf, 1, size, ffd)) != size) {
		printf("Error when write to %s.\n", filename);
	}

	FS_FClose(ffd);
}

void read_frame(char *filename, void *buf, int size)
{
	FS_FILE *fd;
	int count;

	if ((fd = FS_FOpen(filename, "rb")) == NULL) {
		printf("Failed to open file %s\n", filename);
	}
	
	if ((count = FS_FRead(buf, 1, size, fd)) != size) {
		printf("Error when read %s. %s\n", filename);
	}

	FS_FClose(fd);
}

#define NULLRGB565 0x0000
void rgb_convert(int o_xres, int o_yres, int n_xres, int n_yres, int start_x, int start_y, unsigned char *o_buf, unsigned char *n_buf)
{
	int x, y;
	unsigned short *o_ptr = (unsigned short *)o_buf;
	unsigned short *n_ptr = (unsigned short *)n_buf;

	for (y = 0; y < start_y; y++) {
		for (x = 0; x < n_xres; x++)
			*n_ptr++ = NULLRGB565;
	}
	for (y = start_y; y < start_y + o_yres; y++) {
		for (x = 0; x < start_x; x++)
			*n_ptr++ = NULLRGB565;
		for (x = start_x; x < start_x + o_xres; x++)
			*n_ptr++ = *o_ptr++;
		for (x = start_x + o_xres; x < n_xres; x++)
			*n_ptr++ = NULLRGB565;
	}
	for (y = start_y + o_yres; y < n_yres; y++) {
		for (x = 0; x < n_xres; x++)
			*n_ptr++ = NULLRGB565;
	}
}
