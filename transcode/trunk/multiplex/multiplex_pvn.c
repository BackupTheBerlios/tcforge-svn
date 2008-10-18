/*
 * multiplex_pvn.c -- module for writing PVN video streams
 * (http://www.cse.yorku.ca/~jgryn/research/pvnspecs.html)
 * Written by Andrew Church <achurch@achurch.org>
 *
 * This file is part of transcode, a video stream processing tool.
 * transcode is free software, distributable under the terms of the GNU
 * General Public License (version 2 or later).  See the file COPYING
 * for details.
 */

#include "src/transcode.h"
#include "libtc/libtc.h"
#include "libtcutil/optstr.h"
#include "libtcmodule/tcmodule-plugin.h"
#include "libtcvideo/tcvideo.h"

#ifdef OMS_COMPATIBLE
#define MOD_NAME        "export_pvn.so"
#else
#define MOD_NAME        "multiplex_pvn.so"
#endif

#define MOD_VERSION     "v1.0 (2006-10-06)"
#define MOD_CAP         "Writes PVN video files"
#define MOD_AUTHOR      "Andrew Church"

#define MOD_FEATURES \
    TC_MODULE_FEATURE_MULTIPLEX|TC_MODULE_FEATURE_VIDEO
    
#define MOD_FLAGS \
    TC_MODULE_FLAG_RECONFIGURABLE


#ifdef OMS_COMPATIBLE
#define METHOD extern
#else
#define METHOD static
#endif

/*************************************************************************/

/* Local data structure: */

typedef struct {
    int width, height;     // Frame width and height (to catch changes)
    int fd;                // Output file descriptor
    int framecount;        // Number of frames written
    off_t framecount_pos;  // File position of frame count (for rewriting)
} PrivateData;

/*************************************************************************/
#ifdef OMS_COMPATIBLE
/* the needed forward declarations                                       */

int pvn_init(TCModuleInstance *self, uint32_t features);
int pvn_configure(TCModuleInstance *self, const char *options, vob_t *vob);
int pvn_fini(TCModuleInstance *self);
int pvn_multiplex(TCModuleInstance *self,
                  vframe_list_t *vframe, aframe_list_t *aframe);
#endif

/*************************************************************************/

/* Module interface routines and data. */

/*************************************************************************/

/**
 * pvn_stop:  Reset this instance of the module.  See tcmodule-data.h for
 * function details.
 */

static int pvn_stop(TCModuleInstance *self)
{
    PrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "stop");

    pd = self->userdata;

    if (pd->fd != -1) {
        if (pd->framecount > 0 && pd->framecount_pos > 0) {
            /* Write out final frame count, if we can */
            if (lseek(pd->fd, pd->framecount_pos, SEEK_SET) != (off_t)-1) {
                char buf[11];
                int len = tc_snprintf(buf, sizeof(buf), "%10d",pd->framecount);
                if (len > 0)
                    tc_pwrite(pd->fd, buf, len);
            }
        }
        close(pd->fd);
        pd->fd = -1;
    }

    return TC_OK;
}

/*************************************************************************/

/**
 * pvn_configure:  Configure this instance of the module.  See
 * tcmodule-data.h for function details.
 */

METHOD int pvn_configure(TCModuleInstance *self,
                         const char *options, vob_t *vob)
{
    PrivateData *pd = NULL;
    char buf[TC_BUF_MAX];
    int len;

    TC_MODULE_SELF_CHECK(self, "configure");

    pd->width  = vob->ex_v_width;
    pd->height = vob->ex_v_height;
    /* FIXME: stdout should be handled in a more standard fashion */
    if (strcmp(vob->video_out_file, "-") == 0) {  // allow /dev/stdout too?
        pd->fd = 1;
    } else {
        pd->fd = open(vob->video_out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (pd->fd < 0) {
            tc_log_error(MOD_NAME, "Unable to open %s: %s",
                         vob->video_out_file, strerror(errno));
            goto fail;
        }
    }
    len = tc_snprintf(buf, sizeof(buf), "PV%da\r\n%d %d\r\n",
                      vob->decolor ? 5 : 6,
                      pd->width, pd->height);
    if (len < 0)
        goto fail;
    if (tc_pwrite(pd->fd, buf, len) != len) {
        tc_log_error(MOD_NAME, "Unable to write header to %s: %s",
                     vob->video_out_file, strerror(errno));
        goto fail;
    }
    pd->framecount_pos = lseek(pd->fd, 0, SEEK_CUR);  // failure okay
    len = tc_snprintf(buf, sizeof(buf), "%10d\r\n8\r\n%lf\r\n",
                      0, (double)vob->ex_fps);
    if (len < 0)
        goto fail;
    if (tc_pwrite(pd->fd, buf, len) != len) {
        tc_log_error(MOD_NAME, "Unable to write header to %s: %s",
                     vob->video_out_file, strerror(errno));
        goto fail;
    }

    return TC_OK;

  fail:
    pvn_stop(self);
    return TC_ERROR;
}

/*************************************************************************/

/**
 * pvn_init:  Initialize this instance of the module.  See tcmodule-data.h
 * for function details.
 */

METHOD int pvn_init(TCModuleInstance *self, uint32_t features)
{
    PrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "init");
    TC_MODULE_INIT_CHECK(self, MOD_FEATURES, features);

    self->userdata = pd = tc_malloc(sizeof(PrivateData));
    if (!pd) {
        tc_log_error(MOD_NAME, "init: out of memory!");
        return TC_ERROR;
    }
    pd->fd = -1;
    pd->framecount = 0;
    pd->framecount_pos = 0;

    if (verbose) {
        tc_log_info(MOD_NAME, "%s %s", MOD_VERSION, MOD_CAP);
    }
    return TC_OK;
}

/*************************************************************************/

/**
 * pvn_fini:  Clean up after this instance of the module.  See
 * tcmodule-data.h for function details.
 */

METHOD int pvn_fini(TCModuleInstance *self)
{
    TC_MODULE_SELF_CHECK(self, "fini");

    pvn_stop(self);
    tc_free(self->userdata);
    self->userdata = NULL;

    return TC_OK;
}

/*************************************************************************/


/*************************************************************************/

/**
 * pvn_inspect:  Return the value of an option in this instance of the
 * module.  See tcmodule-data.h for function details.
 */

static int pvn_inspect(TCModuleInstance *self,
                       const char *param, const char **value)
{
    static char buf[TC_BUF_MAX];

    TC_MODULE_SELF_CHECK(self, "inspect");
    TC_MODULE_SELF_CHECK(param, "inspect");

    if (optstr_lookup(param, "help")) {
        tc_snprintf(buf, sizeof(buf),
                "Overview:\n"
                "    Writes a PVN video stream (format PV6a, 8-bit data).\n"
                "    A grayscale file (PV5a) is written instead if the -K\n"
                "    switch is given to transcode.\n"
                "    The RGB colorspace must be used (-V rgb24).\n"
                "No options available.\n");
        *value = buf;
    }
    return TC_OK;
}


/*************************************************************************/

/**
 * pvn_multiplex:  Multiplex a frame of data.  See tcmodule-data.h for
 * function details.
 */

METHOD int pvn_multiplex(TCModuleInstance *self,
                         vframe_list_t *vframe, aframe_list_t *aframe)
{
    PrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "multiplex");

    pd = self->userdata;
    if (pd->fd == -1) {
        tc_log_error(MOD_NAME, "multiplex: no file opened!");
        return TC_ERROR;
    }

    if (vframe->v_width != pd->width || vframe->v_height != pd->height) {
        tc_log_error(MOD_NAME, "Video frame size changed in midstream!");
        return TC_ERROR;
    }
    if (vframe->v_codec != CODEC_RGB) {
        tc_log_error(MOD_NAME, "Invalid codec for video frame!");
        return TC_ERROR;
    }
    if (vframe->video_len != pd->width * pd->height * 3
     && vframe->video_len != pd->width * pd->height  // for grayscale
    ) {
        tc_log_error(MOD_NAME, "Invalid size for video frame!");
        return TC_ERROR;
    }
    if (tc_pwrite(pd->fd, vframe->video_buf, vframe->video_len)
        != vframe->video_len
    ) {
        tc_log_error(MOD_NAME, "Error writing frame %d to output file: %s",
                     pd->framecount, strerror(errno));
        return TC_ERROR;
    }
    pd->framecount++;
    return vframe->video_len;
}

/*************************************************************************/

static const TCCodecID pvn_codecs_in[] = { TC_CODEC_RGB, TC_CODEC_ERROR };
static const TCFormatID pvn_formats_out[] = { TC_FORMAT_PVN, TC_CODEC_ERROR };
/* a multiplexor is at the end of pipeline */
TC_MODULE_MPLEX_FORMATS_CODECS(pvn);

TC_MODULE_INFO(pvn);

static const TCModuleClass pvn_class = {
    TC_MODULE_CLASS_HEAD(pvn),

    .init      = pvn_init,
    .fini      = pvn_fini,
    .configure = pvn_configure,
    .stop      = pvn_stop,
    .inspect   = pvn_inspect,

    .multiplex = pvn_multiplex,
};

TC_MODULE_ENTRY_POINT(pvn);

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
