#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <app_cfg.h>
#include <includes.h>
#include "fs_api.h"

#define  USE_YUV            1
//#define  USE_RGB            1
#define  QUAL_DEFAULT      80
/*
 * Select one of the sensors
 */
#define CONFIG_OV7660       1
//#define CONFIG_KSMOV7649  1
//#define CONFIG_HV7131     1 
/*
 * Define the image size
 */
#if CONFIG_OV7660 
#define IMG_WIDTH           640
#define IMG_HEIGHT          480
#define IMG_BPP             16
#endif
/*
 * CONFIG_KSMOV7649
 */
#if CONFIG_KSMOV7649
#define IMG_WIDTH           640
#define IMG_HEIGHT          480
#define IMG_BPP             16
#endif

/*
 * Common interface
 */
extern void camera_open(void);
extern void camera_close(void);
extern void camera_read(unsigned char *, int);
extern void save_frame(char *filename, void *buf, int size);
extern void read_frame(char *filename, void *buf, int size);
extern void rgb_convert(int o_xres, int o_yres, int n_xres, int n_yres, int start_x, int start_y, unsigned char *o_buf, unsigned char *n_buf);
//extern void delay_ms(unsigned int);
//extern void delay_us(unsigned int);

/*
 * CIM interface
 */
//extern void cim_open(void);
extern int cim_init(void);
extern void cim_close(void);
extern int cim_read(unsigned char *);

/*
 * Sensor interface
 */
extern unsigned char sensor_read_reg(unsigned char reg);
extern void sensor_write_reg(unsigned char reg, unsigned char val);

extern void init_ov7660(void);
extern void init_hv7131(void);
extern void init_ksmov7649(void);

/*
 * Application function
 */
extern void convert_yuv422_yuv444(unsigned char *buf, unsigned char *dst, int width, int height);
extern void put_image_jpeg (unsigned char *image, int width, int height, int quality, char *filename);
extern void putjpg(void  *filename);
extern void docapture(void);

/*
 * Application variable
 */
extern unsigned char camera_heap[];

extern int Jpeg_Err_Flag;
extern int Jpeg_Exp_Flag;
extern int Jpeg_Enter;
extern FS_FILE *Jpg_File;
extern struct jpeg_compress_struct cjpeg;
#endif /* __CAMERA_H__ */
