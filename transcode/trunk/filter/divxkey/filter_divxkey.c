/*
 *  filter_divxkey.c
 *
 *  Copyright (C) Thomas Oestreich - December 2001
 *
 *  This file is part of transcode, a video stream processing tool
 *
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define MOD_NAME    "filter_divxkey.so"
#define MOD_VERSION "v0.1 (2002-01-15)"
#define MOD_CAP     "check for DivX 4.xx / OpenDivX / DivX;-) keyframe"
#define MOD_AUTHOR  "Thomas Oestreich"

#include "transcode.h"
#include "filter.h"
#include "libtc/optstr.h"

#include "import/magic.h"
#include "bitstream.h"

static char buffer[128];

static DECODER dec;
static BITSTREAM bs;

uint32_t rounding;
uint32_t quant;
uint32_t fcode;

inline static int stream_read_char(char *d)
{
    return (*d & 0xff);
}

inline static unsigned int stream_read_dword(char *s)
{
    unsigned int y;
    y=stream_read_char(s);
    y=(y<<8)|stream_read_char(s+1);
    y=(y<<8)|stream_read_char(s+2);
    y=(y<<8)|stream_read_char(s+3);
    return y;
}

// Determine of the compressed frame is a keyframe for direct copy
static int quicktime_divx4_is_key(unsigned char *data, long size)
{
        int result = 0;
        int i;

        for(i = 0; i < size - 5; i++)
        {
                if( data[i]     == 0x00 &&
                        data[i + 1] == 0x00 &&
                        data[i + 2] == 0x01 &&
                        data[i + 3] == 0xb6)
                {
                        if((data[i + 4] & 0xc0) == 0x0)
                                return 1;
                        else
                                return 0;
                }
        }

        return result;
}

static int quicktime_divx3_is_key(char *d)
{
    int32_t c=0;

    c=stream_read_dword(d);
    if(c&0x40000000) return(0);

    return(1);
}

/*-------------------------------------------------
 *
 * single function interface
 *
 *-------------------------------------------------*/

int tc_filter(frame_list_t *ptr_, char *options)
{
  vframe_list_t *ptr = (vframe_list_t *)ptr_;
  vob_t *vob=NULL;

  int pre=0, vid=0;

  int cc0=0, cc1=0, cc2=0, cc3=0;


  if(ptr->tag & TC_FILTER_GET_CONFIG) {
      optstr_filter_desc (options, MOD_NAME, MOD_CAP, MOD_VERSION, "Thomas Oestreich", "VE", "1");
      return 0;
  }

  //----------------------------------
  //
  // filter init
  //
  //----------------------------------

  if(ptr->tag & TC_FILTER_INIT) {

    if((vob = tc_get_vob())==NULL) return(-1);

    // filter init ok.

    if(verbose) tc_log_info(MOD_NAME, "%s %s", MOD_VERSION, MOD_CAP);

    if(verbose) tc_log_info(MOD_NAME, "options=%s", options);

    tc_snprintf(buffer, sizeof(buffer), "%s-%s", PACKAGE, VERSION);

    //init filter

    if(verbose) tc_log_info(MOD_NAME, "divxkey");

    return(0);
  }

  //----------------------------------
  //
  // filter close
  //
  //----------------------------------

    if(ptr->tag & TC_FILTER_CLOSE) {

    return(0);
  }

  //----------------------------------
  //
  // filter frame routine
  //
  //----------------------------------

  if(verbose & TC_STATS) tc_log_info(MOD_NAME, "%s/%s %s %s", vob->mod_path, MOD_NAME, MOD_VERSION, MOD_CAP);

  // tag variable indicates, if we are called before
  // transcodes internal video/audo frame processing routines
  // or after and determines video/audio context

  pre = (ptr->tag & TC_PRE_M_PROCESS)? 1:0;
  vid = (ptr->tag & TC_VIDEO)? 1:0;

  if(pre && vid) {

      bs_init_tc(&bs, (char*) ptr->video_buf);

      cc0 = bs_vol(&bs, &dec);
      cc1 = bs_vop(&bs, &dec, &rounding, &quant, &fcode);

      if(verbose & TC_STATS) tc_log_info(MOD_NAME, "frame=%d vop=%d vol=%d (%d %d %d)", ptr->id, cc1, cc0, rounding, quant, fcode);


      // DivX ;-)

      if(vob->v_codec_flag == TC_CODEC_DIVX3) {

	  if(ptr->video_size>4) cc3=quicktime_divx3_is_key((unsigned char *)ptr->video_buf);

	  if(cc3) ptr->attributes |= TC_FRAME_IS_KEYFRAME;
	  if((verbose & TC_DEBUG) && cc3) tc_log_info(MOD_NAME, "key (intra) @ %d", ptr->id);

      }

      // DivX

      if(vob->v_codec_flag == TC_CODEC_DIVX4 || vob->v_codec_flag == TC_CODEC_DIVX5) {

	  cc2=quicktime_divx4_is_key((unsigned char *)ptr->video_buf, (long) ptr->video_size);
	  if(cc2  && cc1 == I_VOP) ptr->attributes |= TC_FRAME_IS_KEYFRAME;
	  if((verbose & TC_DEBUG) && cc2 && cc1 == I_VOP) tc_log_info(MOD_NAME, "key (intra) @ %d", ptr->id);
      }
  }

  return(0);
}
