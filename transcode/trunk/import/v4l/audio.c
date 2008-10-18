/*
 *  audio.c
 *
 *  Copyright (C) Thomas Oestreich - January 2002
 *  some code from xawtv: (c) 1997-2001 Gerd Knorr <kraxel@bytesex.org>
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

#include "config.h"
#include "vcr.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef HAVE_SOUNDCARD_H
# include <soundcard.h>
#endif
#ifdef HAVE_SYS_SOUNDCARD_H
# include <sys/soundcard.h>
#endif

#include "audio.h"

/* -------------------------------------------------------------------- */

static char *names[] = SOUND_DEVICE_NAMES;

static int fd, blocksize;

static int  mix;
static int  dev = -1;
static int  volume;
static int  muted;
extern int  debug;

/* -------------------------------------------------------------------- */

static int verb=0;

int audio_grab_init(const char *dev, int rate, int bits, int chan, int _verb)
{

  struct MOVIE_PARAMS params;

  params.bits=bits;
  params.channels=chan;
  params.rate=rate;
  params.adev=dev;

  verb=_verb;

  if(-1==sound_open(&params)) {
    tc_log_warn("import_v4l.so", "sound init failed");
    return(-1);
  }

  return(0);
}

int audio_grab_frame(char *buffer, int bytes)
{

  int bytes_left=bytes;
  int offset=0;

  while(bytes_left>0) {

    if(blocksize>bytes_left) {

      if (bytes_left != read(fd, buffer + offset, bytes_left)) {
	tc_log_perror("import_v4l.so", "read /dev/dsp");
	return(-1);
      }

    } else {

      if (blocksize != read(fd, buffer + offset, blocksize)) {
	tc_log_perror("import_v4l.so", "read /dev/dsp");
	return(-1);
      }
    }

    offset += blocksize;
    bytes_left -= blocksize;

  }//bytes_left>0

  return(0);

}

void audio_grab_close(int do_audio)
{

  if(do_audio) sound_startrec(0);

  // audio device
  close(fd);
}

/* ------------------------------------------------------------- */

int sound_open(struct MOVIE_PARAMS *params)
{
    int afmt, frag;

    if (-1 == (fd = open(params->adev, O_RDONLY))) {
      tc_log_perror("import_v4l.so", "open audio device");
      goto err;
    }

    fcntl(fd, F_SETFD, FD_CLOEXEC);

    /* format */

    switch (params->bits) {

    case 16:

      afmt = AFMT_S16_LE;  //Signed 16 Low-Endian
      ioctl(fd, SNDCTL_DSP_SETFMT, &afmt);

      if (afmt != AFMT_S16_LE) {
	tc_log_warn("import_v4l.so", "16 bit sound not supported");
	goto err;
      }

      break;

    case 8:

      afmt = AFMT_U8;

      ioctl(fd, SNDCTL_DSP_SETFMT, &afmt);

      if (afmt != AFMT_U8) {
	tc_log_warn("import_v4l.so", "8 bit sound not supported");
	goto err;
      }

      break;

    default:
      tc_log_warn("import_v4l.so", "%d bit sound not supported", params->bits);
      goto err;
    }

    frag = 0x7fff000c;   //4k
    ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag);

    /* channels */
    ioctl(fd, SNDCTL_DSP_CHANNELS, &params->channels);

    /* sample rate */
    ioctl(fd, SNDCTL_DSP_SPEED, &params->rate);

    if (-1 == ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &blocksize))
      goto err;

    if(verb) tc_log_info("import_v4l.so", "audio blocksize %d", blocksize);

    //start recording
    sound_startrec(0);
    sound_startrec(1);

    return fd;

 err:
    return -1;
}

void sound_startrec(int on_off)
{
    long unsigned trigger;

    /* trigger record */

    trigger = (on_off) ? PCM_ENABLE_INPUT : ~PCM_ENABLE_INPUT;

    if (-1 == ioctl(fd,SNDCTL_DSP_SETTRIGGER, &trigger)) {
      tc_log_perror("import_v4l.so", "trigger record");
      exit(1);
    }
}


/* -------------------------------------------------------------------- */

int mixer_open(char *filename, char *device)
{
    int i, devmask;

    if (-1 == (mix = open(filename,O_RDONLY))) {
	tc_log_perror("import_v4l.so", "mixer open");
	return -1;
    }

    fcntl(mix,F_SETFD,FD_CLOEXEC);

    if (-1 == ioctl(mix,MIXER_READ(SOUND_MIXER_DEVMASK),&devmask)) {
	tc_log_perror("import_v4l.so", "mixer read devmask");
	return -1;
    }
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
	if ((1<<i) & devmask && strcasecmp(names[i],device) == 0) {
	    if (-1 == ioctl(mix,MIXER_READ(i),&volume)) {
		tc_log_perror("import_v4l.so", "mixer read volume");
		return -1;
	    } else {
		dev = i;
		muted = 0;
	    }
	}
    }
    if (-1 == dev) {
	char buf[1000];
	*buf = 0;
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
	    if ((1<<i) & devmask) {
		tc_snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
			    " '%s'", names[i]);
	    }
	}
	tc_log_warn("import_v4l.so", "mixer: device '%s' not found", device);
	tc_log_warn("import_v4l.so", "mixer: available: %s", buf);
    }
    return (-1 != dev) ? 0 : -1;
}

void
mixer_close()
{
    close(mix);
    dev = -1;
}

int
mixer_get_volume()
{
    if (-1 == ioctl(mix,MIXER_READ(dev),&volume)) {
	tc_log_perror("import_v4l.so", "mixer write volume");
	return -1;
    }
    return (-1 == dev) ? -1 : (volume & 0x7f);
}

int
mixer_set_volume(int val)
{
    if (-1 == dev)
	return -1;
    val   &= 0x7f;
    volume = val | (val << 8);;
    if (-1 == ioctl(mix,MIXER_WRITE(dev),&volume)) {
	tc_log_perror("import_v4l.so", "mixer write volume");
	return -1;
    }
    muted = 0;
    return 0;
}

int
mixer_mute()
{
    int zero=0;

    muted = 1;
    if (-1 == dev)
	return -1;
    mixer_get_volume();
    if (-1 == ioctl(mix,MIXER_WRITE(dev),&zero))
	return -1;
    return 0;
}

int
mixer_unmute()
{
    muted = 0;
    if (-1 == dev)
	return -1;
    if (-1 == ioctl(mix,MIXER_WRITE(dev),&volume))
	return -1;
    return 0;
}

int
mixer_get_muted()
{
    return (-1 == dev) ? -1 : muted;
}

