/*
 * Capture images
 */

#include <stdio.h>
#include "camera.h"
#include "jpeglib.h"
#include "includes.h"


void convert_yuv422_yuv444(unsigned char *buf, unsigned char *dst, int width, int height)
{
  /* UYVY -> YUVYUV */

#define Y(x) dst[x*3 + 0]
#define U(x) dst[x*3 + 1]
#define V(x) dst[x*3 + 2]

#define Y0(x) buf[x*2 + 1]
#define U0(x) buf[x*2 + 0]
#define V0(x) buf[x*2 + 2]
#define Y1(x) buf[x*2 + 1]
#define U1(x) buf[x*2 - 2]
#define V1(x) buf[x*2 + 0]

  int i;
  for(i=0;i<width*height;i++) {
    if (i%2 == 0) {  
      Y(i) = Y0(i);
      U(i) = U0(i);
      V(i) = V0(i);
    } else {
      Y(i) = Y1(i);
      U(i) = U1(i);
      V(i) = V1(i);
    }
  }
}

void convert_rgb565_rgb888(unsigned char *src, unsigned char *dst, int width, int height)
{
	int x;
	int y;
	int z;
	int line_width;
	int line_width1;

	line_width = width * 2;
	line_width1 = width * 3;
 	
	for ( z = 0; z < height; z++)
	  {
	    for (x = 0, y=0; x < width * 2 || y < width * 3; x += 2,y += 3)
	      {	
#if 0
		dst[y]   = (src[x] & 0x1f) << 3;
		dst[y+1] = ((src[x] & 0xe0) >> 3) | ((src[x+1] & 0x7) << 5);
		dst[y+2] = src[x+1] & 0xf8;
#else  /* for  row_ptr[0] = & image[cjpeg.next_scanline * line_width]; */
		dst[y+2]   = (src[x] & 0x1f) << 3;
		dst[y+1] = ((src[x] & 0xe0) >> 3) | ((src[x+1] & 0x7) << 5);
		dst[y] = src[x+1] & 0xf8;
#endif
	      }

	  src += line_width;
	  dst += line_width1;
	  }
 }

/* Global varibles defined for handling jpeg error and exceptions, used in excpt.c and jerror.c  */
int      Jpeg_Err_Flag = 0;
int      Jpeg_Exp_Flag = 0;
int      Jpeg_Enter    = 0;
FS_FILE *Jpg_File;
struct jpeg_compress_struct cjpeg;

/*
* Description: Compress YUV or RGB image to jpeg file
*
* Arguments  : image     is a pointer to YUV444 or RGB888 data array
*              width     is the width of the image in pixel
*              height    is the height of the image in pixel
*              quality   is the quality of the jpeg file
*              filename  is the name of the jpeg file
*
* Returns    : None
*/

void
put_image_jpeg (unsigned char *image, int width, int height, int quality, char *filename)
{
	int y, x, line_width;
	JSAMPROW row_ptr[1];

	struct jpeg_error_mgr jerr;
	unsigned char *line;
        unsigned char line0[IMG_WIDTH * 3];//


	line = line0;
	cjpeg.err = jpeg_std_error(&jerr);
	jpeg_create_compress (&cjpeg);

	if((Jpg_File = FS_FOpen(filename,"wb")) == NULL)
	  {
	    int x;
	    x = FS_FError(Jpg_File);
	    printf("Can't open file %s, because of error %d.\n", filename, x);
	    OSTaskDel(OS_PRIO_SELF);
	  }
	jpeg_stdio_dest (&cjpeg, Jpg_File);//

	cjpeg.image_width = width;
	cjpeg.image_height= height;
	cjpeg.input_components = 3;

#ifdef USE_YUV
	cjpeg.in_color_space = JCS_YCbCr;
#else
	cjpeg.in_color_space = JCS_RGB;
#endif
	jpeg_set_defaults (&cjpeg);
	jpeg_set_quality (&cjpeg, quality, TRUE);

	cjpeg.dct_method = JDCT_FASTEST;
	//cjpeg.dct_method = JDCT_ISLOW;
	jpeg_start_compress (&cjpeg, TRUE);
	row_ptr[0] = line;

	line_width = width * 3;
	while (cjpeg.next_scanline < cjpeg.image_height) {
	  row_ptr[0] = & image[cjpeg.next_scanline * line_width];
	  jpeg_write_scanlines(&cjpeg, row_ptr, 1);
	}

	jpeg_finish_compress (&cjpeg);
	jpeg_destroy_compress (&cjpeg);
	FS_FClose(Jpg_File);
}

/* memory for framebuf and dstbuf*/
unsigned char camera_heap[IMG_WIDTH * IMG_HEIGHT * 5];

/*
* Arguments :  filename    is the name of jpeg file 
*
* Returns   :  none
*/

void putjpg(void  *filename)
{
	int framesize;
	unsigned char *framebuf;
	unsigned char *dstbuf;
	int framesize_dst;
	int i;

	framesize = IMG_WIDTH * IMG_HEIGHT * (IMG_BPP/8);
	framesize_dst = IMG_WIDTH * IMG_HEIGHT *3;

	/* alloc buffer */
	framebuf = (unsigned char *)camera_heap + IMG_WIDTH * IMG_HEIGHT +2; 
	dstbuf = (unsigned char *)camera_heap;

	//camera_open(); /* open devices */

	i=1;
	while( i-- )
	  cim_read(framebuf);

#ifdef USE_YUV 
	convert_yuv422_yuv444(framebuf,dstbuf,IMG_WIDTH,IMG_HEIGHT);
#else 
	convert_rgb565_rgb888(framebuf,dstbuf,IMG_WIDTH,IMG_HEIGHT);
#endif

	put_image_jpeg (dstbuf,IMG_WIDTH,IMG_HEIGHT, QUAL_DEFAULT, (char *)filename);

	//camera_close(); /* close devices */

}

/* display raw RGB images captured by camera in LCD dynamically */
static U8 * lcd_fb = NULL;
void docapture(void)
{
  lcd_fb = lcd_get_cframe();
  cim_read(lcd_fb);
}
