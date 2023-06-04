#include <fs_api.h>

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


static void _log(const char *msg) {
#if (FS_OS_EMBOS)
  if (strlen(msg)) {
  OS_SendString(msg);
  }
#else
  printf("%s",msg);
#endif
}


static void _show_directory(const char *name) {
  FS_DIR *dirp;
  struct FS_DIRENT *direntp;

  _log("Directory of ");
  _log(name);
  _log("\n");
  dirp = FS_OpenDir(name);
  if (dirp) {
    do {
      direntp = FS_ReadDir(dirp);
      if (direntp) {
        sprintf(mybuffer,"%s\n",direntp->d_name);
        _log(mybuffer);
      }
    } while (direntp);
    FS_CloseDir(dirp);
  }
  else {
    _error("Unable to open directory\n");
  }
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
void TestMp3_jz4730(void)
{
	printf("Test mp3!\n");
	FS_Init();
	_show_directory(""); 
//        _dump_file("nt.mp3");
	_dump_file("lzhd.mp3");
	IS_WRITE_PCM=1;
	pcm_init();
	minimad_main(myfile->size,mp3buffer);
}	 
