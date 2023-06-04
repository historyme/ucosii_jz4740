#include <bsp.h>
#include <jz4740.h>
#include <key.h>
#include <mipsregs.h>
extern PFN_KEYHANDLE UpKey;
extern PFN_KEYHANDLE DownKey;

#define LED_PIN (32 * 3 + 28)
static PFN_KEYHANDLE oldPowerKeyHandle1 = 0;

void jz_control(u32 key)
{
	int i;
		if(key & 8)
	{
		serial_waitfinish();
		udelay(100);
		MP3Init();
	}
   
	if(key & 16)
	{
		serial_waitfinish();
		udelay(100);
		MP3Quit();
		
	}
	if(key & 32)
	{
		serial_waitfinish();
		udelay(100);
		pcm_pause();
	}

}

void ControlMp3(void)
{
	__gpio_as_output(LED_PIN);
	__gpio_clear_pin(LED_PIN);
	oldPowerKeyHandle1 = DownKey;
	DownKey = jz_control;
}

#if 0

FS_FILE *myfile;
unsigned char mybuffer[0x100];
unsigned char mp3buffer[10*1024*1024];


static void _error(const char *msg) {
#if (FS_OS_EMBOS)
  if (strlen(msg)) { 
    OS_SendString(msg);
  }
#else
  printf("%s",msg);
#endif
}
static void _dump_file(const char *name) {
  int x;

  /* open file */
	myfile = FS_FOpen(name,"r");
	if (myfile) {
		/* read until EOF has been reached */
		do {
			x = FS_FRead(mp3buffer,1,myfile->size,myfile);
			mybuffer[x]=0;
			if (x) {
				_log(mybuffer);
			}
		} while (x);
		/* check, if there is no more data, because of EOF */
		x = FS_FError(myfile);
		if (x!=FS_ERR_EOF) {
			/* there was a problem during read operation */
			sprintf(mybuffer,"Error %d during read operation.\n",x);
			_error(mybuffer);
		}
		/* close file */
		FS_FClose(myfile);
  }
	else {
		sprintf(mybuffer,"Unable to open file %s.\n",name);
		_error(mybuffer);
  }
}
extern int IS_WRITE_PCM;
void TestMp3_old(void)
{
	__gpio_as_output(LED_PIN);
	__gpio_clear_pin(LED_PIN);
	oldPowerKeyHandle1 = DownKey;
	DownKey = jz_control;
	
	printf("Test mp3!\n");
	FS_Init();
	_show_directory(""); 

	IS_WRITE_PCM=1;
	pcm_init();
//	minimad_main(myfile->size,mp3buffer);

//	mdelay(500);
//		_dump_file("shenhua.mp3");
//	minimad_main(myfile->size,mp3buffer);
//	_dump_file("aiwo.mp3");
//	minimad_main(myfile->size,mp3buffer);
//		mdelay(500);
//			_dump_file("aiwo.mp3");
//	minimad_main(myfile->size,mp3buffer);
//			mdelay(500);

//	_dump_file("perfect.mp3");
//	minimad_main(myfile->size,mp3buffer);
//				mdelay(500);

//	_dump_file("gsdwr.mp3");
//	minimad_main(myfile->size,mp3buffer);

//		mdelay(500);
//	_dump_file("nddas.mp3");
//	minimad_main(myfile->size,mp3buffer);
//		mdelay(500);
//	_dump_file("all_zero.mp3");
//	minimad_main(myfile->size,mp3buffer);
//		mdelay(500);
	
//	_dump_file("cangha~1.mp3");
//	minimad_main(myfile->size,mp3buffer);

//	_dump_file("nzjhhm.mp3");
//	minimad_main(myfile->size,mp3buffer);
//	_dump_file("caihong.mp3");
//	minimad_main(myfile->size,mp3buffer);
//	_dump_file("bywdsg.mp3");
//	minimad_main(myfile->size,mp3buffer);
	
	_dump_file("perfect.mp3");
	minimad_main(myfile->size,mp3buffer);
	_dump_file("perfect2.mp3");
	minimad_main(myfile->size,mp3buffer);
	_dump_file("vipcnc~1.mp3");
	minimad_main(myfile->size,mp3buffer);
}	 


#endif
