FS_FILE *myfile;
unsigned char mybuffer[0x100];
unsigned char bmpbuffer[0x100000];

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
  // printf("size of file:%d----",myfile->size);
  if (myfile) {
    /* read until EOF has been reached */
    do {
      x = FS_FRead(bmpbuffer,1,sizeof(bmpbuffer)-1,myfile);
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


void bmptest(void) {
	printf("bmptest!\n");
}
