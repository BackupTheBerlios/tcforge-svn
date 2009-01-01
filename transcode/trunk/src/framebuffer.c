/*
 * framebuffer.c -- audio/video frame ringbuffers, reloaded. Again.
 * (C) 2005-2008 - Francesco Romani <fromani -at- gmail -dot- com>
 * Based on code
 * (C) 2001-2006 - Thomas Oestreich.
 *
 * This file is part of transcode, a video stream processing tool.
 *
 * transcode is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * transcode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pthread.h>

#include "transcode.h"
#include "tc_defaults.h"
#include "framebuffer.h"
#include "encoder-common.h"

#include "libtc/tcframes.h"
#include "libtc/ratiocodes.h"

/* unit testing needs this */
#ifndef FBUF_TEST
#define STATIC  static
#else
#define STATIC 
#endif

/* layer ids for debugging/human consumption, from inner to outer */
#define FPOOL_NAME  "framepool"
#define FRING_NAME  "framering"
#define FRBUF_NAME  "framebuffer"
#define MOD_NAME    FRBUF_NAME

#define PTHREAD_ID  ((long)pthread_self())

/*
 * Summary:
 * This code acts as generic ringbuffer implementation, with
 * specializations for main (audio and video) ringbufffers
 * in order to cope legacy constraints from 1.0.x series.
 * It replaces former src/{audio,video}_buffer.c in (hopefully!)
 * a more generic, clean, maintanable, compact way.
 * 
 * Please note that there is *still* some other ringbuffer
 * scatthered through codebase (subtitle buffer,d emux buffers,
 * possibly more). They will be merged lately or will be dropped
 * or reworked.
 */

/*************************************************************************/
/* frame processing stages. `Locked' stage is now ignored.               */
/*************************************************************************/

#define TC_FRAME_STAGE_ID(ST)   ((ST) + 1)
#define TC_FRAME_STAGE_ST(ID)   ((ID) - 1)
#define TC_FRAME_STAGE_NUM      (TC_FRAME_STAGE_ID(TC_FRAME_READY) + 1)

struct stage {
    TCFrameStatus   status;
    const char      *name;
    int             broadcast;
};

static const struct stage frame_stages[] = {
    { TC_FRAME_NULL,    "null",     TC_FALSE },
    { TC_FRAME_EMPTY,   "empty",    TC_FALSE },
    { TC_FRAME_WAIT,    "wait",     TC_TRUE  },
    { TC_FRAME_LOCKED,  "locked",   TC_TRUE  }, /* legacy */
    { TC_FRAME_READY,   "ready",    TC_FALSE },
};

STATIC const char *frame_status_name(TCFrameStatus S)
{
    int i = TC_FRAME_STAGE_ID(S);
    return frame_stages[i].name;
}

/*************************************************************************/
/* frame spec(ification)s. How big those framebuffer should be?          */
/*************************************************************************/

/* 
 * Specs used internally. I don't export this structure directly
 * because I want to be free to change it if needed
 */
static TCFrameSpecs tc_specs = {
    /* Largest supported values, to ensure the buffer is always big enough
     * (see FIXME in tc_framebuffer_set_specs()) */
    .frc      = 3,  // PAL, why not
    .width    = TC_MAX_V_FRAME_WIDTH,
    .height   = TC_MAX_V_FRAME_HEIGHT,
    .format   = TC_CODEC_RGB,
    .rate     = RATE,
    .channels = CHANNELS,
    .bits     = BITS,
    .samples  = 48000.0,
};

const TCFrameSpecs *tc_framebuffer_get_specs(void)
{
    return &tc_specs;
}

/* 
 * we compute (ahead of time) samples value for later usage.
 */
void tc_framebuffer_set_specs(const TCFrameSpecs *specs)
{
    /* silently ignore NULL specs */
    if (specs != NULL) {
        double fps;

        /* raw copy first */
        ac_memcpy(&tc_specs, specs, sizeof(TCFrameSpecs));

        /* restore width/height/bpp
         * (FIXME: temp until we have a way to know the max size that will
         *         be used through the decode/process/encode chain; without
         *         this, -V yuv420p -y raw -F rgb (e.g.) crashes with a
         *         buffer overrun)
         */
        tc_specs.width  = TC_MAX_V_FRAME_WIDTH;
        tc_specs.height = TC_MAX_V_FRAME_HEIGHT;
        tc_specs.format = TC_CODEC_RGB;

        /* then deduct missing parameters */
        if (tc_frc_code_to_value(tc_specs.frc, &fps) == TC_NULL_MATCH) {
            fps = 1.0; /* sane, very worst case value */
        }
/*        tc_specs.samples = (double)tc_specs.rate/fps; */
        tc_specs.samples = (double)tc_specs.rate;
        /* 
         * FIXME
         * ok, so we use a MUCH larger buffer (big enough to store 1 *second*
         * of raw audio, not 1 *frame*) than needed for reasons similar as 
         * seen for above video.
         * Most notably, this helps in keeping buffers large enough to be
         * suitable for encoder flush (see encode_lame.c first).
         */
    }
}

/*************************************************************************/
/* Frame allocation/disposal helpers; those are effectively just thin    */
/* wrappers around libtc facilities. They are just interface adapters.   */
/*************************************************************************/

#define TCFRAMEPTR_IS_NULL(tcf)    (tcf.generic == NULL)

static TCFramePtr tc_video_alloc(const TCFrameSpecs *specs)
{
    TCFramePtr frame;
    frame.video = tc_new_video_frame(specs->width, specs->height,
                                      specs->format, TC_TRUE);
    return frame;
}

static TCFramePtr tc_audio_alloc(const TCFrameSpecs *specs)
{
    TCFramePtr frame;
    frame.audio = tc_new_audio_frame(specs->samples, specs->channels,
                                     specs->bits);
    return frame;
}

static void tc_video_free(TCFramePtr frame)
{
    tc_del_video_frame(frame.video);
}

static void tc_audio_free(TCFramePtr frame)
{
    tc_del_audio_frame(frame.audio);
}

vframe_list_t *vframe_alloc_single(void)
{
    return tc_new_video_frame(tc_specs.width, tc_specs.height,
                              tc_specs.format, TC_TRUE);
}

aframe_list_t *aframe_alloc_single(void)
{
    return tc_new_audio_frame(tc_specs.samples, tc_specs.channels,
                              tc_specs.bits);
}

/*************************************************************************/

typedef struct tcframefifo_ TCFrameFifo;
struct tcframefifo_ {
    TCFramePtr  *frames;
    int         size;
    int         num;
    int         first;
    int         last;

    int         *avalaible;
    int         (*len)(TCFrameFifo *F);
    TCFramePtr  (*get)(TCFrameFifo *F);
    int         (*put)(TCFrameFifo *F, TCFramePtr ptr);
};

STATIC void tc_frame_fifo_dump_status(TCFrameFifo *F, const char *tag)
{
    int i = 0;
    tc_log_msg(FPOOL_NAME, "(%s|fifo|%s) size=%i num=%i first=%i last=%i",
               tag, (F->avalaible) ?"sorted" :"plain",
               F->size, F->num, F->first, F->last);

    for (i = 0; i < F->size; i++) {
        frame_list_t *ptr = F->frames[i].generic;
        int avail = (F->avalaible) ?F->avalaible[i] :-1;

        tc_log_msg(FPOOL_NAME,
                   "(%s|fifo) #%i ptr=%p (bufid=%i|status=%s) avail=%i",
                   tag, i, ptr,
                   (ptr) ?ptr->bufid :-1,
                   (ptr) ?frame_status_name(ptr->status) :"unknown",
                   avail);
    }
}

STATIC void tc_frame_fifo_del(TCFrameFifo *F)
{
    tc_free(F);
}

STATIC int tc_frame_fifo_empty(TCFrameFifo *F)
{
    return (F->len(F) == 0) ?TC_TRUE :TC_FALSE;
}

STATIC int tc_frame_fifo_size(TCFrameFifo *F)
{
    return F->num;
}

static int fifo_len(TCFrameFifo *F)
{
    return F->num;
}

static TCFramePtr fifo_get(TCFrameFifo *F)
{
    TCFramePtr ptr = { .generic = NULL };
    if (F->num > 0) {
        ptr = F->frames[F->first];
        F->first = (F->first + 1) % F->size;
        F->num--;
    }
    return ptr;
}

static int fifo_put(TCFrameFifo *F, TCFramePtr ptr)
{
    int ret = 0;
    if (F->num < F->size) {
        F->frames[F->last] = ptr;
        F->last            = (F->last + 1) % F->size;
        F->num++;
        ret = 1;
    }
    return ret;
}

STATIC TCFramePtr tc_frame_fifo_get(TCFrameFifo *F)
{
    return F->get(F);
}

STATIC int tc_frame_fifo_put(TCFrameFifo *F, TCFramePtr ptr)
{
    return F->put(F, ptr);
}

static int fifo_len_sorted(TCFrameFifo *F)
{
    return F->avalaible[F->first];
}

static TCFramePtr fifo_get_sorted(TCFrameFifo *F)
{
    TCFramePtr ptr = { .generic = NULL };
    if (F->avalaible[F->first]) {
        ptr = F->frames[F->first];
        F->avalaible[F->first] = TC_FALSE;   /* consume it... */
        F->first = (F->first + 1) % F->size; /* ..and wait next one */
        F->num--;
    }
    return ptr;
}

static int fifo_put_sorted(TCFrameFifo *F, TCFramePtr ptr)
{
    int ret = 0;
    F->frames[ptr.generic->bufid] = ptr;
    F->avalaible[ptr.generic->bufid] = TC_TRUE;
    F->num++;
    /* special case need special handling */
    if (ptr.generic->attributes & TC_FRAME_WAS_CLONED) {
        /* roll back, we want the same frame again */
        F->first = ptr.generic->bufid;
    }
    if (ptr.generic->bufid == F->first) {
        ret = 1;
    }
    return ret;
}

STATIC TCFrameFifo *tc_frame_fifo_new(int size, int sorted)
{
    TCFrameFifo *F = NULL;
    uint8_t *mem = NULL;
    size_t memsize = sizeof(TCFrameFifo) + (sizeof(TCFramePtr) * size);
    size_t xsize = (sorted) ?(sizeof(int) * size) :0;

    mem = tc_zalloc(memsize + xsize);
    if (mem) {
        F          = (TCFrameFifo *)mem;
        F->frames  = (TCFramePtr *)(mem + sizeof(TCFrameFifo));
        F->size    = size;
        if (sorted) {
            F->get       = fifo_get_sorted;
            F->put       = fifo_put_sorted;
            F->len       = fifo_len_sorted;
            F->avalaible = (int*)(mem + memsize);
        } else {
            F->get       = fifo_get;
            F->put       = fifo_put;
            F->len       = fifo_len;
            F->avalaible = NULL;
        }
    }
    return F;
}

/*************************************************************************/

typedef struct tcframepool_ TCFramePool;
struct tcframepool_ {
    const char      *ptag;      /* given from ringbuffer */
    const char      *tag;

    pthread_mutex_t lock;
    pthread_cond_t  empty;
    int             waiting;    /* how many thread blocked here? */

    TCFrameFifo     *fifo;
};

STATIC int tc_frame_pool_init(TCFramePool *P, int size, int sorted,
                              const char *tag, const char *ptag)
{
    int ret = TC_ERROR;
    if (P) {
        pthread_mutex_init(&P->lock, NULL);
        pthread_cond_init(&P->empty, NULL);

        P->ptag     = (ptag) ?ptag :"unknown";
        P->tag      = (tag)  ?tag  :"unknown";
        P->waiting  = 0;
        P->fifo     = tc_frame_fifo_new(size, sorted);
        if (P->fifo) {
            ret = TC_OK;
        }
    }
    return ret;
}

STATIC int tc_frame_pool_fini(TCFramePool *P)
{
    if (P && P->fifo) {
        tc_frame_fifo_del(P->fifo);
        P->fifo = NULL;
    }
    return TC_OK;
}

STATIC void tc_frame_pool_dump_status(TCFramePool *P)
{
    tc_log_msg(FPOOL_NAME, "(%s|%s) waiting=%i fifo status:",
               P->ptag, P->tag, P->waiting);
    tc_frame_fifo_dump_status(P->fifo, P->tag);
}

STATIC void tc_frame_pool_put_frame(TCFramePool *P, TCFramePtr ptr)
{
    int wakeup = 0;
    pthread_mutex_lock(&P->lock);
    wakeup = tc_frame_fifo_put(P->fifo, ptr);

    if (verbose >= TC_FLIST)
        tc_log_msg(FPOOL_NAME,
                   "(put_frame|%s|%s|%li) wakeup=%i waiting=%i",
                    P->tag, P->ptag, PTHREAD_ID, wakeup, P->waiting);

    if (P->waiting && wakeup) {
        pthread_cond_signal(&P->empty);
    }

    pthread_mutex_unlock(&P->lock);
}

STATIC TCFramePtr tc_frame_pool_get_frame(TCFramePool *P)
{
    int interrupted = TC_FALSE;

    TCFramePtr ptr = { .generic = NULL };
    pthread_mutex_lock(&P->lock);

    if (verbose >= TC_FLIST)
        tc_log_msg(FPOOL_NAME,
                   "(get_frame|%s|%s|%li) requesting frame",
                   P->tag, P->ptag, PTHREAD_ID);

    P->waiting++;
    while (!interrupted && tc_frame_fifo_empty(P->fifo)) {
        if (verbose >= TC_FLIST)
            tc_log_msg(FPOOL_NAME,
                       "(get_frame|%s|%s|%li) blocking (no frames in pool)",
                       P->tag, P->ptag, PTHREAD_ID);

        pthread_cond_wait(&P->empty, &P->lock);

        if (verbose >= TC_FLIST)
            tc_log_msg(FPOOL_NAME,
                       "(get_frame|%s|%s|%li) UNblocking",
                       P->tag, P->ptag, PTHREAD_ID);

        interrupted = !tc_running();
    }
    P->waiting--;

    if (!interrupted) {
        ptr = tc_frame_fifo_get(P->fifo);
    }

    if (verbose >= TC_FLIST)
        tc_log_msg(FPOOL_NAME,
                   "(got_frame|%s|%s|%li) frame=%p #%i",
                   P->tag, P->ptag, PTHREAD_ID,
                   ptr.generic,
                   (ptr.generic) ?ptr.generic->bufid :(-1));

    pthread_mutex_unlock(&P->lock);
    return ptr;
}

/* to be used ONLY in safe places like init, fini, flush */
STATIC TCFramePtr tc_frame_pool_pull_frame(TCFramePool *P)
{
    return tc_frame_fifo_get(P->fifo);
}

/* ditto */
STATIC void tc_frame_pool_push_frame(TCFramePool *P, TCFramePtr ptr)
{
    tc_frame_fifo_put(P->fifo, ptr);
}

STATIC void tc_frame_pool_wakeup(TCFramePool *P, int broadcast)
{
    pthread_mutex_lock(&P->lock);
    if (broadcast) {
        pthread_cond_broadcast(&P->empty);
    } else {
        pthread_cond_signal(&P->empty);
    }
    pthread_mutex_unlock(&P->lock);
}

/*************************************************************************
 * Layered, custom allocator/disposer for ringbuffer structures.
 * The idea is to simplify (from ringbuffer viewpoint!) frame
 * allocation/disposal and to make it as much generic as is possible
 * (avoid if()s and so on).
 *************************************************************************/

typedef TCFramePtr (*TCFrameAllocFn)(const TCFrameSpecs *);
typedef void       (*TCFrameFreeFn)(TCFramePtr);

/*************************************************************************/

typedef struct tcframering_ TCFrameRing;
struct tcframering_ {
    const char          *tag;

    TCFramePtr          *frames; /* main frame references */
    int                 size;    /* how many of them? */

    TCFramePool         pools[TC_FRAME_STAGE_NUM];

    const TCFrameSpecs  *specs;  /* what we need here? */
    /* (de)allocation helpers */
    TCFrameAllocFn      alloc;
    TCFrameFreeFn       free;
};

static TCFrameRing tc_audio_ringbuffer;
static TCFrameRing tc_video_ringbuffer;

/*************************************************************************/

static TCFramePool *tc_frame_ring_get_pool(TCFrameRing *rfb,
                                           TCFrameStatus S)
{
    return &(rfb->pools[TC_FRAME_STAGE_ID(S)]);
}

/* sometimes the lock is taken on the upper layer */
static int tc_frame_ring_get_pool_size(TCFrameRing *rfb,
                                       TCFrameStatus S,
                                       int locked)
{
    int size;
    TCFramePool *P = tc_frame_ring_get_pool(rfb, S);
    if (locked) {
        pthread_mutex_lock(&P->lock);
    }
    size = tc_frame_fifo_size(P->fifo);
    if (locked) {
        pthread_mutex_unlock(&P->lock);
    }
    return size;
}


static void tc_frame_ring_put_frame(TCFrameRing *rfb,
                                    TCFrameStatus S,
                                    TCFramePtr ptr)
{
    TCFramePool *P = tc_frame_ring_get_pool(rfb, S);
    ptr.generic->status = S;
    tc_frame_pool_put_frame(P, ptr);
}

static TCFramePtr tc_frame_ring_get_frame(TCFrameRing *rfb,
                                          TCFrameStatus S)
{
    TCFramePool *P = tc_frame_ring_get_pool(rfb, S);
    return tc_frame_pool_get_frame(P);
}

static void tc_frame_ring_dump_status(TCFrameRing *rfb,
                                      const char *id)
{
    tc_log_msg(FRBUF_NAME,
               "(%s|%s|%li) frame status: null=%i empty=%i wait=%i"
               " locked=%i ready=%i",
               id, rfb->tag, PTHREAD_ID,
               tc_frame_ring_get_pool_size(rfb, TC_FRAME_NULL,   TC_FALSE),
               tc_frame_ring_get_pool_size(rfb, TC_FRAME_EMPTY,  TC_FALSE),
               tc_frame_ring_get_pool_size(rfb, TC_FRAME_WAIT,   TC_FALSE),
               tc_frame_ring_get_pool_size(rfb, TC_FRAME_LOCKED, TC_FALSE), /* legacy */
               tc_frame_ring_get_pool_size(rfb, TC_FRAME_READY,  TC_FALSE));
}


/*************************************************************************/
/* NEW API, yet private                                                  */
/*************************************************************************/

/*
 * tc_frame_ring_init: (NOT thread safe)
 *     initialize a framebuffer ringbuffer by allocating needed
 *     amount of frames using given parameters.
 *
 * Parameters:
 *       rfb: ring framebuffer structure to initialize.
 *     specs: frame specifications to use for allocation.
 *     alloc: frame allocation function to use.
 *      free: frame disposal function to use.
 *      size: size of ringbuffer (number of frame to allocate)
 * Return Value:
 *      > 0: wrong (NULL) parameters
 *        0: succesfull
 *      < 0: allocation failed for one or more framesbuffers/internal error
 */
static int tc_frame_ring_init(TCFrameRing *rfb,
                              const char *tag,
                              const TCFrameSpecs *specs,
                              TCFrameAllocFn alloc,
                              TCFrameFreeFn free,
                              int size)
{
    int i = 0;

    if (rfb == NULL   || specs == NULL || size < 0
     || alloc == NULL || free == NULL) {
        return 1;
    }
    size = (size > 0) ?size :1; /* allocate at least one frame */

    rfb->frames = tc_malloc(size * sizeof(TCFramePtr));
    if (rfb->frames == NULL) {
        return -1;
    }

    rfb->tag   = tag;
    rfb->size  = size;
    rfb->specs = specs;
    rfb->alloc = alloc;
    rfb->free  = free;

    /* first, warm up the pools */
    for (i = 0; i < TC_FRAME_STAGE_NUM; i++) {
        TCFrameStatus S = TC_FRAME_STAGE_ST(i);
        const char *name = frame_status_name(S);

        int err = tc_frame_pool_init(&(rfb->pools[i]), size,
                                     (S == TC_FRAME_READY),
                                     name, tag);
        
        if (err) {
            tc_log_error(FRING_NAME,
                         "(init|%s) failed to init [%s] frame pool", tag, name);
            return err;
        }
    }

    /* then, fillup the `NULL' pool */
    for (i = 0; i < size; i++) {
        rfb->frames[i] = rfb->alloc(rfb->specs);
        if (TCFRAMEPTR_IS_NULL(rfb->frames[i])) {
            if (verbose >= TC_DEBUG) {
                tc_log_error(FRING_NAME,
                             "(init|%s) failed frame allocation", tag);
            }
            return -1;
        }

        rfb->frames[i].generic->bufid = i;
        tc_frame_ring_put_frame(rfb, TC_FRAME_NULL, rfb->frames[i]);

        if (verbose >= TC_FLIST) {
            tc_log_error(FRING_NAME,
                         "(init|%s) frame [%p] allocated at bufid=[%i]",
                         tag,
                         rfb->frames[i].generic,
                         rfb->frames[i].generic->bufid);
        }
 
    }
    return 0;
}

/*
 * tc_frame_ring_fini: (NOT thread safe)
 *     finalize a framebuffer ringbuffer by freeing all acquired
 *     resources (framebuffer memory).
 *
 * Parameters:
 *       rfb: ring framebuffer structure to finalize.
 * Return Value:
 *       None.
 */
static void tc_frame_ring_fini(TCFrameRing *rfb)
{
    if (rfb != NULL && rfb->free != NULL) {
        int i = 0;
 
        /* cool down the pools */
        for (i = 0; i < TC_FRAME_STAGE_NUM; i++) {
            const char *name = frame_status_name(TC_FRAME_STAGE_ST(i));
            int err = tc_frame_pool_fini(&(rfb->pools[i]));
        
            if (err) {
                tc_log_error(FRBUF_NAME,
                             "(fini|%s) failed to fini [%s] frame pool",
                             rfb->tag, name);
            }
        }
   
        for (i = 0; i < rfb->size; i++) {
            if (verbose >= TC_CLEANUP) {
                tc_log_info(FRING_NAME,
                            "(fini|%s) freeing frame #%i in [%s] status",
                            rfb->tag, i,
                            frame_status_name(rfb->frames[i].generic->status));
            }
            rfb->free(rfb->frames[i]);
        }
        tc_free(rfb->frames);
    }
}

/*
 * tc_frame_ring_register_frame:
 *      retrieve and register a framebuffer from a ringbuffer by
 *      attaching an ID to it, setup properly status and updating
 *      internal ringbuffer counters.
 *
 *      That's the function that client code is supposed to use
 *      (maybe wrapped by some thin macros to save status setting troubles).
 *      In general, dont' use retrieve_frame directly, use register_frame
 *      instead.
 *
 * Parameters:
 *         rfb: ring framebuffer to use
 *          id: id to attach to registered framebuffer
 *      status: status of framebuffer to register. This was needed to
 *              make registering process multi-purpose.
 * Return Value:
 *      Always a generic framebuffer pointer. That can be pointing to NULL
 *      if there isn't no more framebuffer avalaible on given ringbuffer;
 *      otherwise, it will point to a valid framebuffer.
 */
static TCFramePtr tc_frame_ring_register_frame(TCFrameRing *rfb,
                                               int id, int status)
{
    TCFramePtr ptr;

    if (verbose >= TC_FLIST)
        tc_log_msg(FRING_NAME,
                   "(register_frame|%s|%li) registering frame id=[%i]",
                   rfb->tag, PTHREAD_ID,
                   id);

    ptr = tc_frame_ring_get_frame(rfb, TC_FRAME_NULL);

    if (!TCFRAMEPTR_IS_NULL(ptr)) {
        if (status == TC_FRAME_EMPTY) {
            /* reset the frame data */
            ptr.generic->id         = id;
            ptr.generic->tag        = 0;
            ptr.generic->filter_id  = 0;
            ptr.generic->attributes = 0;
            /*ptr.generic->codec      = TC_CODEC_NULL;*/
            ptr.generic->next       = NULL;
            ptr.generic->prev       = NULL;
        }

        ptr.generic->status = status;

        if (verbose >= TC_FLIST) {
            tc_frame_ring_dump_status(rfb, "register_frame");
        }
    }
    return ptr; 
}

/*
 * tc_frame_ring_remove_frame:
 *      De-register and release a given framebuffer;
 *      also updates internal ringbuffer counters.
 *      
 *      That's the function that client code is supposed to use.
 *      In general, don't use release_frame directly, use remove_frame
 *      instead.
 *
 * Parameters:
 *        rfb: ring framebuffer to use.
 *      frame: generic pointer to frambuffer to remove.
 * Return Value:
 *      None.
 */
static void tc_frame_ring_remove_frame(TCFrameRing *rfb,
                                       TCFramePtr frame)
{
    if (rfb != NULL && !TCFRAMEPTR_IS_NULL(frame)) {
        /* release valid pointer to pool */
        tc_frame_ring_put_frame(rfb, TC_FRAME_NULL, frame);

        if (verbose >= TC_FLIST) {
            tc_frame_ring_dump_status(rfb, "remove_frame");
        }
    }
}

static void tc_frame_ring_reinject_frame(TCFrameRing *rfb,
                                         TCFramePtr frame)
{
    if (rfb != NULL && !TCFRAMEPTR_IS_NULL(frame)) {
        /* reinject valid pointer into pool */
        tc_frame_ring_put_frame(rfb, frame.generic->status, frame);

        if (verbose >= TC_FLIST) {
            tc_frame_ring_dump_status(rfb, "reinject_frame");
        }
    }
}


/*
 * tc_frame_ring_flush (NOT thread safe):
 *      unclaim ALL claimed frames on given ringbuffer, making
 *      ringbuffer ready to be used again.
 *
 * Parameters:
 *      rfb: ring framebuffer to use.
 * Return Value:
 *      amount of framebuffer unclaimed by this function.
 */
static int tc_frame_ring_flush(TCFrameRing *rfb)
{
    int i = 0, n = 0;
    TCFramePool *NP = tc_frame_ring_get_pool(rfb, TC_FRAME_NULL);

    for (i = 0; i < rfb->size; i++) {
        TCFrameStatus S = rfb->frames[i].generic->status;

        if (S == TC_FRAME_NULL) {
            if (verbose >= TC_FLIST) {
                /* 99% of times we don't want to see this. */
                tc_log_info(FRING_NAME,
                            "(flush|%s) frame #%i already free (not flushed)",
                            rfb->tag, i);
            }
        } else {
            TCFramePool *P = tc_frame_ring_get_pool(rfb, S);
            TCFramePtr frame;

            if (verbose >= TC_CLEANUP) {
                tc_log_info(FRING_NAME,
                            "(flush|%s) flushing frame #%i in [%s] status",
                            rfb->tag, i, frame_status_name(S));
            }

            frame = tc_frame_pool_pull_frame(P);
            if (TCFRAMEPTR_IS_NULL(frame)) {
                tc_log_info(FRING_NAME,
                            "(flush|%s) got NULL while flushing frame #%i",
                            rfb->tag, i);
                tc_frame_pool_dump_status(P);
            } else {
                frame.generic->status = TC_FRAME_NULL;
                tc_frame_pool_push_frame(NP, frame);
                n++;
            }
        }
    }

    return n;
}

#define TC_FRAME_STAGE_ALL (-1)

static void tc_frame_ring_wakeup(TCFrameRing *rfb, int stage)
{
    int i = 0;

    for (i = 0; i < TC_FRAME_STAGE_NUM; i++) {
        if (stage == TC_FRAME_STAGE_ALL || stage == i) {
            if (verbose >= TC_CLEANUP) {
                tc_log_info(FRING_NAME,
                            "(wakeup|%s|%li) waking up pool [%s]",
                            rfb->tag, PTHREAD_ID,
                            frame_stages[i].name);
            }

            tc_frame_pool_wakeup(&(rfb->pools[i]), frame_stages[i].broadcast);
        }
    }
}

static void tc_frame_ring_push_next(TCFrameRing *rfb,
                                    TCFramePtr ptr, int status)
{
    if (verbose >= TC_FLIST) {
        tc_log_msg(FRBUF_NAME,
                   "(push_next|%s|%li) frame=[%p] bufid=[%i] [%s] -> [%s]",
                   rfb->tag, PTHREAD_ID,
                   ptr.generic,
                   ptr.generic->bufid,
                   frame_status_name(ptr.generic->status),
                   frame_status_name(status));
    }
    tc_frame_ring_put_frame(rfb, status, ptr);
}

static void tc_frame_ring_get_counters(TCFrameRing *rfb,
                                       int *im, int *fl, int *ex)
{
    *im = tc_frame_ring_get_pool_size(rfb, TC_FRAME_EMPTY, TC_FALSE);
    *fl = tc_frame_ring_get_pool_size(rfb, TC_FRAME_WAIT,  TC_FALSE);
    *ex = tc_frame_ring_get_pool_size(rfb, TC_FRAME_READY, TC_FALSE);
}

/*************************************************************************/
/* Backward-compatible API                                               */
/*************************************************************************/

int aframe_alloc(int num)
{
    return tc_frame_ring_init(&tc_audio_ringbuffer,
                              "audio", &tc_specs,
                              tc_audio_alloc, tc_audio_free, num);
}

int vframe_alloc(int num)
{
    return tc_frame_ring_init(&tc_video_ringbuffer,
                              "video", &tc_specs,
                              tc_video_alloc, tc_video_free, num);
}

void aframe_free(void)
{
    tc_frame_ring_fini(&tc_audio_ringbuffer);
}

void vframe_free(void)
{
    tc_frame_ring_fini(&tc_video_ringbuffer);
}


aframe_list_t *aframe_dup(aframe_list_t *f)
{
    TCFramePtr frame;

    if (f == NULL) {
        tc_log_warn(FRBUF_NAME, "aframe_dup: empty frame");
        return NULL;
    }

    frame = tc_frame_ring_register_frame(&tc_audio_ringbuffer,
                                         0, TC_FRAME_WAIT);
    if (!TCFRAMEPTR_IS_NULL(frame)) {
        aframe_copy(frame.audio, f, 1);
        tc_frame_ring_put_frame(&tc_audio_ringbuffer,
                                TC_FRAME_WAIT, frame);
    }
    return frame.audio;
}

vframe_list_t *vframe_dup(vframe_list_t *f)
{
    TCFramePtr frame;

    if (f == NULL) {
        tc_log_warn(FRBUF_NAME, "vframe_dup: empty frame");
        return NULL;
    }

    frame = tc_frame_ring_register_frame(&tc_video_ringbuffer,
                                         0, TC_FRAME_WAIT);
    if (!TCFRAMEPTR_IS_NULL(frame)) {
        vframe_copy(frame.video, f, 1);
        /* currently noone seems to care about this */
        frame.video->clone_flag = f->clone_flag + 1;
        tc_frame_ring_put_frame(&tc_video_ringbuffer,
                                TC_FRAME_WAIT, frame);
    }
    return frame.video;
}

/*************************************************************************/

aframe_list_t *aframe_register(int id)
{
    TCFramePtr frame = tc_frame_ring_register_frame(&tc_audio_ringbuffer,
                                                    id, TC_FRAME_EMPTY);
    return frame.audio;
}

vframe_list_t *vframe_register(int id)
{
    TCFramePtr frame = tc_frame_ring_register_frame(&tc_video_ringbuffer,
                                                    id, TC_FRAME_EMPTY); 
    return frame.video;
}

void aframe_remove(aframe_list_t *ptr)
{
    if (ptr == NULL) {
        tc_log_warn(FRBUF_NAME, "aframe_remove: given NULL frame pointer");
    } else {
        TCFramePtr frame = { .audio = ptr };
        tc_frame_ring_remove_frame(&tc_audio_ringbuffer, frame);
    }
}

void vframe_remove(vframe_list_t *ptr)
{
    if (ptr == NULL) {
        tc_log_warn(FRBUF_NAME, "vframe_remove: given NULL frame pointer");
    } else {
        TCFramePtr frame = { .video = ptr };
        tc_frame_ring_remove_frame(&tc_video_ringbuffer, frame);
    }
}

void aframe_reinject(aframe_list_t *ptr)
{
    if (ptr == NULL) {
        tc_log_warn(FRBUF_NAME, "aframe_reinject: given NULL frame pointer");
    } else {
        TCFramePtr frame = { .audio = ptr };
        tc_frame_ring_reinject_frame(&tc_audio_ringbuffer, frame);
    }
}

void vframe_reinject(vframe_list_t *ptr)
{
    if (ptr == NULL) {
        tc_log_warn(FRBUF_NAME, "vframe_reinject: given NULL frame pointer");
    } else {
        TCFramePtr frame = { .video = ptr };
        tc_frame_ring_reinject_frame(&tc_video_ringbuffer, frame);
    }
}

aframe_list_t *aframe_retrieve(void)
{
    TCFramePtr ptr = tc_frame_ring_get_frame(&tc_audio_ringbuffer, TC_FRAME_READY);
    return ptr.audio;
}

vframe_list_t *vframe_retrieve(void)
{
    TCFramePtr ptr = tc_frame_ring_get_frame(&tc_video_ringbuffer, TC_FRAME_READY);
    return ptr.video;
}

aframe_list_t *aframe_reserve(void)
{
    TCFramePtr ptr = tc_frame_ring_get_frame(&tc_audio_ringbuffer, TC_FRAME_WAIT);
    return ptr.audio;
}

vframe_list_t *vframe_reserve(void)
{
    TCFramePtr ptr = tc_frame_ring_get_frame(&tc_video_ringbuffer, TC_FRAME_WAIT);
    return ptr.video;
}

void aframe_push_next(aframe_list_t *ptr, int status)
{
    if (ptr == NULL) {
        /* a bit more of paranoia */
        tc_log_warn(FRBUF_NAME, "aframe_push_next: given NULL frame pointer");
    } else {
        TCFramePtr frame = { .audio = ptr };

        tc_frame_ring_push_next(&tc_audio_ringbuffer, frame, status);
    }
}

void vframe_push_next(vframe_list_t *ptr, int status)
{
    if (ptr == NULL) {
        /* a bit more of paranoia */
        tc_log_warn(FRBUF_NAME, "vframe_push_next: given NULL frame pointer");
    } else {
        TCFramePtr frame = { .video = ptr };

        tc_frame_ring_push_next(&tc_video_ringbuffer, frame, status);
    }
}

/*************************************************************************/

void aframe_flush(void)
{
    tc_frame_ring_flush(&tc_audio_ringbuffer);
}

void vframe_flush(void)
{
    tc_frame_ring_flush(&tc_video_ringbuffer);
}

void tc_framebuffer_flush(void)
{
    tc_frame_ring_flush(&tc_audio_ringbuffer);
    tc_frame_ring_flush(&tc_video_ringbuffer);
}

/*************************************************************************/

void tc_framebuffer_interrupt_stage(TCFrameStatus S)
{
    int i = TC_FRAME_STAGE_ID(S);
    if (i >= 0 && i < TC_FRAME_STAGE_NUM) {
        tc_frame_ring_wakeup(&tc_audio_ringbuffer, i);
        tc_frame_ring_wakeup(&tc_video_ringbuffer, i);
    } else {
        tc_log_warn(FRBUF_NAME, "interrupt_stage: bad status (%i)", S);
    }
}

void tc_framebuffer_interrupt(void)
{
    tc_frame_ring_wakeup(&tc_audio_ringbuffer, TC_FRAME_STAGE_ALL);
    tc_frame_ring_wakeup(&tc_video_ringbuffer, TC_FRAME_STAGE_ALL);
}

/*************************************************************************/

void aframe_dump_status(void)
{
    tc_frame_ring_dump_status(&tc_audio_ringbuffer,
                              "buffer status");
}

void vframe_dump_status(void)
{
    tc_frame_ring_dump_status(&tc_video_ringbuffer,
                              "buffer status");
}

/*************************************************************************/

int vframe_have_more(void)
{
    return tc_frame_ring_get_pool_size(&tc_video_ringbuffer,
                                       TC_FRAME_EMPTY, TC_TRUE);
}

int aframe_have_more(void)
{
    return tc_frame_ring_get_pool_size(&tc_audio_ringbuffer,
                                       TC_FRAME_EMPTY, TC_TRUE);
}

/*************************************************************************/
/* Frame copying routines                                                */
/*************************************************************************/

void aframe_copy(aframe_list_t *dst, const aframe_list_t *src,
                 int copy_data)
{
    if (!dst || !src) {
        tc_log_warn(__FILE__, "aframe_copy: given NULL frame pointer");
    	return;
    }

    /* copy all common fields with just one move */
    ac_memcpy(dst, src, sizeof(frame_list_t));
    
    if (copy_data) {
        /* really copy video data */
        ac_memcpy(dst->audio_buf, src->audio_buf, dst->audio_size);
    } else {
        /* soft copy, new frame points to old audio data */
        dst->audio_buf = src->audio_buf;
    }
}

void vframe_copy(vframe_list_t *dst, const vframe_list_t *src,
                 int copy_data)
{
    if (!dst || !src) {
        tc_log_warn(__FILE__, "vframe_copy: given NULL frame pointer");
    	return;
    }

    /* copy all common fields with just one move */
    ac_memcpy(dst, src, sizeof(frame_list_t));
    
    dst->clone_flag   = src->clone_flag;
    dst->deinter_flag = src->deinter_flag;
    dst->free         = src->free;
    /* 
     * we assert that plane pointers *are already properly set*
     * we're focused on copy _content_ here.
     */

    if (copy_data == 1) {
        /* really copy video data */
        ac_memcpy(dst->video_buf,  src->video_buf,  dst->video_size);
        ac_memcpy(dst->video_buf2, src->video_buf2, dst->video_size);
    } else {
        /* soft copy, new frame points to old video data */
        dst->video_buf  = src->video_buf;
        dst->video_buf2 = src->video_buf2;
    }
}

/*************************************************************************/

void vframe_get_counters(int *im, int *fl, int *ex)
{
    tc_frame_ring_get_counters(&tc_video_ringbuffer, im, fl, ex);
}

void aframe_get_counters(int *im, int *fl, int *ex)
{
    tc_frame_ring_get_counters(&tc_audio_ringbuffer, im, fl, ex);
}

void tc_framebuffer_get_counters(int *im, int *fl, int *ex)
{
    int v_im, v_fl, v_ex, a_im, a_fl, a_ex;

    vframe_get_counters(&v_im, &v_fl, &v_ex);
    aframe_get_counters(&a_im, &a_fl, &a_ex);

    *im = v_im + a_im;
    *fl = v_fl + a_fl;
    *ex = v_ex + a_ex;
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

