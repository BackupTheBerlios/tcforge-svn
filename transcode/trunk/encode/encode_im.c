/*
 *  encode_im.c -- encodes video frames using ImageMagick.
 *  (C) 2007-2008 Francesco Romani <fromani at gmail dot com>
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


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Note: because of ImageMagick bogosity, this must be included first, so
 * we can undefine the PACKAGE_* symbols it splats into our namespace */
#ifdef HAVE_BROKEN_WAND
#include <wand/magick-wand.h>
#else /* we have a SANE wand header */
#include <wand/MagickWand.h>
#endif /* HAVE_BROKEN_WAND */

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "src/transcode.h"
#include "src/framebuffer.h"
#include "libtcutil/optstr.h"

#include "libtcmodule/tcmodule-plugin.h"



#define MOD_NAME    "encode_im.so"
#define MOD_VERSION "v0.1.0 (2007-11-19)"
#define MOD_CAP     "ImageMagick video frames encoder"

#define MOD_FEATURES \
    TC_MODULE_FEATURE_ENCODE|TC_MODULE_FEATURE_VIDEO

#define MOD_FLAGS \
    TC_MODULE_FLAG_RECONFIGURABLE

#define FMT_NAME_LEN    16
#define DEFAULT_QUALITY 75
#define DEFAULT_FORMAT  "png"

static const char tc_im_help[] = ""
    "Overview:\n"
    "    This module encodes video frames independently in various\n"
    "    image formats using ImageMagick libraries.\n"
    "Options:\n"
    "    format  name of the format to use for encoding images\n"
    "    quality select output quality (higher is better)\n"
    "    help    produce module overview and options explanations\n";

typedef struct tcimprivatedata_ TCIMPrivateData;
struct tcimprivatedata_ {
    MagickWand *wand;
    unsigned long quality;
    int width, height;
    char opt_buf[TC_BUF_MIN];
    char img_fmt[FMT_NAME_LEN];
};

static int TCHandleMagickError(MagickWand *wand)
{
    ExceptionType severity;
    const char *description = MagickGetException(wand, &severity);

    tc_log_error(MOD_NAME, "%s", description);

    MagickRelinquishMemory((void*)description);
    return TC_ERROR;
}

/*************************************************************************/

static int tc_im_configure(TCModuleInstance *self,
                          const char *options, vob_t *vob)
{
    TCIMPrivateData *pd = NULL;
    int ret = 0;
    TCCodecID id = TC_CODEC_ERROR;

    TC_MODULE_SELF_CHECK(self, "configure");

    pd = self->userdata;

    pd->quality = DEFAULT_QUALITY;
    pd->width   = vob->ex_v_width;
    pd->height  = vob->ex_v_height;

    pd->img_fmt[0] = '\0';
    optstr_get(options, "format", "%15s", pd->img_fmt);
    id = tc_codec_from_string(pd->img_fmt);
    
    if (id == TC_CODEC_ERROR) {
        strlcpy(pd->img_fmt, DEFAULT_FORMAT, sizeof(pd->img_fmt));
    }

    ret = optstr_get(options, "quality", "%lu", &pd->quality);
    if (ret != 1) {
        pd->quality = DEFAULT_QUALITY;
    }

    if (verbose >= TC_INFO) {
        tc_log_info(MOD_NAME, "encoding %s with quality %lu",
                    pd->img_fmt, pd->quality);
    }

    pd->wand = NewMagickWand();
    if (pd->wand == NULL) {
        tc_log_error(MOD_NAME, "cannot create magick wand");
        return TC_ERROR;
    }
    return TC_OK;
}

static int tc_im_inspect(TCModuleInstance *self,
                         const char *param, const char **value)
{
    TCIMPrivateData *pd = NULL;
    
    TC_MODULE_SELF_CHECK(self, "inspect");

    pd = self->userdata;

    if (optstr_lookup(param, "help")) {
        *value = tc_im_help;
    }
    if (optstr_lookup(param, "format")) {
        *value = pd->img_fmt;
    }
    if (optstr_lookup(param, "quality")) {
        tc_snprintf(pd->opt_buf, sizeof(pd->opt_buf), "%lu", pd->quality);
        *value = pd->opt_buf;
    }
    return TC_OK;
}

static int tc_im_stop(TCModuleInstance *self)
{
    TCIMPrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "stop");

    pd = self->userdata;

    if (pd->wand != NULL) {
        DestroyMagickWand(pd->wand);
        pd->wand = NULL;
    }
    return TC_OK;
}

static int tc_im_init(TCModuleInstance *self, uint32_t features)
{
    TCIMPrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "init");
    TC_MODULE_INIT_CHECK(self, MOD_FEATURES, features);

    MagickWandGenesis();

    pd = tc_zalloc(sizeof(TCIMPrivateData));
    if (!pd) {
        tc_log_error(MOD_NAME, "init: can't allocate private data");
        return TC_ERROR;
    }
    self->userdata = pd;

    if (verbose) {
        tc_log_info(MOD_NAME, "%s %s", MOD_VERSION, MOD_CAP);
    }
    return TC_OK;
}

static int tc_im_fini(TCModuleInstance *self)
{
    TC_MODULE_SELF_CHECK(self, "fini");

    tc_im_stop(self);

    tc_free(self->userdata);
    self->userdata = NULL;

    MagickWandTerminus();

    return TC_OK;
}

static int tc_im_encode_video(TCModuleInstance *self,
                              vframe_list_t *inframe, vframe_list_t *outframe)
{
    TCIMPrivateData *pd = NULL;
    MagickBooleanType status;
    const uint8_t *img = NULL;
    size_t img_len = 0;

    TC_MODULE_SELF_CHECK(self, "encode_video");

    pd = self->userdata;

    ClearMagickWand(pd->wand);
    /* 
     * This avoids IM to buffer all read images.
     * I'm quite sure that this can be done in a smarter way,
     * but I haven't yet figured out how. -- FRomani
     */

    status = MagickConstituteImage(pd->wand, pd->width, pd->height, "RGB",
                                   CharPixel, inframe->video_buf);
    if (status == MagickFalse) {
        return TCHandleMagickError(pd->wand);
    }

    MagickSetLastIterator(pd->wand);

    status = MagickSetFormat(pd->wand, pd->img_fmt);
    if (status == MagickFalse) {
        return TCHandleMagickError(pd->wand);
    }
    MagickSetCompressionQuality(pd->wand, pd->quality); /* will not fail */
    
    img = MagickGetImageBlob(pd->wand, &img_len);
    if (!img) {
        return TCHandleMagickError(pd->wand);
    }

    ac_memcpy(outframe->video_buf, img, img_len);
    outframe->video_len = img_len;
    outframe->attributes |= TC_FRAME_IS_KEYFRAME;

    MagickRelinquishMemory(img);

    return TC_OK;
}


/*************************************************************************/

static const TCCodecID tc_im_codecs_in[] = { 
    TC_CODEC_RGB, TC_CODEC_ERROR
};
static const TCCodecID tc_im_codecs_out[] = {
    TC_CODEC_JPEG, TC_CODEC_TIFF, TC_CODEC_PNG,
    TC_CODEC_PPM,  TC_CODEC_PGM,  TC_CODEC_GIF,
    TC_CODEC_ERROR
};
TC_MODULE_CODEC_FORMATS(tc_im);

TC_MODULE_INFO(tc_im);

static const TCModuleClass tc_im_class = {
    TC_MODULE_CLASS_HEAD(tc_im),

    .init         = tc_im_init,
    .fini         = tc_im_fini,
    .configure    = tc_im_configure,
    .stop         = tc_im_stop,
    .inspect      = tc_im_inspect,

    .encode_video = tc_im_encode_video,
};

TC_MODULE_ENTRY_POINT(tc_im);

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

