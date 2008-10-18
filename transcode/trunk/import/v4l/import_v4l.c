/*
 *  import_v4l.c
 *
 *  Copyright (C) Thomas Oestreich - February 2002
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

#include "src/transcode.h"
#include "libtcutil/optstr.h"

#include "vcr.h"

#define MOD_NAME    "import_v4l.so"
#define MOD_VERSION "v0.0.6 (2006-05-08)"
#define MOD_CODEC   "(video) v4l | (audio) PCM"

static int verbose_flag=TC_QUIET;
static int capability_flag=TC_CAP_RGB|TC_CAP_YUV|TC_CAP_PCM;

#define MOD_PRE v4l
#include "import_def.h"

#define MAX_DISPLAY_PTS 25

//static char *default_audio="/dev/dsp";
//static char *default_video="/dev/video";

static uint32_t aframe_cnt=0;
static uint32_t vframe_cnt=0;

static double aframe_pts0=0, aframe_pts=0;
static double vframe_pts0=0, vframe_pts=0;

static int audio_drop_frames=25;
static int video_drop_frames=0;
static int do_audio = 1;

static int do_resync = 1;

/* ------------------------------------------------------------
 *
 * open stream
 *
 * ------------------------------------------------------------*/

MOD_open
{
    int fmt = VIDEO_PALETTE_YUV420P;

    tc_log_warn(MOD_NAME, "this module is deprecated: "
                          "please use import_v4l2 instead");
    if (param->flag == TC_VIDEO) {
        char station_buf[TC_BUF_MIN] = { '\0' };
        char *station_id = NULL;
        int chan_id = -1;

        // print out
        if (verbose_flag) {
            tc_log_info(MOD_NAME, "video4linux video grabbing");
        }
        param->fd = NULL;

        if (vob->im_v_string != NULL) {
            optstr_get(vob->im_v_string, "chanid", "%i", &chan_id);
            if (optstr_get(vob->im_v_string, "stationid",
                           "%128s", station_buf) == 1) { /* XXX; magic number */
                station_id = station_buf;
            }
        }

        if ((vob->video_in_file && strlen(vob->video_in_file)>=11
          && strncmp(vob->video_in_file, "/dev/video1", 11))) {
            do_resync = 0; //no resync stuff for webcams
        }

        switch (vob->im_v_codec) {
          case CODEC_RGB:
            fmt = VIDEO_PALETTE_RGB24;
            break;
          case CODEC_YUV:
            if (vob->im_v_string && strlen (vob->im_v_string) > 0) {
                if ( (strcmp (vob->im_v_string, "yuv422")) == 0) {
                    fmt = VIDEO_PALETTE_YUV422;
                }
            } /* else we're fine with default (yuv420p) */
            break;
            /* default already set at initialization */
        }
        
        if (video_grab_init(vob->video_in_file, chan_id, station_id,
                            vob->im_v_width, vob->im_v_height,
                            fmt, verbose_flag, do_audio) < 0) {
            tc_log_error(MOD_NAME, "error grab init");
            return TC_IMPORT_ERROR;
        }

        vframe_pts = v4l_counter_init();
        vframe_pts0 = vframe_pts;
        if (do_audio) {
            video_drop_frames = audio_drop_frames - 
                                (int) ((vframe_pts0-aframe_pts0)*vob->fps);
        }
        if (verbose_flag) {
            tc_log_info(MOD_NAME, "dropping %d video frames for AV sync ",
                        video_drop_frames);
        }

        return TC_IMPORT_OK;
    }
    if (param->flag == TC_AUDIO) {
        if (verbose_flag) {
            tc_log_info(MOD_NAME, "video4linux audio grabbing");
        }

        if (audio_grab_init(vob->audio_in_file,
                            vob->a_rate, vob->a_bits, vob->a_chan,
                            verbose_flag) < 0) {
            return TC_IMPORT_ERROR;
        }
        aframe_pts = v4l_counter_init();
        aframe_pts0 = aframe_pts;
        param->fd = NULL;
        return TC_IMPORT_OK;
    }
    return TC_IMPORT_ERROR;
}

/* ------------------------------------------------------------
 *
 * decode  stream
 *
 * ------------------------------------------------------------*/

MOD_decode
{
    if (param->flag == TC_VIDEO) {
        if (!do_resync) {
            video_drop_frames = 1;
        }
        
        do {
            video_grab_frame(param->buffer);
            if ((verbose & TC_STATS) && vframe_cnt<MAX_DISPLAY_PTS) {
                v4l_counter_print("VIDEO", vframe_cnt, vframe_pts0,
                                  &vframe_pts);
            }
            ++vframe_cnt;
            --video_drop_frames;
        } while (video_drop_frames > 0);
        
        video_drop_frames = 1;
        
        return TC_IMPORT_OK;
    }

    if (param->flag == TC_AUDIO) {
        if (!do_resync) {
            audio_drop_frames = 1;
        }
    
        do {
            audio_grab_frame(param->buffer, param->size);
            if ((verbose & TC_STATS) && aframe_cnt<MAX_DISPLAY_PTS) {
                v4l_counter_print("AUDIO", aframe_cnt, aframe_pts0,
                                  &aframe_pts);
            }

            ++aframe_cnt;
            --audio_drop_frames;

        } while (audio_drop_frames>0);

        audio_drop_frames = 1;

        return TC_IMPORT_OK;
    }
    return TC_IMPORT_ERROR;
}

/* ------------------------------------------------------------
 *
 * close stream
 *
 * ------------------------------------------------------------*/

MOD_close
{
    if (param->flag == TC_VIDEO) {
        video_grab_close(do_audio);
        return TC_IMPORT_OK;
    }

    if (param->flag == TC_AUDIO) {
        audio_grab_close(do_audio);
        return TC_IMPORT_OK;
    }
    return TC_IMPORT_ERROR;
}


