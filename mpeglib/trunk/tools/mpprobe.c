/*
 *  Copyright (C) 2005 Francesco Romani <fromani@gmail.com>
 * 
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 *  copies of the Software, and to permit persons to whom the Software is 
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in 
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "mpeglib.h"

#define PROGNAME        "mpprobe"

extern char *optarg;
extern int optind, opterr, optopt;

int
codec_str2int(const char *codecstr) {
    if(!strcmp(codecstr, "audio/mpeg")) {
        return 0x50; /* mp3 not allowd in MPEG files (right?) */
    } else if(!strcmp(codecstr, "audio/ac3")) {
        return 0x2000;
    } else if(!strcmp(codecstr, "audio/lpcm")) {
        return 0x10001;
    } else if(!strcmp(codecstr, "audio/pcm")) {
        /* we can have this? */
        return 0x1;
    }
    return 0; /* fallback */
}

int
fps2frc(const mpeg_fraction_t *fps) {
    if(fps->num == 24000 && fps->den == 1001) {
        return 1;
    } else if(fps->num == 24 && fps->den == 1) {
        return 2;
    } else if(fps->num == 25 && fps->den == 1) {
        return 3;
    } else if(fps->num == 30000 && fps->den == 1001) {
        return 4;
    } else if(fps->num == 30 && fps->den == 1) {
        return 5;
    } else if(fps->num == 50 && fps->den == 1) {
        return 6;
    } else if(fps->num == 60 && fps->den == 1) {
        return 7;
    } else if(fps->num == 60000 && fps->den == 1001) {
        return 8;
    }

    return 0; /* fallback */
}

void
version(void) {
    fprintf(stderr, "%s (MPEGlib v\"%s\") (C) 2005 Francesco Romani"
            " (et al)\n\n", PROGNAME, VERSION);
}

void 
usage(void) {
    version();
    fprintf(stderr, PROGNAME" probe mpeg files and print "
            "information on the standard output.\n"
            "syntax and output are intentionally similar (or identical)\n"
            "to transcode's tcprobe.\n"
            "It's used mainly for debug of MPEGlib\n\n");
    fprintf(stderr, "Usage: %s [options] [-]\n", PROGNAME);
    fprintf(stderr, "\t-i name    input file name [stdin]\n");
    fprintf(stderr, "\t-d         emit debug messages from MPEGlib [no]\n");
    fprintf(stderr, "\t-v         print version\n");
    fprintf(stderr, "\t-h         print this message\n");
    fflush(stderr);
}

#define VID_CANONICAL(pvid) \
    (((pvid)->width == 720) && ((pvid)->height == 576))

#define AUD_CANONICAL(paud) \
    (((paud)->sample_rate == 48000) && (((paud)->sample_size == 16) && ((paud)->channels == 2)))

#define FPS_CANONICAL(pfps) \
    (((pfps)->num) == 25 && ((pfps)->den == 1))

#define FRAME_TIME(pfps) \
    ((int)(1.0f/((double)(pfps)->num / (double)(pfps)->den)*1000))
    

int 
main(int argc, char *argv[]) {
    /* 
     * step 1: declare the basic data structure used by MPEGlib
     */
    int use_stdin = TRUE, be_quiet = TRUE; /* options */
    int num = 0, i = 0, ch = 0;
    mpeg_t *MPEG = NULL;
    mpeg_file_t *MFILE = NULL;
    const mpeg_stream_t *mps = NULL;
    const char *fname = NULL;
    int astream_num = 0, codec_id = 0;
    
    while((ch = getopt(argc, argv, "i:dvh")) != -1) {
        switch(ch) {
        case 'i':
            if(!optarg || ! strlen(optarg)) {
                usage();
                exit(EXIT_FAILURE);
            }
            use_stdin = FALSE;
            fname = optarg;
            break;
        case 'd':
            be_quiet = FALSE;
            break;
        case 'v':
            version();
            exit(EXIT_SUCCESS);
            break;
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
            break;
        default:
            usage();
            exit(EXIT_FAILURE);
            break;
        }
    }

    /* last check, be compatible with tcprobe */
    if(argc == 1) {
        usage();
        exit(EXIT_FAILURE);
    }

    if(be_quiet) {
        mpeg_set_logging(mpeg_log_null, stderr);
    }
    
    /* 
     * step 2: open data source.
     * mpeg_file_open is the default FILE wrapper used by MPEGlib and will
     * be always avalaible. It acts just like a plain old fopen() and support
     * only local files
     */
    if(use_stdin) {
        MFILE = mpeg_file_open_link(stdin);
    } else {
        MFILE = mpeg_file_open(fname, "r");
        if(!MFILE) {
            fprintf(stderr, "unable to open: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    /*
     * step 3: create a new MPEG descriptor. We need basically a pointer
     * to a FILE wrapper (mpeg_file_t) and a flag which indicates the kind
     * of MPEG file to be open. 
     */
    MPEG = mpeg_open(MPEG_TYPE_ANY, MFILE, 
             MPEG_DEFAULT_FLAGS|MPEG_FLAG_TCORDER, NULL);
    if(!MPEG) {
        mpeg_file_close(MFILE);
        fprintf(stderr, "mpeg_open() failed (maybe you don't used -i?)\n");
        exit(1);
    }

    fprintf(stderr, "[%s] MPEG %s stream (%s)\n", "tcprobe", 
        (MPEG->type == MPEG_TYPE_PS) ?"program" :"elementary",
        (MPEG->type == MPEG_TYPE_PS) ?"PS" :"ES");
    printf("[%s] summary for %s, (*) = not default, 0 = not detected\n",
            "tcprobe", fname);
        
    /*
     * step 4a: now (and above) we print what we have found. 
     * How many streams out there?
     */
    num = mpeg_get_stream_number(MPEG);
    for(i = 0; i < num; i++) {
        /*
         * step 4b: get a descriptor for stream #i. This is obviously a 
         * constant reference. We do not free() them.
         */
        mps = mpeg_get_stream_info(MPEG, i);
        if(mps != NULL) {
            if(mps->stream_type == MPEG_STREAM_TYPE_VIDEO) {
                printf("import frame size: -g %ix%i [720x576]%s\n",
                    mps->video.width, mps->video.height,
                    VID_CANONICAL(&(mps->video)) ?"" :" (*)");
                printf("     aspect ratio: %i:%i \n", 
                        mps->video.aspect.num,
                        mps->video.aspect.den);
                printf("       frame rate: -f %.3f [25.000] frc=%i%s\n",
                    (double)mps->video.frame_rate.num/
                    (double)mps->video.frame_rate.den,
                    fps2frc(&(mps->video.frame_rate)),
                    FPS_CANONICAL(&(mps->video.frame_rate)) ?"" :" (*)");
                printf("                   PTS=%3.4f, frame_time=%i ms, "
                       "bitrate=%i kbps\n", 
		       (double)mps->common.start_time/10000,
                       FRAME_TIME(&(mps->video.frame_rate)), 
                       mps->common.bit_rate);
            } else if(mps->stream_type == MPEG_STREAM_TYPE_AUDIO) {
                codec_id = codec_str2int(mps->common.codec);
                printf("      audio track: -a %i [0] -e %i,%i,%i "
                       "[48000,16,2]%s -n 0x%x [0x2000]%s\n"
                       "                   bitrate=%i kbps\n",
                    astream_num++, mps->audio.sample_rate, 
                    mps->audio.sample_size, mps->audio.channels, 
            AUD_CANONICAL(&(mps->audio)) ?"" :" (*)", 
            codec_id, (codec_id == 0x2000) ?"" :" (*)", 
            mps->common.bit_rate);
            }
        } else {
            printf("(%s) WARNING: can't get stream information for "
                   "stream #%02i\n",  __FILE__, i);
        }
    }
    
    /*
     * step 5: finalize the MPEG descriptor. Weird things can happen if we do not
     * follow the REVERSE order of finalization strictly.
     */
    mpeg_close(MPEG);

    /*
     * step 5: closing the FILE wrapper
     */
    mpeg_file_close(MFILE);

    /*
     * all done! :)
     */
    return 0;
}
