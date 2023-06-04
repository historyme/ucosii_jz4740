/*
 * jmemansi.c
 *
 * Copyright (C) 1992-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides a simple generic implementation of the system-
 * dependent portion of the JPEG memory manager.  This implementation
 * assumes that you have the ANSI-standard library routine tmpfile().
 * Also, the problem of determining the amount of memory available
 * is shoved onto the user.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		/* import the system-dependent declarations */
#include "includes.h"

#ifndef SEEK_SET		/* pre-ANSI systems may not define this; */
#define SEEK_SET  0		/* if not, assume 0 is correct */
#endif

#if 0          //use the memory management of uCOSii 
/* number of bytes per allocation */
#define HEAPBLKSIZE  0x8000
/* number of allocations available */
#define HEAPBLKS    13
/* our heap */
OS_MEM *heap;
unsigned char heapmem[HEAPBLKS * HEAPBLKSIZE];
void *malloc ( size_t size )
{
   INT8U err = OS_NO_ERR;
   void *alloc = 0;
   static int n=1;
   //   printf("%d malloc size=%ld\n",n++,size);
  /* initialize, if necessary */
   //  OS_ENTER_CRITICAL();
   if( !heap )
      heap = OSMemCreate( heapmem, HEAPBLKS,
                          HEAPBLKSIZE, &err );
   //  OS_EXIT_CRITICAL();
   if( heap && err == OS_NO_ERR ) {
      /* if the request fits the heap block length,
         then make the allocation from the heap */
      if( size <= HEAPBLKSIZE )
         alloc = OSMemGet( heap, &err );
      /* otherwise, we're sunk */
      else err = OS_MEM_NO_FREE_BLKS;
   }
  /* deny the allocation on errors */
   if( err != OS_NO_ERR )
      alloc = 0;
   return alloc;
}

/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  //  free(object);
}


/*
 * "Large" objects are treated the same as "small" ones.
 * NB: although we include FAR keywords in the routine declarations,
 * this file won't actually work in 80x86 small/medium model; at least,
 * you probably won't be able to process useful-size images in only 64KB.
 */

GLOBAL(void FAR *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void FAR *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
  // free(object);
}


#else 

/* number of bytes per allocation */
#define HEAPSIZE  0x15000
/* our heap */
static unsigned char heapmem[HEAPSIZE];
static void *alloc = heapmem;
void *malloc ( size_t size )
{
  alloc  += size;
#if 0
 {
   static int n=1, t=0;
   t +=size;
   printf("%d malloc size=%ld total=0x%lx\n",n++,size,t);
   printf("alloc = %p\n", alloc - size); 
 }
#endif

  if ((unsigned int)alloc > (unsigned int)&heapmem[HEAPSIZE])
    return 0;
  return (void *) (alloc - size);
 }

void free ( size_t size )
{
  alloc -= size;
}


/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  free(sizeofobject);
}


/*
 * "Large" objects are treated the same as "small" ones.
 * NB: although we include FAR keywords in the routine declarations,
 * this file won't actually work in 80x86 small/medium model; at least,
 * you probably won't be able to process useful-size images in only 64KB.
 */

GLOBAL(void *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
  return  malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void  * object, size_t sizeofobject)
{
  free(sizeofobject);
}

#endif
/*
 * This routine computes the total memory space available for allocation.
 * It's impossible to do this in a portable way; our current solution is
 * to make the user tell us (with a default value set at compile time).
 * If you can actually get the available space, it's a good idea to subtract
 * a slop factor of 5% or so.
 */

#ifndef DEFAULT_MAX_MEM		/* so can override from makefile */
#define DEFAULT_MAX_MEM		1000000L /* default: one megabyte */
#endif

GLOBAL(long)
jpeg_mem_available (j_common_ptr cinfo, long min_bytes_needed,
		    long max_bytes_needed, long already_allocated)
{
  return cinfo->mem->max_memory_to_use - already_allocated;
}


/*
 * Backing store (temporary file) management.
 * Backing store objects are only used when the value returned by
 * jpeg_mem_available is less than the total space needed.  You can dispense
 * with these routines if you have plenty of virtual memory; see jmemnobs.c.
 */


METHODDEF(void)
read_backing_store (j_common_ptr cinfo, backing_store_ptr info,
		    void FAR * buffer_address,
		    long file_offset, long byte_count)
{
  if (FS_FSeek(info->temp_file, file_offset, FS_SEEK_SET))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  if (JFREAD(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(cinfo, JERR_TFILE_READ);
}


METHODDEF(void)
write_backing_store (j_common_ptr cinfo, backing_store_ptr info,
		     void FAR * buffer_address,
		     long file_offset, long byte_count)
{
  if (FS_FSeek(info->temp_file, file_offset, FS_SEEK_SET))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  if (JFWRITE(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(cinfo, JERR_TFILE_WRITE);
}


METHODDEF(void)
close_backing_store (j_common_ptr cinfo, backing_store_ptr info)
{

  FS_FClose(info->temp_file);
  FS_Remove(info->temp_name);	/* delete the file */
 
}


/*
 * Initial opening of a backing-store object.
 *
 * This version uses tmpfile(), which constructs a suitable file name
 * behind the scenes.  We don't have to use info->temp_name[] at all;
 * indeed, we can't even find out the actual name of the temp file.
 */

GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
			 long total_bytes_needed)
{
  strcpy(info->temp_name,"jpegtmp");
  if((info->temp_file = FS_FOpen(info->temp_name, "w+b")) == NULL)
    {
      printf("File %s not found! \n",info->temp_name);
      ERREXITS(cinfo, JERR_TFILE_CREATE, info->temp_name);
    }
 
  info->read_backing_store = read_backing_store;
  info->write_backing_store = write_backing_store;
  info->close_backing_store = close_backing_store;
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.
 */

GLOBAL(long)
jpeg_mem_init (j_common_ptr cinfo)
{
  return DEFAULT_MAX_MEM;	/* default for max_memory_to_use */
}

GLOBAL(void)
jpeg_mem_term (j_common_ptr cinfo)
{
  /* no work */
}
