/*
 * libid3tag - ID3 tag manipulation library
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
 * $Id: id3app.c,v 1.1.1.1 2007/11/21 09:59:32 jgao Exp $
 */
#if 0
# include "global.h"
# include "id3tag.h"


/*
 * NAME:	get_id3()
 * DESCRIPTION:	read and parse an ID3 tag from a stream
 */
static
struct id3_tag *get_id3(struct mad_stream *stream, id3_length_t tagsize,
			struct input *input)
{
  struct id3_tag *tag = 0;
  id3_length_t count;
  id3_byte_t const *data;
  id3_byte_t *allocated = 0;

  count = stream->bufend - stream->this_frame;

  if (tagsize <= count) {
    data = stream->this_frame;
    mad_stream_skip(stream, tagsize);
  }
  else {
    allocated = malloc(tagsize);
    if (allocated == 0) {
      error("id3", _("not enough memory to allocate tag data buffer"));
      goto fail;
    }

    memcpy(allocated, stream->this_frame, count);
    mad_stream_skip(stream, count);

    while (count < tagsize) {
      int len;

      do
	len = read(input->fd, allocated + count, tagsize - count);
      while (len == -1 && errno == EINTR);

      if (len == -1) {
	error("id3", ":read");
	goto fail;
      }

      if (len == 0) {
	error("id3", _("EOF while reading tag data"));
	goto fail;
      }

      count += len;
    }

    data = allocated;
  }

  tag = id3_tag_parse(data, tagsize);

 fail:
  if (allocated)
    free(allocated);

  return tag;
}

/*
 * NAME:	show_id3()
 * DESCRIPTION:	display ID3 tag information
 */
static
void show_id3(struct id3_tag const *tag)
{
  unsigned int i;
  struct id3_frame const *frame;
  id3_ucs4_t const *ucs4;
  id3_latin1_t *latin1;

  static struct {
    char const *id;
    char const *label;
  } const info[] = {
    { ID3_FRAME_TITLE,  N_("Title")     },
    { "TIT3",           0               },  /* Subtitle */
    { "TCOP",           0               },  /* Copyright */
    { "TPRO",           0               },  /* Produced */
    { "TCOM",           N_("Composer")  },
    { ID3_FRAME_ARTIST, N_("Artist")    },
    { "TPE2",           N_("Orchestra") },
    { "TPE3",           N_("Conductor") },
    { "TEXT",           N_("Lyricist")  },
    { ID3_FRAME_ALBUM,  N_("Album")     },
    { ID3_FRAME_TRACK,  N_("Track")     },
    { ID3_FRAME_YEAR,   N_("Year")      },
    { "TPUB",           N_("Publisher") },
    { ID3_FRAME_GENRE,  N_("Genre")     },
    { "TRSN",           N_("Station")   },
    { "TENC",           N_("Encoder")   }
  };

  /* text information */

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
	detail(gettext(info[i].label), 0, latin1);
      else {
	if (strcmp(info[i].id, "TCOP") == 0 ||
	    strcmp(info[i].id, "TPRO") == 0) {
	  detail(0, "%s %s", (info[i].id[1] == 'C') ?
		 _("Copyright (C)") : _("Produced (P)"), latin1);
	}
	else
	  detail(0, 0, latin1);
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

    detail(_("Comment"), 0, latin1);

    free(latin1);
    break;
  }

  if (0) {
  fail:
    error("id3", _("not enough memory to display tag"));
  }
}
#endif
