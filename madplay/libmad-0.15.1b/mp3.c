#include <bsp.h>
#include <jz4740.h>
#include <ucos_ii.h>
#include <fs_api.h>

#define MP3_TASK_STK_SIZE	1024*20
#define MP3_TASK_PRIO	12

static OS_STK MP3TaskStack[MP3_TASK_STK_SIZE];


FS_FILE *myfile;
static unsigned char mybuffer[0x100];
static unsigned char mp3buffer[12*1024*1024];

static void _log(const char *msg) {
  printf("%s",msg);
}
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
void TestMp3(void)
{
	printf("Install MP3!\n");
	FS_Init();

	IS_WRITE_PCM=1;
	pcm_init();
//	_dump_file("shenhua.mp3");
//	_dump_file("nndas.mp3");
//	_dump_file("tiankong.mp3");
//	minimad_main(myfile->size,mp3buffer,myfile);

//	_dump_file("nzjhhm.mp3");
//	minimad_main(myfile->size,mp3buffer);

	//_dump_file("gsdwr.mp3");
	minimad_main(myfile->size,mp3buffer);
//	_dump_file("perfect.mp3");
//	minimad_main(myfile->size,mp3buffer);
//	_dump_file("caihong.mp3");
//	minimad_main(myfile->size,mp3buffer);
//	_dump_file("bywdsg.mp3");
//	minimad_main(myfile->size,mp3buffer);
//	_dump_file("perfect.mp3");
//	minimad_main(myfile->size,mp3buffer);
//	_dump_file("tiankong.mp3");
//	minimad_main(myfile->size,mp3buffer,myfile);
//	_dump_file("perfect2.mp3");
//	minimad_main(myfile->size,mp3buffer);
//	_dump_file("vipcnc~1.mp3");
//	minimad_main(myfile->size,mp3buffer);
}
int EndMp3 = 0;
static OS_EVENT *mp3Event;
static void MP3TaskEntry(void *arg)
{
	unsigned char err;	
	void (*finish)(void);
	
	while(1)
	{
	    OSSemPend(mp3Event,0,&err);
	    TestMp3();
	    finish = EndMp3;
			if(finish != 0)
	   	{
				finish();
			}
	    
	}
		
}

int initMp3()
{
	  mp3Event = OSSemCreate(0);
		OSTaskCreate(MP3TaskEntry, 0,
		     (void *)&MP3TaskStack[MP3_TASK_STK_SIZE - 1],
		     MP3_TASK_PRIO);
		     
}
int MP3_Play(char *fname,char *buf,int handle)
{
	int d ;
  _dump_file(fname);
  EndMp3 = handle;
  d = GetMp3InfoEx(myfile->size,mp3buffer,buf);
  OSSemPost(mp3Event);
  return 0;	
}
void MP3Quit(){
      printf(" Quit MP3!\n");
      OSTaskDel(MP3_TASK_PRIO);
}
