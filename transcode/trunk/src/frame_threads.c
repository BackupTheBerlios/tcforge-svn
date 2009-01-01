/*
 *  frame_threads.c -- implementation of transcode multithreaded filter
 *                     processing code.
 *
 *  Copyright (C) Thomas Oestreich - June 2001
 *  updates and partial rewrite:
 *  Copyright (C) Francesco Romani - October 2007
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

#include <pthread.h>

#include "transcode.h"
#include "encoder-common.h"
#include "framebuffer.h"
#include "video_trans.h"
#include "audio_trans.h"
#include "decoder.h"
#include "filter.h"

#include "frame_threads.h"

/*************************************************************************/

typedef struct tcframethreaddata_ TCFrameThreadData;
struct tcframethreaddata_ {
    pthread_t threads[TC_FRAME_THREADS_MAX];    /* thread pool           */
    int count;                                  /* how many workers?     */

    pthread_mutex_t lock;
    volatile int running;                       /* _pool_ running flag   */
};

TCFrameThreadData audio_threads = {
    .count   = 0,
    .lock    = PTHREAD_MUTEX_INITIALIZER,
    .running = TC_FALSE,
};

TCFrameThreadData video_threads = {
    .count   = 0,
    .lock    = PTHREAD_MUTEX_INITIALIZER,
    .running = TC_FALSE,
};

/*************************************************************************/

/*
 * tc_frame_threads_stop (Thread safe):
 * set the stop flag for a thread pool.
 *
 * Parameters:
 *      data: thread pool descriptor.
 * Return Value:
 *      None.
 */
static void tc_frame_threads_stop(TCFrameThreadData *data)
{
    pthread_mutex_lock(&data->lock);
    data->running = TC_FALSE;
    pthread_mutex_unlock(&data->lock);
}

/*
 * tc_frame_threads_are_active (Thread safe):
 * verify if there is a pending stop request for given thread pool.
 *
 * Parameters:
 *      data: thread pool descriptor.
 * Return Value:
 *      !0: there is pending pool stop request.
 *       0: otherwise.
 */
static int tc_frame_threads_are_active(TCFrameThreadData *data)
{
    int ret;
    pthread_mutex_lock(&data->lock);
    ret = data->running;
    pthread_mutex_unlock(&data->lock);
    return ret;
}

/*
 * stop_requested: verify if the pool thread has to stop.
 * First thread in the pool notifying the core has to stop must
 * set the flag to notify the others.
 *
 * Parameters:
 *      data: thread pool descriptor.
 * Return Value:
 *      !0: thread pool has to halt as soon as is possible.
 *       0: thread pool can continue to run.
 */
static int stop_requested(TCFrameThreadData *data)
{
    return (!tc_running() || !tc_frame_threads_are_active(data));
}


/*************************************************************************/
/*         frame processing core threads                                 */
/*************************************************************************/


#define DUP_vptr_if_cloned(vptr) do { \
    if(vptr->attributes & TC_FRAME_IS_CLONED) { \
        vframe_list_t *tmptr = vframe_dup(vptr); \
        \
        /* ptr was successfully cloned */ \
        /* delete clone flag */ \
        tmptr->attributes &= ~TC_FRAME_IS_CLONED; \
        vptr->attributes  &= ~TC_FRAME_IS_CLONED; \
        \
        /* set info for filters */ \
        tmptr->attributes |= TC_FRAME_WAS_CLONED; \
        \
        /* this frame is to be processed _after_ the current one */ \
        /* so put it back into the queue */ \
        vframe_push_next(tmptr, TC_FRAME_WAIT); \
        \
    } \
} while (0)



#define DUP_aptr_if_cloned(aptr) do { \
    if(aptr->attributes & TC_FRAME_IS_CLONED) {  \
        aframe_list_t *tmptr = aframe_dup(aptr);  \
        \
        /* ptr was successfully cloned */ \
        \
        /* delete clone flag */ \
        tmptr->attributes &= ~TC_FRAME_IS_CLONED;  \
        aptr->attributes  &= ~TC_FRAME_IS_CLONED;  \
        \
        /* set info for filters */ \
        tmptr->attributes |= TC_FRAME_WAS_CLONED;  \
        \
        /* this frame is to be processed _after_ the current one */ \
        /* so put it back into the queue */ \
        aframe_push_next(tmptr, TC_FRAME_WAIT);  \
        \
    }  \
} while (0)


#define SET_STOP_FLAG(DATAP, MSG) do { \
    if (verbose >= TC_CLEANUP) \
        tc_log_msg(__FILE__, "%s", (MSG)); \
    tc_frame_threads_stop((DATAP)); \
} while (0)

static void *process_video_frame(void *_vob)
{
    static int res = 0; // XXX
    vframe_list_t *ptr = NULL;
    vob_t *vob = _vob;

    while (!stop_requested(&video_threads)) {
        ptr = vframe_reserve();
        if (ptr == NULL) {
            SET_STOP_FLAG(&video_threads, "video interrupted: exiting!");
            res = 1;
            break;
        }
        if (ptr->attributes & TC_FRAME_IS_END_OF_STREAM) {
            SET_STOP_FLAG(&video_threads, "video stream end: marking!");
        }

        if (ptr->attributes & TC_FRAME_IS_SKIPPED) {
            vframe_remove(ptr);  /* release frame buffer memory */
            continue;
        }

        if (TC_FRAME_NEED_PROCESSING(ptr)) {
            // external plugin pre-processing
            ptr->tag = TC_VIDEO|TC_PRE_M_PROCESS;
            tc_filter_process((frame_list_t *)ptr);

            if (ptr->attributes & TC_FRAME_IS_SKIPPED) {
                vframe_remove(ptr);  /* release frame buffer memory */
                continue;
            }

            // clone if the filter told us to do so.
            DUP_vptr_if_cloned(ptr);

            // internal processing of video
            ptr->tag = TC_VIDEO;
            process_vid_frame(vob, ptr);

            // external plugin post-processing
            ptr->tag = TC_VIDEO|TC_POST_M_PROCESS;
            tc_filter_process((frame_list_t *)ptr);

            if (ptr->attributes & TC_FRAME_IS_SKIPPED) {
                vframe_remove(ptr);  /* release frame buffer memory */
                continue;
            }
        }

        vframe_push_next(ptr, TC_FRAME_READY);
    }
    if (verbose >= TC_CLEANUP)
        tc_log_msg(__FILE__, "video stream end: got, so exiting!");

    pthread_exit(&res);
    return NULL;
}


static void *process_audio_frame(void *_vob)
{
    static int res = 0; // XXX
    aframe_list_t *ptr = NULL;
    vob_t *vob = _vob;

    while (!stop_requested(&audio_threads)) {
        ptr = aframe_reserve();
        if (ptr == NULL) {
            SET_STOP_FLAG(&audio_threads, "audio interrupted: exiting!");
            break;
            res = 1;
        }
        if (ptr->attributes & TC_FRAME_IS_END_OF_STREAM) {
            SET_STOP_FLAG(&audio_threads, "audio stream end: marking!");
        }

        if (ptr->attributes & TC_FRAME_IS_SKIPPED) {
            aframe_remove(ptr);  /* release frame buffer memory */
            continue;
        }

        if (TC_FRAME_NEED_PROCESSING(ptr)) {
            // external plugin pre-processing
            ptr->tag = TC_AUDIO|TC_PRE_M_PROCESS;
            tc_filter_process((frame_list_t *)ptr);

            DUP_aptr_if_cloned(ptr);

            if (ptr->attributes & TC_FRAME_IS_SKIPPED) {
                aframe_remove(ptr);  /* release frame buffer memory */
                continue;
            }

            // internal processing of audio
            ptr->tag = TC_AUDIO;
            process_aud_frame(vob, ptr);

            // external plugin post-processing
            ptr->tag = TC_AUDIO|TC_POST_M_PROCESS;
            tc_filter_process((frame_list_t *)ptr);

            if (ptr->attributes & TC_FRAME_IS_SKIPPED) {
                aframe_remove(ptr);  /* release frame buffer memory */
                continue;
            }
        }

        aframe_push_next(ptr, TC_FRAME_READY);
    }
    if (verbose >= TC_CLEANUP)
        tc_log_msg(__FILE__, "audio stream end: got, so exiting!");

    pthread_exit(&res);
    return NULL;
}

/*************************************************************************/


int tc_frame_threads_have_video_workers(void)
{
    return (video_threads.count > 0);
}

int tc_frame_threads_have_audio_workers(void)
{
    return (audio_threads.count > 0);
}


void tc_frame_threads_init(vob_t *vob, int vworkers, int aworkers)
{
    int n = 0;

    if (vworkers > 0 && !video_threads.running) {
        video_threads.count   = vworkers;
        video_threads.running = TC_TRUE; /* enforce, needed when restarting */

        if (verbose >= TC_DEBUG)
            tc_log_info(__FILE__, "starting %i video frame"
                                 " processing thread(s)", vworkers);

        // start the thread pool
        for (n = 0; n < vworkers; n++) {
            if (pthread_create(&video_threads.threads[n], NULL,
                               process_video_frame, vob) != 0)
                tc_error("failed to start video frame processing thread");
        }
    }

    if (aworkers > 0 && !audio_threads.running) {
        audio_threads.count   = aworkers;
        audio_threads.running = TC_TRUE; /* enforce, needed when restarting */

        if (verbose >= TC_DEBUG)
            tc_log_info(__FILE__, "starting %i audio frame"
                                 " processing thread(s)", aworkers);

        // start the thread pool
        for (n = 0; n < aworkers; n++) {
            if (pthread_create(&audio_threads.threads[n], NULL,
                               process_audio_frame, vob) != 0)
                tc_error("failed to start audio frame processing thread");
        }
    }
    return;
}

void tc_frame_threads_close(void)
{
    void *status = NULL;
    int n = 0;

    if (audio_threads.count > 0) {
        tc_frame_threads_stop(&audio_threads);
        if (verbose >= TC_CLEANUP)
            tc_log_msg(__FILE__, "wait for %i audio frame processing threads",
                       audio_threads.count);
        for (n = 0; n < audio_threads.count; n++)
            pthread_join(audio_threads.threads[n], &status);
        if (verbose >= TC_CLEANUP)
            tc_log_msg(__FILE__, "audio frame processing threads canceled");
    }

    if (video_threads.count > 0) {
        tc_frame_threads_stop(&video_threads);
        if (verbose >= TC_CLEANUP)
            tc_log_msg(__FILE__, "wait for %i video frame processing threads",
                       video_threads.count);
        for (n = 0; n < video_threads.count; n++)
            pthread_join(video_threads.threads[n], &status);
        if (verbose >= TC_CLEANUP)
            tc_log_msg(__FILE__, "video frame processing threads canceled");
    }
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
