

#ifndef _TC_V4L_COMMON_H
#define _TC_V4L_COMMON_H 1

#define NAME "vcr"

//#define DEBUG


// video

#define VCR_CODEC VIDEO_PALETTE_YUV422 //VIDEO_PALETTE_RGB24;
#define VCR_DEPTH 2                    //3

#define VCR_WIDTH  768 //360
#define VCR_HEIGHT 576 //288

#define SIZE_AUD_FRAME   7680   // 48000Hz

// audio
#define SAMPLE_RATE  48000
#define BITS            16
#define CHANNELS         2

#define DECODE_AHEAD    25

#define FRAME_FIRST      0
#define FRAME_ALL    65536

//get rescaled frame parameters or global parameter infos
unsigned int get_width(void);
unsigned int get_height(void);
unsigned int get_pipe_mode(void);
unsigned int get_decode_ahead(void);

#define MOV_T  144737919
#define AVI_T 1179011410

#define CHAN_TV        0
#define CHAN_VCR       1

#define CAP_STATUS_INIT     1
#define CAP_STATUS_VCAPTURE 2
#define CAP_STATUS_ACAPTURE 4
#define CAP_STATUS_WRITE    8
#define CAP_STATUS_LOOP    16

// all times in usec
#define FRAME_UTIME   40000
#define  WAIT_UTIME       0

#define MAX_FRAMES    10000

extern long int a_startsec, a_startusec;
extern long int v_startsec, v_startusec;

extern int v_source;

double v4l_counter_init(void);
void v4l_counter_print(char *s, long _n, double ini, double *last);

#endif
