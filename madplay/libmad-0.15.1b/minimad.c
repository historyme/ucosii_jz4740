/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: minimad.c,v 1.4 2007/11/23 09:38:21 dsqiu Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include "mad.h"
#include <ucos_ii.h>
#include "pcm.h"

#include <fs_api.h>
#include "global.h"
#include "id3tag.h"
#include "genre.dat"
extern int pcm_ioctl(unsigned int cmd, unsigned long arg);
#define PAGE_SIZE 1024
/*
 * This is perhaps the simplest example use of the MAD high-level API.
 * Standard input is mapped into memory via mmap(), then the high-level API
 * is invoked with three callbacks: input, output, and error. The output
 * callback converts MAD's high-resolution PCM samples to 16 bits, then
 * writes them to standard output in little-endian, stereo-interleaved
 * format.
 */

static int decode(unsigned char const *, unsigned long);
int GetMp3Info(unsigned int size,char *buff,char *info)
{
  id3_byte_t query[ID3_TAG_QUERYSIZE];
	signed long size_tt;
	id3_byte_t *data;
	struct id3_tag *tag = 0;

	memcpy(query,buff,sizeof(query));
	size_tt = id3_tag_query(query, sizeof(query));
//	printf("size:%d\n",size_tt);
	if(size_tt>0)
	{
		
		data = (id3_byte_t *)malloc(size_tt);
		memcpy(data,buff,size_tt);
		tag = id3_tag_parse(data, size_tt);
		free(data);
		return show_id3(tag,info);
	}
  return 0;
}
struct id3tag1 {
	char tag2[3];
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	char comment[30];
	unsigned char genre;
};
int GetMp3InfoEx(unsigned int size,char *buff,char *info)
{
		memcpy(info,(buff+size-128),128);
 		return 128;
}
int MP3_PrintTAG(unsigned int size, char *buff){

        struct id3tag1 tag1;
        char title[31]={0,};
        char artist[31]={0,};
        char album[31]={0,};
        char year[5]={0,};
        char comment[31]={0,};
        char genre[31]={0,};
  int fsize;
  int ret;

  memcpy(&tag1,(buff+size-128),128);
 
  if( tag1.tag2[0]!='T' || tag1.tag2[1]!='A' || tag1.tag2[2]!='G') return size;
        strncpy(title,tag1.title,30);
        strncpy(artist,tag1.artist,30);
        strncpy(album,tag1.album,30);
        strncpy(year,tag1.year,4);
        strncpy(comment,tag1.comment,30);

        if ( tag1.genre <= sizeof(genre_table)/sizeof(*genre_table) ) {
                strncpy(genre, genre_table[tag1.genre], 30);
        } else {
                strncpy(genre,"Unknown",30);
        }

//      printf("\n");
        printf("Title  : %30s  Artist: %s\n",title,artist);
        printf("Album  : %30s  Year  : %4s\n",album,year);
        printf("Comment: %30s  Genre : %s\n",comment,genre);
        printf("\n");
  return size-128;
}

int minimad_main(unsigned int size, char *buff)
{
	#if 0
	id3_byte_t query[ID3_TAG_QUERYSIZE];
	signed long size_tt;
	id3_byte_t *data;
	struct id3_tag *tag = 0;

	memcpy(query,buff,sizeof(query));
	size_tt = id3_tag_query(query, sizeof(query));
//	printf("size:%d\n",size_tt);
	if(size_tt>0)
	{
		
		data = (id3_byte_t *)malloc(size_tt);
		memcpy(data,buff,size_tt);
		tag = id3_tag_parse(data, size_tt);
		free(data);
		show_id3(tag);
	}

#endif
	
//	MP3_PrintTAG(size,buff);
	
	decode(buff,size);
	return 0;
}



/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

struct buffer {
  unsigned char const *start;
  unsigned long length;
};

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
	struct buffer *buffer = data;
	if (!buffer->length)
		return MAD_FLOW_STOP;
	mad_stream_buffer(stream, buffer->start, buffer->length);
	buffer->length = 0;
	return MAD_FLOW_CONTINUE;
}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline
signed int scale(mad_fixed_t sample)
{
  /* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;
	
	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */
unsigned int nsamples;

static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
//  unsigned int nchannels, nsamples;
  unsigned int nchannels;
  mad_fixed_t const *left_ch, *right_ch;
  short pcmbuf[4*PAGE_SIZE];
  int pcmcount = 0;

  /* pcm->samplerate contains the sampling frequency */
  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];
  pcmcount = 0;
  while (nsamples--) {
	  nsamples=get_sample();
	  pcmbuf[pcmcount++] = scale(*left_ch++);
	  pcmbuf[pcmcount++] = scale(*right_ch++);
  }
  pcm_write((char *)pcmbuf,2 * pcmcount);
  return MAD_FLOW_CONTINUE;
}

unsigned int get_sample()
{
	return nsamples;
	
}

		
/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct buffer *buffer = data;
//  printf("decoding error 0x%04x (%s) at byte offset %u\n",
//	  stream->error, mad_stream_errorstr(stream),
//	  stream->this_frame - buffer->start);

  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */
//  return MAD_FLOW_BREAK;
  return MAD_FLOW_CONTINUE;
}

/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */
  struct mad_decoder decoder;
static
int decode(unsigned char const *start, unsigned long length)
{
  struct buffer buffer;
//  struct mad_decoder decoder;
  int result;

  /* initialize our private message structure */

  buffer.start  = start;
  buffer.length = length;

  /* configure input, output, and error functions */

  mad_decoder_init(&decoder, &buffer,
		   input, 0 /* header */, 0 /* filter */, output,
		   error, 0 /* message */);

  /* start decoding */
  pcm_ioctl(PCM_SET_SAMPLE_RATE,44100);
  pcm_ioctl(PCM_SET_FORMAT,  AFMT_S16_LE);
  pcm_ioctl(PCM_SET_CHANNEL, 2);
  printf("MP3 playing start......\n");

  result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
  /* release the decoder */
  printf("MP3 playing end!\n");
  mad_decoder_finish(&decoder);

  return result;
}

int close_minimad()
{
//	  struct mad_stream *stream;
//  struct mad_frame *frame;
//  struct mad_synth *synth;
	  printf("MP3 playing end!\n");
	  nsamples=0;
//  stream = &decoder->sync->stream;
//  frame  = &decoder->sync->frame;
//  synth  = &decoder->sync->synth;

//	  mad_synth_finish(&decoder->sync->synth);
//	  mad_frame_finish(&decoder->sync->synth);
//	  mad_stream_finish(&decoder->sync->stream);
	  
	  mad_decoder_finish(&decoder);
	return 0;
}
/*
 * NAME:	get_id3()
 * DESCRIPTION:	read and parse an ID3 tag from a stream
 */
static
struct id3_tag *get_id3(unsigned char const *start, unsigned long length)
{
	struct id3_tag *tag = 0;
	id3_length_t count;
	id3_byte_t const *data;




	data = start;

	tag = id3_tag_parse(data, length);

	return tag;
}


/*
 * NAME:	show_id3()
 * DESCRIPTION:	display ID3 tag information
 */
static
int show_id3(struct id3_tag const *tag,char *arg)
{
	unsigned int i;
	struct id3_frame const *frame;
	id3_ucs4_t const *ucs4;
	id3_latin1_t *latin1;
  int len;
	static struct {
		char const *id;
		char const *label;
	} const info[] = {
		{ ID3_FRAME_TITLE,  "Title"     },
		{ "TIT3",           0               },  /* Subtitle */
		{ "TCOP",           0               },  /* Copyright */
		{ "TPRO",           0               },  /* Produced */
		{ "TCOM",           "Composer"  },
		{ ID3_FRAME_ARTIST, "Artist"    },
		{ "TPE2",           "Orchestra" },
		{ "TPE3",           "Conductor" },
		{ "TEXT",           "Lyricist"  },
		{ ID3_FRAME_ALBUM,  "Album"     },
		{ ID3_FRAME_TRACK,  "Track"     },
		{ ID3_FRAME_YEAR,   "Year"      },
		{ "TPUB",           "Publisher" },
		{ ID3_FRAME_GENRE,  "Genre"     },
		{ "TRSN",           "Station"   },
		{ "TENC",           "Encoder"   }
	};

	/* text information */
   arg[0] = 0;
   len = 0;
	for (i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
		union id3_field const *field;
		unsigned int nstrings, j;
		
		frame = id3_tag_findframe(tag, info[i].id, 0);
		if (frame == 0)
			continue;
		
		field    = id3_frame_field(frame, 1);
		nstrings = id3_field_getnstrings(field);
		
		for (j = 0; j < nstrings; ++j) {
			ucs4 = id3_field_getstrings(field, j);
			assert(ucs4);
			
			if (strcmp(info[i].id, ID3_FRAME_GENRE) == 0)
				ucs4 = id3_genre_name(ucs4);
			
			latin1 = id3_ucs4_latin1duplicate(ucs4);
			if (latin1 == 0)
				goto fail;
			
			if (j == 0 && info[i].label)
				{
					sprintf(&arg[len],"%s",info[i].label);
					printf("%s(%d)",&arg[len],len);
					len += strlen(&arg[len]);
					sprintf(&arg[len + 1],"%s",latin1);	
					
					printf("%s(%d)",&arg[len + 1],len);
					
					len += strlen(&arg[len + 1]);
				}
				//printf("%s:%s\n",info[i].label,latin1);
			
			else {
				if (strcmp(info[i].id, "TCOP") == 0 ||
					strcmp(info[i].id, "TPRO") == 0) {
					sprintf(&arg[len],"%s",(info[i].id[1] == 'C') ?
						   ("Copyright (C)") : ("Produced (P)"));
					len += strlen(&arg[len]);
					
					sprintf(&arg[len + 1],"%s",latin1);	
					len += strlen(&arg[len + 1]);
					//printf("%s- %s\n", (info[i].id[1] == 'C') ?
					//	   ("Copyright (C)") : ("Produced (P)"), latin1);
				}
				//else
				//	printf("else:%s", latin1);
			}
			
			free(latin1);
		}
	}

  /* comments */

	i = 0;
	while ((frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, i++))) {
		ucs4 = id3_field_getstring(id3_frame_field(frame, 2));
		assert(ucs4);
		
		if (*ucs4)
			continue;

		ucs4 = id3_field_getfullstring(id3_frame_field(frame, 3));
		assert(ucs4);
		
		latin1 = id3_ucs4_latin1duplicate(ucs4);
		if (latin1 == 0)
			goto fail;
		sprintf(&arg[len],"%s","Comment");
		len += strlen(&arg[len]);
		sprintf(&arg[len + 1],"%s",latin1);	
		len += strlen(&arg[len + 1]);
		free(latin1);
		break;
	}
  return len;
	if (0) {
	fail:
		printf("id3 _not enough memory to display tag\n");
	}
}

