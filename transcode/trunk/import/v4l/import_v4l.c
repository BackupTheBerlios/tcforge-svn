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

#include "transcode.h"
#include "libtc/libtc.h"
#include "libtc/optstr.h"
#include "aclib/imgconvert.h"

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "videodev.h"

#define MOD_NAME    "import_v4l.so"
#define MOD_VERSION "v0.1.1 (2008-03-02)"
#define MOD_CODEC   "(video) v4l"

static int verbose_flag    = TC_QUIET;
static int capability_flag = TC_CAP_RGB|TC_CAP_YUV;

#define MOD_PRE v4l
#include "import_def.h"

/*************************************************************************/

#define MAX_BUFFER 32

typedef struct v4lsource V4LSource;
struct v4lsource {
    int video_dev;

    int width;
    int height;
    int input;
    int format;

    struct video_mmap vid_mmap[MAX_BUFFER];
    int grab_buf_idx;
    int grab_buf_max;
    struct video_mbuf vid_mbuf;
    char *video_map;
    int grabbing_active;
    int have_new_frame;
    void *current_image;
    int totalframecount;
    int image_size;
    int image_pixels;
    int framecount;
    int fps_update_interval;
    double fps;
    double lasttime;

    int (*grab)(V4LSource *vs, uint8_t *buf, size_t bufsize);
    int (*close)(V4LSource *vs);
};

/*************************************************************************/

static struct v4lsource fg;

/*************************************************************************/

static int v4lsource_mmap_init(V4LSource *vs);
static int v4lsource_mmap_grab(V4LSource *vs, uint8_t *buffer, size_t bufsize);
static int v4lsource_mmap_close(V4LSource *vs);

#if 0
static int v4lsource_read_init(V4LSource *vs);
static int v4lsource_read_grab(V4LSource *vs, uint8_t *buffer, size_t bufsize);
#endif
static int v4lsource_read_close(V4LSource *vs);

/*************************************************************************/

#define RETURN_IF_ERROR(RET, MSG) do { \
     if (-1 == (RET)) { \
        tc_log_perror(MOD_NAME, (MSG)); \
        return TC_ERROR; \
    } \
} while (0)

/*************************************************************************/

#if 0
static int v4lsource_read_init(V4LSource *vs)
{
    if (verbose_flag)
        tc_log_info(MOD_NAME, "capture method: read");

    vs->grab  = v4lsource_read_grab;
    vs->close = v4lsource_read_close;
    return TC_OK;
}

static int v4lsource_read_grab(V4LSource *vs, uint8_t *buffer, size_t bufsize)
{
    return TC_ERROR;
}
#endif

static int v4lsource_read_close(V4LSource *vs)
{
    close(vs->video_dev);
    return TC_OK;
}

/*************************************************************************/

static int v4lsource_mmap_init(V4LSource *vs)
{
    int i, ret;

    if (verbose_flag)
        tc_log_info(MOD_NAME, "capture method: mmap");

    // retrieve buffer size and offsets
    ret = ioctl(vs->video_dev, VIDIOCGMBUF, &vs->vid_mbuf);
    RETURN_IF_ERROR(ret, "error requesting capture buffers");

    if (verbose)
        tc_log_info(MOD_NAME, "%d frame buffer%s available", 
                    vs->vid_mbuf.frames, (vs->vid_mbuf.frames > 0) ?"s" :"");
    vs->grab_buf_max = vs->vid_mbuf.frames;

    if (!vs->grab_buf_max) {
        tc_log_error(MOD_NAME, "no frame capture buffer(s) available");
        return TC_ERROR;
    }

    // map grabber memory onto user space
    vs->video_map = mmap(0, vs->vid_mbuf.size, PROT_READ|PROT_WRITE, MAP_SHARED, vs->video_dev, 0);
    if ((unsigned char *) -1 == (unsigned char *)vs->video_map) {
        tc_log_perror(MOD_NAME, "error mapping capture buffers in memory");
        return TC_ERROR;
    }

    // generate mmap records
    for (i = 0; i < vs->grab_buf_max; i++) {
        vs->vid_mmap[i].frame  = i;
        vs->vid_mmap[i].format = vs->format;
        vs->vid_mmap[i].width  = vs->width;
        vs->vid_mmap[i].height = vs->height;
    }

    // initiate capture for frames
    for (i = 1; i < vs->grab_buf_max + 1; i++) {
        ret = ioctl(vs->video_dev, VIDIOCMCAPTURE, &vs->vid_mmap[i % vs->grab_buf_max]);
        RETURN_IF_ERROR(ret, "error setting up a capture buffer");
    }

    vs->grab  = v4lsource_mmap_grab;
    vs->close = v4lsource_mmap_close;
    return TC_OK;
}


static int v4lsource_mmap_close(V4LSource *vs)
{
    // video device
    munmap(vs->video_map, vs->vid_mbuf.size);
    return v4lsource_read_close(vs);
}

static int v4lsource_mmap_grab(V4LSource *vs, uint8_t *buffer, size_t bufsize)
{
    int ret = 0;
    uint8_t *p, *planes[3] = { NULL, NULL, NULL };

    // advance grab-frame number
    vs->grab_buf_idx = ((vs->grab_buf_idx + 1) % vs->grab_buf_max);

    // wait for next image in the sequence to complete grabbing
    ret = ioctl(vs->video_dev, VIDIOCSYNC, &vs->vid_mmap[vs->grab_buf_idx]);
    RETURN_IF_ERROR(ret, "error waiting for new video frame (VIDIOCSYNC)");

    //copy frame
    p = &vs->video_map[vs->vid_mbuf.offsets[vs->grab_buf_idx]];

    switch (vs->format) {
      case VIDEO_PALETTE_RGB24:
      case VIDEO_PALETTE_YUV420P: /* fallback */
        ac_memcpy(buffer, p, vs->image_size);
        break;
      case VIDEO_PALETTE_YUV422:
        YUV_INIT_PLANES(planes, buffer, IMG_YUV_DEFAULT, vs->width, vs->height);
        ac_imgconvert(&p, IMG_YUY2, planes, IMG_YUV_DEFAULT, vs->width, vs->height);
        break;
    }

    vs->totalframecount++;

    // issue new grab command for this buffer
    ret = ioctl(vs->video_dev, VIDIOCMCAPTURE, &vs->vid_mmap[vs->grab_buf_idx]);
    RETURN_IF_ERROR(ret, "error acquiring new video frame (VIDIOCMCAPTURE)");
    
    return TC_OK;
}

/*************************************************************************/

static int v4lsource_init(const char *device, int w, int h, int fmt,
                           int verbose)
{
    struct video_capability capability;
    struct video_picture pict;
    int ret;

    fg.video_dev = open(device, O_RDWR);
    if (fg.video_dev == -1) {
        tc_log_perror(MOD_NAME, "error opening grab device");
        return TC_ERROR;
    }

    ret = ioctl(fg.video_dev, VIDIOCGCAP, &capability);
    RETURN_IF_ERROR(ret, "error quering capabilities (VIDIOCGCAP)");
   
    if (verbose_flag)
        tc_log_info(MOD_NAME, "capture device: %s", capability.name);
    if (!(capability.type & VID_TYPE_CAPTURE)) {
        tc_log_error(MOD_NAME, "device does NOT support capturing!");
        return TC_ERROR;
    }

    // picture parameter
    ret = ioctl(fg.video_dev, VIDIOCGPICT, &pict);
    RETURN_IF_ERROR(ret, "error getting picture parameters (VIDIOCGPICT)");

    // store variables
    fg.width           = w;
    fg.height          = h;
    fg.format          = fmt;
    // reset grab counter variables
    fg.grab_buf_idx    = 0;
    fg.totalframecount = 0;
    // calculate framebuffer size
    fg.image_pixels    = w * h;
    switch (fg.format) {
      case VIDEO_PALETTE_RGB24:
        fg.image_size = fg.image_pixels * 3;
        break;
      case VIDEO_PALETTE_YUV420P:
        fg.image_size = (fg.image_pixels * 3) / 2;
        break;
      case VIDEO_PALETTE_YUV422:
        fg.image_size = fg.image_pixels * 2; // XXX
        break;
    }

    if (fmt == VIDEO_PALETTE_RGB24) {
        pict.depth = 24;
    }
    pict.palette = fmt;

    ret = ioctl(fg.video_dev, VIDIOCSPICT, &pict);
    RETURN_IF_ERROR(ret, "error setting picture parameters (VIDIOCSPICT)");

    return v4lsource_mmap_init(&fg);
}

/*************************************************************************/

MOD_open
{
    int fmt = 0;

    if (verbose_flag)
        tc_log_warn(MOD_NAME, "this module is deprecated: "
                              "please use import_v4l2 instead");
    if (param->flag == TC_VIDEO) {
        // print out
        param->fd = NULL;

        switch (vob->im_v_codec) {
          case CODEC_RGB:
            fmt = VIDEO_PALETTE_RGB24;
            break;
          case CODEC_YUV422:
             fmt = VIDEO_PALETTE_YUV422;
            break;
          case CODEC_YUV:
            fmt = VIDEO_PALETTE_YUV420P;
            break;
        }
        
        if (v4lsource_init(vob->video_in_file,
                            vob->im_v_width, vob->im_v_height,
                            fmt, verbose_flag) < 0) {
            tc_log_error(MOD_NAME, "error grab init");
            return TC_ERROR;
        }
        return TC_OK;
    }
    return TC_ERROR;
}

MOD_decode
{
    if (param->flag == TC_VIDEO) {
        return fg.grab(&fg, param->buffer, param->size);
        return TC_OK;
    }
    return TC_ERROR;
}

MOD_close
{
    if (param->flag == TC_VIDEO) {
        fg.close(&fg);
        return TC_OK;
    }
    return TC_ERROR;
}

/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
