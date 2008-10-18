/*
 *  video.c
 *
 *  Copyright (C) Thomas Oestreich - January 2002
 *  some code from xawtv: (c) 1997-2001 Gerd Knorr <kraxel@bytesex.org>
 *  updates for general channel selection and .xavtv processing by
 *  Chris C. Hoover <cchoover@charter.net>
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

#include "transcode.h"
#include "libtc/cfgfile.h"
#include "aclib/imgconvert.h"

#include "vcr.h"
#include "video.h"
#include "counter.h"
#include "frequencies.h"

#include "src/filter.h"

struct fgdevice fg;

static int fh;

long int v_startsec, v_startusec;

static struct video_capability capability;
static struct video_channel *channels = NULL;
static struct video_audio audio;
static struct video_picture pict;
static struct video_tuner tuner;

static int v4l_max_buffer = 0;

int do_audio = 0;

#define cf_get_named_key(x,y,z) cf_get_named_section_value_of_key(x,y,z)

static int read_xawtv_config(char *pStation, char *pNorm,
                             unsigned long *tfreq_ret,
                             int *bright_ret, int *contrast_ret,
                             int *color_ret, int *hue_ret)
{
  int found_station = 0;
#if 0  /* FIXME: rewrite to use libtc/cfgfile.c (AC) */
  int j;
  char *pHome, pConfig[TC_BUF_MIN], pChannel[8], *pTemp;
  unsigned long tfreq = *tfreq_ret;
  int fine = 0, bright = *bright_ret, contrast = *contrast_ret,
      color = *color_ret, hue = *hue_ret;
  CF_ROOT_TYPE *pRoot;
  CF_SECTION_TYPE *pSection;
  extern struct STRTAB chanlist_names[];
  extern struct CHANLISTS chanlists[];
  struct STRTAB channame;
  struct CHANLIST *pChanlist;

  /*
   * let's see if we can find a .xawtv
   * file in the users home directory.
   *
   * this is needed even if the chosen
   * channel has no tuner because we
   * need to set the values for bright,
   * contrast, hue and saturation in
   * any case.
   */
  /*
   * get the name of the users home directory
   */
  if ((pHome = getenv ("HOME")) != NULL) {
    /*
     * construct the filename from $HOME/.xawtv
     */
    if (tc_snprintf (pConfig, TC_BUF_MIN, "%s/%s", pHome, ".xawtv") > 0) {
      /*
       * open and read the config file ($HOME/.xawtv)
       */
      if ((pRoot = cf_read (pConfig)) != NULL) {
        /*
         * get a pointer to the first section in the file.
         */
        if ((pSection = cf_get_section (pRoot)) != NULL) {
          do {
            /*
             * get the default values for bright, contrast, hue and
             * saturation from the .xawtv file
             */
            if (strncmp (pSection->name, "defaults", strlen (pSection->name)) == 0) {
              /*
               * get the default brightness value from .xawtv
               */
              if ((pTemp = cf_get_named_key (pRoot, pSection->name, "bright")) != NULL) {

                char *percentsign;

                if ((percentsign = strstr (pTemp, "%")) == NULL) {
                  bright = atoi (pTemp);
                }
                else {
                  *percentsign = '\0';
                  bright = (int) ((float) atoi (pTemp) * 655.36f);
                  *percentsign = '%';
                }

              }

              /*
               * get the default contrast value from .xawtv
               */
              if ((pTemp = cf_get_named_key (pRoot, pSection->name, "contrast")) != NULL) {

                char *percentsign;

                if ((percentsign = strstr (pTemp, "%")) == NULL) {
                  contrast = atoi (pTemp);
                }
                else {
                  *percentsign = '\0';
                  contrast = (int) ((float) atoi (pTemp) * 655.36f);
                  *percentsign = '%';
                }

              }

              /*
               * get the default color value from .xawtv
               */
              if ((pTemp = cf_get_named_key (pRoot, pSection->name, "color")) != NULL) {

                char *percentsign;

                if ((percentsign = strstr (pTemp, "%")) == NULL) {
                  color = atoi (pTemp);
                }
                else {
                  *percentsign = '\0';
                  color = (int) ((float) atoi (pTemp) * 655.36f);
                  *percentsign = '%';
                }

              }

              /*
               * get the default hue value from .xawtv
               */
              if ((pTemp = cf_get_named_key (pRoot, pSection->name, "hue")) != NULL) {

                char *percentsign;

                if ((percentsign = strstr (pTemp, "%")) == NULL) {
                  hue = atoi (pTemp);
                }
                else {
                  *percentsign = '\0';
                  hue = (int) ((float) atoi (pTemp) * 655.36f);
                  *percentsign = '%';
                }

              }

              if (!channel_has_tuner)
                break;
            }

            if (channel_has_tuner) {
              /*
               * get the freq table to use from the 'global' section.
               */
              if (strncmp (pSection->name, "global", strlen (pSection->name)) == 0)
                if ((pTemp = cf_get_named_key (pRoot, pSection->name, "freqtab")) != NULL)
                  tc_snprintf (pNorm, TC_BUF_MIN - 1, "%s", pTemp);

              /*
               * is this section a channel?
               */
              if ((pTemp = cf_get_named_key (pRoot, pSection->name, "channel")) != NULL) {
                tc_snprintf (pChannel, 7, "%s", pTemp);
                /*
                 * is this section our channel?
                 */
                if (station_id != NULL &&
                    (strcmp (station_id, pSection->name) == 0 || strcmp (station_id, pChannel) == 0)) {

                  if ((pTemp = cf_get_named_key (pRoot, pSection->name, "fine")) != NULL)
                    fine = atoi (pTemp);

                  i = 0;
                  j = 1;
                  channame = chanlist_names[i];
                  /*
                   * scan the chanlist_names array looking for the freqtab.
                   */
                  while (channame.name != NULL) {
                    if (!strcmp (pNorm, channame.name)) {
                      pChanlist = chanlists[i].list;
                      /*
                       * scan the freqtab looking for the station.
                       */
                      while (pChanlist->name != NULL) {
                        if (!strcmp (pChannel, pChanlist->name)) {
                          tfreq = (unsigned long) ((pChanlist->freq * .016 + fine));
                          break;
                        }
                        pChanlist = chanlists[i].list + j++;
                      }
                      break;
                    }
                    channame = chanlist_names[++i];
                  }

                  found_station = 1;
                }
              }
              else if ((pTemp = cf_get_named_key (pRoot, pSection->name, "freq")) != NULL) {
                if (station_id != NULL &&
                    (strcmp (station_id, pSection->name) == 0 || strcmp (station_id, pTemp) == 0)) {
                  /*
                   * xawtv freq is in MHz
                   */
                  tfreq = (unsigned long) (strtod (pTemp, NULL) * 16.0);
                  found_station = 1;
                }
              }

              if (found_station) {
                tc_snprintf (pStation, TC_BUF_MIN - 1, "%s", pSection->name);
                if ((pTemp = cf_get_named_key (pRoot, pSection->name, "bright")) != NULL) {
                  bright = atoi (pTemp);
                  if (strstr (pTemp, "%") != NULL) {
                    bright *= 655.36f;
                  }
                }
                if ((pTemp = cf_get_named_key (pRoot, pSection->name, "contrast")) != NULL) {
                  contrast = atoi (pTemp);
                  if (strstr (pTemp, "%") != NULL) {
                    contrast *= 655.36f;
                  }
                }
                if ((pTemp = cf_get_named_key (pRoot, pSection->name, "color")) != NULL) {
                  color = atoi (pTemp);
                  if (strstr (pTemp, "%") != NULL) {
                    color *= 655.36f;
                  }
                }
                if ((pTemp = cf_get_named_key (pRoot, pSection->name, "hue")) != NULL) {
                  hue = atoi (pTemp);
                  if (strstr (pTemp, "%") != NULL) {
                    hue *= 655.36f;
                  }
                }

                if (verb)
                  tc_log_info("import_v4l.so", "\"%s\": using .xawtv from %s, freq=%.2fMHZ",
			      pStation, pHome, (float) tfreq / 16.0);
                break;
              }
            }
          } while ((pSection = cf_get_next_section (pRoot, pSection)) != NULL);
        }
        if (channel_has_tuner && !found_station && station_id != NULL) {
          tc_log_warn("import_v4l.so", "Cannot find channel/name %s in .xawtv from %s", station_id, pHome);
          return -1;
        }
        CF_FREE_ROOT (pRoot);

        if (verb)
          tc_log_info("import_v4l.so", "%s: station=%s bright=%2.0f%% contrast=%2.0f%% color=%2.0f%% hue=%2.0f%%",
                  pNorm, station_id == NULL ? "none" : station_id,
                  (float) bright / 65535 * 100, (float) contrast / 65535 * 100,
                  (float) color / 65535 * 100, (float) hue / 65535 * 100);
      }
    }
  }
  *bright_ret = bright;
  *contrast_ret = contrast;
  *color_ret = color;
  *hue_ret = hue;
  *tfreq_ret = tfreq;
#endif  // #if 0
  return found_station;
}

int
video_grab_init (const char *device,  // device the video/audio comes from [/dev/video0, etc]
                 int chanid,    // channel on that device [composit, s-video, tuner, etc] (-1 == don't touch)
                 const char *station_id,      // if tuner, staion to tune ["11", "E3", "PBS", "ARD", etc]
                 int w,         // image width
                 int h,         // image height
                 int fmt,       // pixel format
                 int verb,      // verbosity flag
                 int _do_audio  // init audio or not?
  )
{
  int i, channel_has_tuner = 0;
  char pNorm[TC_BUF_MIN], pStation[TC_BUF_MIN];
  unsigned long tfreq = 0;
  int bright = -1, contrast = -1, color = -1, hue = -1;
  int found_station = 0;

  do_audio = _do_audio;

  tc_snprintf (pNorm, TC_BUF_MIN - 1, "%s", "don't touch");

  // open video device
  if ((fh = open (device, O_RDWR)) == -1) {
    tc_log_perror("import_v4l.so", "grab device open");
    return (-1);
  }

  // get grabber caps
  if (-1 == ioctl (fh, VIDIOCGCAP, &capability)) {
    tc_log_perror("import_v4l.so", "query capabilities");
    return (-1);
  }

  if (chanid < 0)
    goto dont_touch;

  if (!channels) {
    channels = tc_malloc (sizeof (struct video_channel) * capability.channels);
  }

  channels[chanid].channel = chanid;

  if (-1 == ioctl (fh, VIDIOCGCHAN, &channels[chanid])) {
    tc_log_perror("import_v4l.so", "invalid channel");
    return (-1);
  }

  tc_snprintf (pNorm, TC_BUF_MIN - 1, "%s", channels[chanid].name);

  if (channels[chanid].flags & VIDEO_VC_TUNER) {
    channel_has_tuner = 1;
    // get tuner caps
    if (-1 == ioctl (fh, VIDIOCGTUNER, &tuner)) {
      tc_log_perror("import_v4l.so", "query tuner");
      return (-1);
    }

    // print tuner capability
    if (verb)
      tc_log_info("import_v4l.so", "%s: has[ %s%s%s%s%s] is[ %s%s%s%s]", tuner.name,
		  (tuner.flags & VIDEO_TUNER_PAL) ? "PAL " : "",
		  (tuner.flags & VIDEO_TUNER_NTSC) ? "NTSC " : "",
		  (tuner.flags & VIDEO_TUNER_SECAM) ? "SECAM " : "",
		  (tuner.flags & VIDEO_TUNER_LOW) ? "USE-KHZ " : "",
		  (tuner.flags & VIDEO_TUNER_NORM) ? "AGILE " : "",
		  (tuner.mode == VIDEO_MODE_PAL) ? "PAL " : "",
		  (tuner.mode == VIDEO_MODE_NTSC) ? "NTSC " : "",
		  (tuner.mode == VIDEO_MODE_SECAM) ? "SECAM " : "",
		  (tuner.mode == VIDEO_MODE_AUTO) ? "AUTO " : "");
  }

  /* FIXME: we shouldn't read other apps' config files */
  found_station = read_xawtv_config(pStation, pNorm, &tfreq,
				    &bright, &contrast, &color, &hue);

  // print channel capability
  if (verb)
    tc_log_info("import_v4l.so", "%s: input #%d, %s%s%s%s",
		channels[chanid].name, chanid,
		(channels[chanid].flags & VIDEO_VC_TUNER) ? "tuner " : "",
		(channels[chanid].flags & VIDEO_VC_AUDIO) ? "audio " : "",
		(channels[chanid].type & VIDEO_TYPE_TV) ? "tv " : "",
		(channels[chanid].type & VIDEO_TYPE_CAMERA) ? "camera " : "");

dont_touch:

  if (do_audio) {
    // audio parameter
    if (-1 == ioctl (fh, VIDIOCGAUDIO, &audio)) {
      //tc_log_perror("import_v4l.so", "ioctl VIDIOCGAUDIO");
      //return(-1);
      if (verb)
        tc_log_info("import_v4l.so", "device has no audio channel");
      do_audio = 0;             //reset
    }

    if (verb) {
      char buf1[100], buf2[100], buf3[100], buf4[100];
      *buf1 = *buf2 = *buf3 = *buf4 = 0;
      if (audio.flags & VIDEO_AUDIO_MUTABLE)
        tc_snprintf (buf1, sizeof(buf1), " muted=%s",
		     (audio.flags & VIDEO_AUDIO_MUTE) ? "yes" : "no");
      if (audio.flags & VIDEO_AUDIO_VOLUME)
        tc_snprintf (buf2, sizeof(buf2), " volume=%2.0f%%",
		     (float) audio.volume / 65535 * 100);
      if (audio.flags & VIDEO_AUDIO_BASS)
        tc_snprintf (buf3, sizeof(buf3), " bass=%2.0f%%",
		     (float) audio.bass / 65535 * 100);
      if (audio.flags & VIDEO_AUDIO_TREBLE)
        tc_snprintf (buf4, sizeof(buf4), " treble=%2.0f%%",
		     (float) audio.treble / 65535 * 100);
      tc_log_info("import_v4l.so", "(audio-%s):%s%s%s%s", audio.name,
		  buf1, buf2, buf3, buf4);
    }
  }

  // picture parameter
  if (-1 == ioctl (fh, VIDIOCGPICT, &pict)) {
    tc_log_perror("import_v4l.so", "ioctl VIDIOCGPICT");
    return (-1);
  }

  if (verb)
    tc_log_info("import_v4l.so", "picture: brightness=%2.0f%% hue=%2.0f%% colour=%2.0f%% contrast=%2.0f%%",
		(float) pict.brightness / 65535 * 100,
		(float) pict.hue / 65535 * 100,
		(float) pict.colour / 65535 * 100,
		(float) pict.contrast / 65535 * 100);

  /* -------------------------------------------------------------------
   *
   * user section
   *
   */

  if (do_audio) {
    // turn on audio
    grab_setattr (GRAB_ATTR_MUTE, 0);

    // turn on stereo mode
    grab_setattr (GRAB_ATTR_MODE, 1);

    // reduce output volume to 7/8 of maximum
    grab_setattr (GRAB_ATTR_VOLUME, 57344);
  }

  if (chanid >= 0) {
    // set input channel
    if (-1 == ioctl (fh, VIDIOCSCHAN, &channels[chanid])) {
      tc_log_perror("import_v4l.so", "invalid input channel");
      return (-1);
    }

    if (bright >= 0) {
      grab_setattr (GRAB_ATTR_BRIGHT, bright);
      if (verb) {
        tc_log_info("import_v4l.so", "setattr: setting bright   to %d (%2.0f%%).",
		    bright, (float) bright / 65535.0 * 100.0);
      }
    }
    if (contrast >= 0) {
      grab_setattr (GRAB_ATTR_CONTRAST, contrast);
      if (verb) {
        tc_log_info("import_v4l.so", "setattr: setting contrast to %d (%2.0f%%).",
		    contrast, (float) contrast / 65535.0 * 100.0);

      }
    }
    if (color >= 0) {
      grab_setattr (GRAB_ATTR_COLOR, color);
      if (verb) {
        tc_log_info("import_v4l.so", "setattr: setting color    to %d (%2.0f%%).",
		    color, (float) color / 65535.0 * 100.0);
      }
    }
    if (hue >= 0) {
      grab_setattr (GRAB_ATTR_HUE, hue);
      if (verb) {
        tc_log_info("import_v4l.so", "setattr: setting hue      to %d (%2.0f%%).",
		    hue, (float) hue / 65535.0 * 100.0);
      }
    }

    if (channel_has_tuner && found_station) {
      // set station frequency
      if (-1 == ioctl (fh, VIDIOCSFREQ, &tfreq)) {
        tc_log_perror("import_v4l.so", "invalid tuner frequency");
        return (-1);
      }
    }
  }

  // store variables
  fg.video_dev = fh;

  fg.width = w;
  fg.height = h;

  fg.format = fmt;

  if (fmt == VIDEO_PALETTE_RGB24) {
    pict.palette = fmt;
    pict.depth = 24;
    if (-1 == ioctl (fh, VIDIOCSPICT, &pict)) {
      tc_log_error("import_v4l.so", "Cannot not set RGB picture attributes. ioctl VIDIOCSPICT: %s", strerror(errno));
      return (-1);
    }
  }

  /*
  if (fmt == VIDEO_PALETTE_YUV420P) {
    pict.palette = fmt;
    if (-1 == ioctl(fh, VIDIOCSPICT,&pict)) {
      tc_log_error("import_v4l.so", "Cannot not set YUV picture attributes. ioctl VIDIOCSPICT: %s", strerror(errno));
      return(-1);
    }
  }
   */

  if (fmt == VIDEO_PALETTE_YUV422) {
    pict.palette = fmt;
    if (-1 == ioctl (fh, VIDIOCSPICT, &pict)) {
      tc_log_error("import_v4l.so", "Cannot not set YUV 2 picture attributes. ioctl VIDIOCSPICT: %s", strerror(errno));
      return(-1);
    }
  }

  // retrieve buffer size and offsets

  if (ioctl (fg.video_dev, VIDIOCGMBUF, &fg.vid_mbuf) == -1) {
    tc_log_perror("import_v4l.so", "ioctl (VIDIOCGMBUF)");
    return (-1);
  }

  if (verb)
    tc_log_info("import_v4l.so", "%d frame buffer(s) available", fg.vid_mbuf.frames);

  v4l_max_buffer = fg.vid_mbuf.frames;

  if (!v4l_max_buffer) {
    tc_log_error("import_v4l.so", "no frame buffer(s) available");
    return (-1);
  }

  // map grabber memory onto user space

  fg.video_map = mmap (0, fg.vid_mbuf.size, PROT_READ | PROT_WRITE, MAP_SHARED, fg.video_dev, 0);
  if ((unsigned char *) -1 == (unsigned char *) fg.video_map) {
    tc_log_perror("import_v4l.so", "mmap()");
    return (-1);
  }

  // generate mmap records

  for (i = 0; i < v4l_max_buffer; i++) {

    fg.vid_mmap[i].format = fg.format;
    fg.vid_mmap[i].frame = i;
    fg.vid_mmap[i].width = w;
    fg.vid_mmap[i].height = h;

  }

  // calculate framebuffer size
  switch (fg.format) {
  case VIDEO_PALETTE_RGB24:
    fg.image_pixels = w * h;
    fg.image_size = fg.image_pixels * 3;

    break;

  case VIDEO_PALETTE_YUV420P:
    fg.image_pixels = w * h;
    fg.image_size = (fg.image_pixels * 3) / 2;
    break;

  case VIDEO_PALETTE_YUV422:
    fg.image_pixels = w * h;
    fg.image_size = (fg.image_pixels * 3);


    break;
  }

  // reset grab counter variables

  fg.current_grab_number = 0;
  fg.totalframecount = 0;

  // initiate capture for frames

  for (i = 1; i < v4l_max_buffer + 1; i++)
    if (ioctl (fg.video_dev, VIDIOCMCAPTURE, &fg.vid_mmap[i % v4l_max_buffer]) == -1)
      tc_log_perror("import_v4l.so", "VIDIOCMCAPTURE");

  if (found_station)
    tc_log_info("import_v4l.so", "video device OK - recording from [%s]", pStation);

  return (0);
}


int
video_grab_close (int do_audio)
{

  // turn off audio
  if (do_audio)
    grab_setattr (GRAB_ATTR_MUTE, 1);

  // video device
  munmap (fg.video_map, fg.vid_mbuf.size);
  close (fg.video_dev);

  return (0);

}


static struct GRAB_ATTR
{
  int id;
  int have;
  int get;
  int set;
  void *arg;
} grab_attr[] = {
  {
  GRAB_ATTR_VOLUME, 1, VIDIOCGAUDIO, VIDIOCSAUDIO, &audio}, {
  GRAB_ATTR_MUTE, 1, VIDIOCGAUDIO, VIDIOCSAUDIO, &audio}, {
  GRAB_ATTR_MODE, 1, VIDIOCGAUDIO, VIDIOCSAUDIO, &audio}, {
  GRAB_ATTR_COLOR, 1, VIDIOCGPICT, VIDIOCSPICT, &pict}, {
  GRAB_ATTR_BRIGHT, 1, VIDIOCGPICT, VIDIOCSPICT, &pict}, {
  GRAB_ATTR_HUE, 1, VIDIOCGPICT, VIDIOCSPICT, &pict}, {
  GRAB_ATTR_CONTRAST, 1, VIDIOCGPICT, VIDIOCSPICT, &pict},
};


int
grab_getattr (int id)
{
  int i;

  for (i = 0; i < NUM_ATTR; i++)
    if (id == grab_attr[i].id && grab_attr[i].have)
      break;
  if (i == NUM_ATTR)
    return -1;
  if (-1 == ioctl (fh, grab_attr[i].get, grab_attr[i].arg))
    tc_log_perror("import_v4l.so", "ioctl get");

  switch (id) {
  case GRAB_ATTR_VOLUME:
    return audio.volume;
  case GRAB_ATTR_MUTE:
    return audio.flags & VIDEO_AUDIO_MUTE;
  case GRAB_ATTR_MODE:
    return audio.mode;
  case GRAB_ATTR_COLOR:
    return pict.colour;
  case GRAB_ATTR_BRIGHT:
    return pict.brightness;
  case GRAB_ATTR_HUE:
    return pict.hue;
  case GRAB_ATTR_CONTRAST:
    return pict.contrast;
  default:
    return -1;
  }
}

int
grab_setattr (int id, int val)
{
  int i;

  /* read ... */
  for (i = 0; i < NUM_ATTR; i++)
    if (id == grab_attr[i].id && grab_attr[i].have)
      break;
  if (i == NUM_ATTR)
    return -1;
  if (-1 == ioctl (fh, grab_attr[i].set, grab_attr[i].arg))
    tc_log_perror("import_v4l.so", "ioctl get");

  /* ... modify ... */
  switch (id) {
  case GRAB_ATTR_VOLUME:
    audio.volume = val;
    break;
  case GRAB_ATTR_MUTE:
    if (val)
      audio.flags |= VIDEO_AUDIO_MUTE;
    else
      audio.flags &= ~VIDEO_AUDIO_MUTE;
    break;
  case GRAB_ATTR_MODE:
    audio.mode = val;
    break;
  case GRAB_ATTR_COLOR:
    pict.colour = val;
    break;
  case GRAB_ATTR_BRIGHT:
    pict.brightness = val;
    break;
  case GRAB_ATTR_HUE:
    pict.hue = val;
    break;
  case GRAB_ATTR_CONTRAST:
    pict.contrast = val;
    break;
  default:
    return -1;
  }

  /* ... write */
  if (-1 == ioctl (fh, grab_attr[i].set, grab_attr[i].arg))
    tc_log_perror("import_v4l.so", "ioctl set");
  return 0;
}


int
video_grab_frame (char *buffer)
{
  uint8_t *p;

  // advance grab-frame number
  fg.current_grab_number = ((fg.current_grab_number + 1) % v4l_max_buffer);

  // wait for next image in the sequence to complete grabbing
  if (ioctl (fg.video_dev, VIDIOCSYNC, &fg.vid_mmap[fg.current_grab_number]) == -1) {
    tc_log_perror("import_v4l.so", "VIDIOCSYNC");
    return (-1);
  }

  //copy frame
  p = &fg.video_map[fg.vid_mbuf.offsets[fg.current_grab_number]];

  switch (fg.format) {

  case VIDEO_PALETTE_RGB24:
    ac_memcpy (buffer, p, fg.image_size);

    break;

  case VIDEO_PALETTE_YUV420P:
    ac_memcpy (buffer, p, fg.image_pixels);
    ac_memcpy (buffer + fg.image_pixels, p + (fg.image_pixels * 10 / 8), fg.image_pixels >> 2);
    ac_memcpy (buffer + (fg.image_pixels * 10 / 8), p + fg.image_pixels, fg.image_pixels >> 2);

    break;

  case VIDEO_PALETTE_YUV422: {
    uint8_t *planes[3];
    YUV_INIT_PLANES(planes, buffer, IMG_YUV_DEFAULT, fg.width, fg.height);
    ac_imgconvert(&p, IMG_YUY2, planes, IMG_YUV_DEFAULT, fg.width, fg.height);

    break;
  }
  }

  fg.totalframecount++;

  // issue new grab command for this buffer

  if (ioctl (fg.video_dev, VIDIOCMCAPTURE, &fg.vid_mmap[fg.current_grab_number]) == -1) {
    tc_log_perror("import_v4l.so", "VIDIOCMCAPTURE");
    return (-1);
  }

  return (0);

}
