/*
 *  video.h
 *
 *  Copyright (C) Thomas Oestreich - January 2002
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


#ifndef _TC_V4L_VIDEO_H
#define _TC_V4L_VIDEO_H 1

#include "videodev.h"
#include "vcr.h"

#define GRAB_ATTR_VOLUME     1
#define GRAB_ATTR_MUTE       2
#define GRAB_ATTR_MODE       3

#define GRAB_ATTR_COLOR     11
#define GRAB_ATTR_BRIGHT    12
#define GRAB_ATTR_HUE       13
#define GRAB_ATTR_CONTRAST  14

#define NUM_ATTR (sizeof(grab_attr)/sizeof(struct GRAB_ATTR))

int video_grab_init(const char *device, int chanid, const char *station_id, int w, int h, int fmt, int verb, int do_audio);
int video_grab_frame(char *buffer);
int video_grab_close(int do_audio);

int grab_setattr(int id, int val);
int grab_getattr(int id);

#define MAX_BUFFER 32

struct fgdevice {
  int video_dev;

  int width;
  int height;
  int input;
  int format;

  struct video_mmap vid_mmap[MAX_BUFFER];
  int current_grab_number;
  struct video_mbuf vid_mbuf;
  char *video_map;
  int grabbing_active;
  int have_new_frame;
  void *current_image;
  pthread_mutex_t buffer_mutex;
  pthread_t grab_thread;
  pthread_cond_t buffer_cond;
  int totalframecount;
  int image_size;
  int image_pixels;
  int framecount;
  int fps_update_interval;
  double fps;
  double lasttime;
  unsigned short *y8_to_rgb565;
  unsigned char  *rgb565_to_y8;
};

extern int counter_get_range(void);

#endif
