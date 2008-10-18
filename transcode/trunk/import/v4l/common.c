/*
 *  common.c
 *
 *  Copyright (C) Thomas Oestreich - January 2002
 *
 *  This file is part of transcode, a video stream  processing tool
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


#include "vcr.h"

pthread_mutex_t capture_lock=PTHREAD_MUTEX_INITIALIZER;

int capture=CAP_STATUS_INIT;


int capture_get_status()
{
  return(capture);
}


void capture_set_status(int flag, int mode)
{

  pthread_mutex_lock(&capture_lock);

  if(mode) {
    capture |=  flag;
  } else {
    capture &= ~flag;
  }

  pthread_mutex_unlock(&capture_lock);

}

double v4l_counter_init()
{
  struct timeval tv;

  if(gettimeofday(&tv, NULL)<0) return(0.0);

  return((double) tv.tv_sec + tv.tv_usec/1000000.0);
}

void v4l_counter_print(char *s, long _n, double ini, double *last)
{
  struct timeval tv;

  double tt;

  if(gettimeofday(&tv, NULL)<0) return;

  tt = (double) tv.tv_sec + tv.tv_usec/1000000.0;

  tc_log_msg("import_v4l.so", "%s frame=%6ld  pts=%.6f  diff_pts=%.6f", s, _n, tt-ini, tt-(*last));

  *last = tt;
}
