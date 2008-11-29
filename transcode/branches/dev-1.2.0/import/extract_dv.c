/*
 *  extract_dv.c
 *
 *  Copyright (C) Thomas Oestreich - June 2001
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
#include "tcinfo.h"

#include "ioaux.h"
#include "avilib/avilib.h"
#include "tc.h"

extern void import_exit(int ret);

#define DV_PAL_FRAME_SIZE  144000
#define DV_NTSC_FRAME_SIZE 120000


/* ------------------------------------------------------------
 *
 * dv extract thread
 *
 * magic: TC_MAGIC_AVI
 *        TC_MAGIC_RAW  <-- default
 *
 * ------------------------------------------------------------*/

void extract_dv(info_t *ipipe)
{

    avi_t *avifile=NULL;
    char *video;

    int key, error=0;

    long frames, bytes, n;


    /* ------------------------------------------------------------
     *
     * AVI
     *
     * ------------------------------------------------------------*/

    switch(ipipe->magic) {

    case TC_MAGIC_AVI:

	// scan file
	if (ipipe->nav_seek_file) {
	  if(NULL == (avifile = AVI_open_indexfd(ipipe->fd_in,0,ipipe->nav_seek_file))) {
	    AVI_print_error("AVI open");
	    import_exit(1);
	  }
	} else {
	  if(NULL == (avifile = AVI_open_fd(ipipe->fd_in,1))) {
	    AVI_print_error("AVI open");
	    import_exit(1);
	  }
	}

	// read video info;

	frames =  AVI_video_frames(avifile);
	if (ipipe->frame_limit[1] < frames)
	{
		frames=ipipe->frame_limit[1];
	}

	if(ipipe->verbose & TC_STATS)
	    tc_log_msg(__FILE__, "%ld video frames", frames);

	if((video = tc_zalloc(DV_PAL_FRAME_SIZE))==NULL) {
	    tc_log_error(__FILE__, "out of memory");
	    error=1;
	    break;
	}
	(int)AVI_set_video_position(avifile,ipipe->frame_limit[0]);
	for (n=ipipe->frame_limit[0]; n<=frames; ++n) {

	    // video
	    if((bytes = AVI_read_frame(avifile, video, &key))<0) {
		error=1;
		break;
	    }
	    if(tc_pwrite(ipipe->fd_out, video, bytes)!=bytes) {
	        error=1;
	    	break;
	    }
	}

	free(video);

	break;


	/* ------------------------------------------------------------
	 *
	 * RAW
	 *
         * RAW frame skipping code lifted from VLC, which lifted
         * (some of it) from libdv.
         *
         * ------------------------------------------------------------*/


    case TC_MAGIC_RAW:

    default:
    {
        unsigned char frame[DV_PAL_FRAME_SIZE];
        ssize_t size;

        if(ipipe->magic == TC_MAGIC_UNKNOWN)
	    tc_log_warn(__FILE__, "no file type specified, assuming %s",
			filetype(TC_MAGIC_RAW));

        /* Skip to the first desired frame. */
        for(n=0; n<ipipe->frame_limit[0]; ++n) {
            /* The first 32 bits contain the flag for PAL or NTSC frame. */
            if(tc_pread(ipipe->fd_in, frame, 4) != 4) {
                error = 1;
                return;
            }

            /* Read the rest of the frame. */
            size = frame[3] & 128 ? DV_PAL_FRAME_SIZE : DV_NTSC_FRAME_SIZE;
            if (tc_pread(ipipe->fd_in, frame, size-4) != size-4) {
                error = 1;
                return;
            }
        }

        /* Read the right number of frames. */
        for(n=ipipe->frame_limit[0]; n<=ipipe->frame_limit[1]; ++n) {
            /* The first 32 bits contain the flag for PAL or NTSC frame. */
            if(tc_pread(ipipe->fd_in, frame, 4)!=4) {
                error = 1;
                return;
            }

            /* Read the rest of the frame. */
            size = frame[3] & 128 ? DV_PAL_FRAME_SIZE : DV_NTSC_FRAME_SIZE;
            if(tc_pread(ipipe->fd_in, &frame[4], size-4)!=size-4) {
                error = 1;
                return;
            }

            /* Write it out. */
            if(tc_pwrite(ipipe->fd_out, frame, size)!=size) {
                error = 1;
                return;
            }
        }
        break;
    }
    }
}

