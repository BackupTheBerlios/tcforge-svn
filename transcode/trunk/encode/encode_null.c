/*
 *  encode_null.c -- produces empty (as in zero-sized) A/V frames.
 *  (C) 2005-2008 Francesco Romani <fromani at gmail dot com>
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


#include "transcode.h"
#include "framebuffer.h"
#include "libtc/optstr.h"

#include "libtc/tcmodule-plugin.h"

#define MOD_NAME    "encode_null.so"
#define MOD_VERSION "v0.0.3 (2005-06-05)"
#define MOD_CAP     "null (fake) A/V encoder"

#define MOD_FEATURES \
    TC_MODULE_FEATURE_ENCODE|TC_MODULE_FEATURE_VIDEO|TC_MODULE_FEATURE_AUDIO

#define MOD_FLAGS \
    TC_MODULE_FLAG_RECONFIGURABLE
    

static const char null_help[] = ""
    "Overview:\n"
    "    this module absorbs provided A/V frames and produces fake,"
    "    empty \"encoded\" frames.\n"
    "Options:\n"
    "    help    produce module overview and options explanations\n";

static int null_init(TCModuleInstance *self, uint32_t features)
{
    TC_MODULE_SELF_CHECK(self, "init");
    TC_MODULE_INIT_CHECK(self, MOD_FEATURES, features);

    if (verbose) {
        tc_log_info(MOD_NAME, "%s %s", MOD_VERSION, MOD_CAP);
    }
    self->userdata = NULL;

    return TC_OK;
}

static int null_fini(TCModuleInstance *self)
{
    TC_MODULE_SELF_CHECK(self, "fini");

    return TC_OK;
}

static int null_configure(TCModuleInstance *self,
                          const char *options, vob_t *vob)
{
    TC_MODULE_SELF_CHECK(self, "configure");

    return TC_OK;
}

static int null_inspect(TCModuleInstance *self,
                        const char *param, const char **value)
{
    TC_MODULE_SELF_CHECK(self, "inspect");

    if (optstr_lookup(param, "help")) {
        *value = null_help;
    }

    return TC_OK;
}

static int null_stop(TCModuleInstance *self)
{
    TC_MODULE_SELF_CHECK(self, "stop");

    return TC_OK;
}

static int null_encode_video(TCModuleInstance *self,
                              vframe_list_t *inframe, vframe_list_t *outframe)
{
    TC_MODULE_SELF_CHECK(self, "encode_video");

    outframe->video_len = 0;
    return TC_OK;
}

static int null_encode_audio(TCModuleInstance *self,
                              aframe_list_t *inframe, aframe_list_t *outframe)
{
    TC_MODULE_SELF_CHECK(self, "encode_audio");

    outframe->audio_len = 0;
    return TC_OK;
}


/*************************************************************************/

static const TCCodecID null_codecs_in[] = { TC_CODEC_ANY, TC_CODEC_ERROR };
static const TCCodecID null_codecs_out[] = { TC_CODEC_ANY, TC_CODEC_ERROR };
TC_MODULE_CODEC_FORMATS(null);

TC_MODULE_INFO(null);

static const TCModuleClass null_class = {
    TC_MODULE_CLASS_HEAD(null),

    .init         = null_init,
    .fini         = null_fini,
    .configure    = null_configure,
    .stop         = null_stop,
    .inspect      = null_inspect,

    .encode_video = null_encode_video,
    .encode_audio = null_encode_audio,
};

TC_MODULE_ENTRY_POINT(null)

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
