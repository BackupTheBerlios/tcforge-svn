/*
 * export_pvn.c -- module for exporting PVN video streams
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

static int verbose_flag;
static int capability_flag = TC_CAP_RGB;

#define MOD_NAME        "export_pvn.so"
#define MOD_VERSION     "v1.0 (2006-10-06)"
#define MOD_CAP         "Writes PVN video files"
#define MOD_AUTHOR      "Andrew Church"


#define MOD_PRE pvn
#define MOD_CODEC "(video) PVN"


#include "export_def.h"

/*************************************************************************/
/* the needed forward declarations                                       */

int pvn_init(TCModuleInstance *self, uint32_t features);
int pvn_configure(TCModuleInstance *self, const char *options, vob_t *vob);
int pvn_fini(TCModuleInstance *self);
int pvn_multiplex(TCModuleInstance *self,
                  vframe_list_t *vframe, aframe_list_t *aframe);

/*************************************************************************/
/* Old-fashioned module interface. */

static TCModuleInstance mod;

MOD_init
{
    return TC_OK;
}

MOD_stop
{
    return TC_OK;
}


/*************************************************************************/

MOD_open
{
    if (param->flag != TC_VIDEO)
        return TC_ERROR;
    if (pvn_init(&mod, TC_MODULE_FEATURE_MULTIPLEX|TC_MODULE_FEATURE_VIDEO) < 0)
        return TC_ERROR;
    return pvn_configure(&mod, vob->ex_v_string, vob);
}

/*************************************************************************/

MOD_close
{
    if (param->flag != TC_VIDEO)
        return TC_ERROR;
    pvn_fini(&mod);
    return TC_OK;
}

/*************************************************************************/

MOD_encode
{
    vframe_list_t vframe;
    vob_t *vob = tc_get_vob();

    if (param->flag != TC_VIDEO)
        return TC_ERROR;

    vframe.v_width   = vob->ex_v_width;
    vframe.v_height  = vob->ex_v_height;
    vframe.v_codec   = vob->ex_v_codec;
    vframe.video_buf = param->buffer;
    vframe.video_len = param->size;
    if (!vframe.v_codec)
        vframe.v_codec = TC_CODEC_RGB24;  // assume it's correct
    if (vob->decolor) {
        // Assume the data is already decolored and just take every third byte
        int i;
        vframe.video_len /= 3;
        for (i = 0; i < vframe.video_len; i++)
            vframe.video_buf[i] = vframe.video_buf[i*3];
    }
    if (pvn_multiplex(&mod, &vframe, NULL) < 0)
        return TC_ERROR;

    return TC_OK;
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
