/*
 *  import_mpeg.c
 *
 *  Copyright (C) Francesco Romani - October 2005
 *
 *  This file would be part of transcode, a linux video stream  processing tool
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <mpeg2.h>
#include <mpeg2convert.h>

#include "optstr.h"
#include "transcode.h"
#include "ac.h"
#include "mpeglib.h"

#define MOD_NAME    "import_mpeg.so"
#define MOD_VERSION "v0.1.4 (2005-10-08)"
#define MOD_CODEC   "(video) MPEG"

extern int tc_accel;

static int verbose_flag = TC_QUIET;
static int capability_flag = TC_CAP_RGB | TC_CAP_YUV | TC_CAP_VID;

#define MOD_PRE mpeg
#include "import_def.h"

static int rgb_mode = FALSE; // flag

static mpeg_file_t *MFILE;
static mpeg_t *MPEG;

static mpeg2dec_t *decoder;
static const mpeg2_info_t *info;
static const mpeg2_sequence_t *sequence;
static mpeg2_state_t state;

static void
copy_frame(const mpeg2_sequence_t *sequence, const mpeg2_info_t *info,
           transfer_t *param) {
    size_t len = 0;
    // Y plane
    len = sequence->width * sequence->height;

    tc_memcpy(param->buffer, info->display_fbuf->buf[0], len);
    param->size = len;
    
    len = sequence->chroma_width * sequence->chroma_height; 
    // U plane
    tc_memcpy(param->buffer + param->size, 
              info->display_fbuf->buf[2], len);
    param->size += len;
    // V plane
    tc_memcpy(param->buffer + param->size, 
              info->display_fbuf->buf[1], len);
    param->size += len;
}

static uint32_t
translate_accel(int accel) {
    uint32_t mpeg_accel = 0; // NO accel
    
    switch(accel) {
        case MM_C:
            mpeg_accel = 0;
            break;
        case MM_MMX:
            mpeg_accel |= MPEG2_ACCEL_X86_MMX;
            break;
        case MM_MMXEXT:
            mpeg_accel |= MPEG2_ACCEL_X86_MMXEXT;
            mpeg_accel |= MPEG2_ACCEL_X86_MMX;
            break;
        case MM_3DNOW:
            mpeg_accel |= MPEG2_ACCEL_X86_3DNOW;
            mpeg_accel |= MPEG2_ACCEL_X86_MMX;
            break;
        case (-1): // autodetect, fallthrough
        default:
            mpeg_accel = MPEG2_ACCEL_DETECT;
            break;
    }
    return mpeg_accel;
}


static void
show_accel(uint32_t mpeg_accel) {
    const char *acstr = "none (plain C)";
    
    if(mpeg_accel & MPEG2_ACCEL_X86_3DNOW)
        acstr = "3dnow";
    else if(mpeg_accel & MPEG2_ACCEL_X86_MMXEXT)
        acstr = "mmxext";
    else if(mpeg_accel & MPEG2_ACCEL_X86_MMX)
        acstr = "mmx";
    fprintf (stderr, "[%s] libmpeg2 acceleration: %s\n", 
            MOD_NAME, acstr);
}

/* ------------------------------------------------------------
 *
 * open stream
 *
 * ------------------------------------------------------------*/

MOD_open
{
    char faccel[64]; // force acceleration
    char *options = vob->im_v_string;
    int faccel_flag = 0;
    uint32_t ac = 0, rac = 0; // ac == accel, rac == requested accel

    if(param->flag != TC_VIDEO) { 
        return(TC_IMPORT_ERROR);
    }
    
    if(options != NULL && 
       optstr_get(options, "accel", "%[^:]", &faccel) >= 0) {
        if(!strcmp(faccel, "3dnow")) {
            rac |= MPEG2_ACCEL_X86_3DNOW;
            rac |= MPEG2_ACCEL_X86_MMX;
            faccel_flag = 1;
        } else if(!strcmp(faccel, "mmxext")) {
            rac |= MPEG2_ACCEL_X86_MMXEXT;
            rac |= MPEG2_ACCEL_X86_MMX;
            faccel_flag = 1;
        } else if(!strcmp(faccel, "mmx")) {
            rac |= MPEG2_ACCEL_X86_MMX;
            faccel_flag = 1;
        } else if(!strcmp(faccel, "none")) {
            // plain C code, no acceleration
            rac = 0;
            faccel_flag = 1;
        } else {
            fprintf(stderr, "[%s] unknown acceleration '%s' "
	    		    "(disabled)\n", MOD_NAME, faccel);
            fprintf(stderr, "[%s] avalaible accelerations: "
		    	    "3dnow mmxext mmx none (disabled)\n", MOD_NAME);
        }
    } else {
        rac = translate_accel(tc_accel);
    }

    if(vob->im_v_codec == CODEC_RGB) {
        rgb_mode = TRUE;
    }
    
    mpeg_set_logging(mpeg_log_null, stderr);
    
    fprintf(stderr, "[%s] native MPEG1/2 import module using "
            "MPEGlib and libmpeg2\n", MOD_NAME);
    
    MFILE = mpeg_file_open(vob->video_in_file, "r");
    if(!MFILE) {
        fprintf(stderr, "[%s] unable to open: %s\n", 
                        MOD_NAME, vob->video_in_file);
        return(TC_IMPORT_ERROR);
    }
    
    MPEG = mpeg_open(MPEG_TYPE_ANY, MFILE, MPEG_DEFAULT_FLAGS, NULL);
    if(!MPEG) {
        mpeg_file_close(MFILE);
        fprintf(stderr, "[%s] mpeg_open() failed\n", MOD_NAME);
        return(TC_IMPORT_ERROR);
    }
    
    ac = mpeg2_accel(rac);

    decoder = mpeg2_init();
    if(decoder == NULL) {
        fprintf(stderr, "[%s] could not allocate a "
                        "MPEG2 decoder object.\n", MOD_NAME);
        return(TC_IMPORT_ERROR);
    }
    info = mpeg2_info(decoder);

    param->fd = NULL;
    if(vob->ts_pid1 != 0) { // we doesn't support transport streams
        fprintf(stderr, "[%s] this import module doesn't support TS streams."
                        " Use old import_mpeg2 module instead.\n", MOD_NAME);
        return(TC_IMPORT_ERROR);
    }
    
    if(verbose_flag || faccel_flag) { 
        show_accel(ac);
    }

    return(TC_IMPORT_OK);
}

/* ------------------------------------------------------------ 
 *
 * decode  stream
 *
 * ------------------------------------------------------------*/

#define READS_MAX   (4096)

MOD_decode
{
    int decoding = TRUE;
    uint32_t reads = 0;
    const mpeg_pkt_t *pes = NULL;

    do {
        state = mpeg2_parse(decoder);
        sequence = info->sequence;
	
        switch(state) {
            case STATE_BUFFER:
                pes = mpeg_read_packet(MPEG, MPEG_STREAM_VIDEO(0));
                if(!pes) {
                    decoding = FALSE;
                    break;
                } else {
                    mpeg2_buffer(decoder, (uint8_t*)pes->data, 
                            (uint8_t*)(pes->data + pes->size)); // FIXME
                    if(reads++ > READS_MAX) {
                        fprintf(stderr, "[%s] reached read limit. "
                                "This should'nt happen. "
                                "Check your input source\n", 
                                MOD_NAME);
                        return(TC_IMPORT_ERROR);
                    }
                    mpeg_pkt_del(pes);
                }
                break;

            case STATE_SEQUENCE:
                if(rgb_mode == TRUE) {
                    mpeg2_convert(decoder, mpeg2convert_rgb24, NULL);
                }
                break;
            case STATE_SLICE:
            case STATE_END:
            case STATE_INVALID_END:
                if(info->display_fbuf) {
                    copy_frame(sequence, info, param);
                    reads = 0; // this is redundant?
                    return(TC_IMPORT_OK);
                }
            default:
                break;
        }
    } while (decoding);
    
    return(TC_IMPORT_ERROR);
}

/* ------------------------------------------------------------ 
 *
 * close stream
 *
 * ------------------------------------------------------------*/

MOD_close
{  
    mpeg_close(MPEG);

    mpeg_file_close(MFILE);

    mpeg2_close(decoder);
        
    return(TC_IMPORT_OK);
}

/*
 * vim: set tabstop=4
 */

