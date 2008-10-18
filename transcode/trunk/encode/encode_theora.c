/*
 *  encode_theora.c -- produces a theora stream using libtheora.
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

#include <theora/theora.h>

#include "transcode.h"
#include "framebuffer.h"
#include "libtc/optstr.h"

#include "libtc/ratiocodes.h"
#include "libtc/tcmodule-plugin.h"

#define TC_ENCODER 1
#include "libtcext/tc_ogg.h"

#define MOD_NAME    "encode_theora.so"
#define MOD_VERSION "v0.1.1 (2008-04-20)"
#define MOD_CAP     "theora video encoder using libtheora"

#define MOD_FEATURES \
    TC_MODULE_FEATURE_ENCODE|TC_MODULE_FEATURE_VIDEO

#define MOD_FLAGS \
    TC_MODULE_FLAG_RECONFIGURABLE

//#define TC_THEORA_DEBUG 1 // until 0.x.y at least

enum {
    TC_THEORA_QUALITY = 24,
    TC_THEORA_NSENS   = 0,
    TC_THEORA_QUICK   = 0,
    TC_THEORA_SHARP   = 0,
};

static const char tc_theora_help[] = ""
    "Overview:\n"
    "    this module produces a theora video stream using libtheora.\n"
    "Options:\n"
    "    quality encoder quality\n"
    "    nsens   noise sensitivity\n"
    "    sharp   sharpness setting [0-2]\n"
    "    quick   enable quick encoding\n"
    "    help    produce module overview and options explanations\n";


/*************************************************************************/

typedef struct theoraprivatedata_ TheoraPrivateData;
struct theoraprivatedata_ {
    int flush_flag;

    OGGExtraData xdata;

    theora_state td;
    TCFrameVideo *tbuf;
  
    int quality;
    int nsens;
    int sharp;
    int quick;

    uint32_t frames;
    uint32_t packets;

    char conf_str[TC_BUF_MIN];
};


/*************************************************************************/

static void tc_frame_video_setup(TCFrameVideo *f)
{
    int16_t *pkt_num = (int16_t*)f->video_buf;
    f->video_len = sizeof(*pkt_num);
    *pkt_num = 0;
}

static int tc_frame_video_add_ogg_packet(TheoraPrivateData *pd, 
                                         TCFrameVideo *f, ogg_packet *op)
{
    int16_t *pkt_num = (int16_t*)f->video_buf;
    double ts = theora_granule_time(&(pd->td), op->granulepos);
    int needed = sizeof(*op) + op->bytes;
    int avail = f->video_size - f->video_len;

    TC_FRAME_SET_TIMESTAMP_DOUBLE(f, ts);
    if (avail < needed) {
        tc_log_error(__FILE__, "(%s) no buffer in frame: (avail=%i|needed=%i)",
                     __func__, avail, needed);
        return TC_ERROR;
    }
    ac_memcpy(f->video_buf + f->video_len, op, sizeof(*op));
    f->video_len += sizeof(*op);
    ac_memcpy(f->video_buf + f->video_len, op->packet, op->bytes);
    f->video_len += op->bytes;
    *pkt_num += 1;

    if (op->e_o_s) {
        f->attributes |= TC_FRAME_IS_END_OF_STREAM; // useless?
    }
    return TC_OK;
}

// FIXME: better error checking
static int tc_ogg_new_extradata(TheoraPrivateData *pd)
{
    theora_comment tc;
    ogg_packet op;
    int ret;

    theora_encode_header(&(pd->td), &op);
    ret = tc_ogg_dup_packet(&(pd->xdata.header), &op);
    if (ret == TC_ERROR) {
        goto no_header;
    }
    theora_comment_init(&tc);
    theora_comment_add_tag(&tc, "ENCODER", PACKAGE " " VERSION);
    theora_encode_comment(&tc, &op);
    ret = tc_ogg_dup_packet(&(pd->xdata.comment), &op);
    if (ret == TC_ERROR) {
        goto no_comment;
    }
    /* theora_encode_comment() doesn't take a theora_state parameter, so it has to
       allocate its own buffer to pass back the packet data.
       If we don't free it here, we'll leak.
       libogg2 makes this much cleaner: the stream owns the buffer after you call
       packetin in libogg2, but this is not true in libogg1.*/
    free(op.packet);
    theora_encode_tables(&(pd->td), &op);
    ret = tc_ogg_dup_packet(&(pd->xdata.code), &op);
    if (ret == TC_ERROR) {
        goto no_code;
    }
    pd->xdata.magic = TC_CODEC_THEORA; // XXX

    return TC_OK;

  no_code:
    tc_ogg_del_packet(&(pd->xdata.comment));
  no_comment:
    tc_ogg_del_packet(&(pd->xdata.header));
  no_header:
    return TC_ERROR;
}


/*************************************************************************/


static int tc_theora_configure(TCModuleInstance *self,
                               const char *options, vob_t *vob)
{
    uint32_t x_off = 0, y_off = 0, w = 0, h = 0;
    TheoraPrivateData *pd = NULL;
    theora_info ti;
    int ret = TC_ERROR;

    TC_MODULE_SELF_CHECK(self, "configure");

    pd = self->userdata;

    pd->flush_flag = vob->encoder_flush;
    pd->packets    = 0;
    pd->frames     = 0;
    pd->quality    = TC_THEORA_QUALITY;
    pd->nsens      = TC_THEORA_NSENS;
    pd->sharp      = TC_THEORA_SHARP;
    pd->quick      = TC_THEORA_QUICK;

    if (options) {
        optstr_get(options, "quality",  "%i", &pd->quality);
        pd->quality = TC_CLAMP(pd->quality, 0, 63);
        if (optstr_lookup(options, "nsens")) {
            pd->nsens = 1;
        }
        if (optstr_lookup(options, "sharp")) {
            pd->sharp = 1;
        }
        if (optstr_lookup(options, "quick")) {
            pd->quick = 1;
        }
    }
 
    /* Theora has a divisible-by-sixteen restriction for the encoded video size */
    /* scale the frame size up to the nearest /16 and calculate offsets */
    w = ((vob->ex_v_width  + 15) >> 4) << 4;
    h = ((vob->ex_v_height + 15) >> 4) << 4;
    /* We force the offset to be even.
       This ensures that the chroma samples align properly with the luma
       samples. */
    x_off = ((w - vob->ex_v_width ) /2) & ~1;
    y_off = ((h - vob->ex_v_height) /2) & ~1;

    theora_info_init(&ti);
    ti.width                        = w;
    ti.height                       = h;
    ti.frame_width                  = vob->ex_v_width;
    ti.frame_height                 = vob->ex_v_height;
    ti.offset_x                     = x_off;
    ti.offset_y                     = y_off;
    ret = tc_frc_code_to_ratio(vob->ex_frc,
                                    &ti.fps_numerator,
                                    &ti.fps_denominator);
                               /* watch out here */
    if (ret == TC_NULL_MATCH) {
        ti.fps_numerator            = 25;
        ti.fps_denominator          = 1;
    }
//    ti.aspect_numerator             = 1; // XXX
//    ti.aspect_denominator           = 1; // XXX
    ret = tc_find_best_aspect_ratio(vob,
                                    &ti.aspect_numerator,
                                    &ti.aspect_denominator,
                                    MOD_NAME);
    if (ret != TC_OK) {
        tc_log_error(MOD_NAME, "unable to find sane value for SAR");
        return TC_ERROR;
    }
    ti.colorspace                   = OC_CS_UNSPECIFIED;
    ti.pixelformat                  = OC_PF_420;
    ti.target_bitrate               = vob->divxbitrate;
    ti.quality                      = pd->quality;
    ti.dropframes_p                 = 0; // XXX
    ti.quick_p                      = pd->quick;
    ti.keyframe_auto_p              = 1; // XXX
    ti.keyframe_frequency           = vob->divxkeyframes;
    ti.keyframe_frequency_force     = vob->divxkeyframes;
    ti.keyframe_data_target_bitrate = vob->divxbitrate * 1.5;
    ti.keyframe_auto_threshold      = 80; // XXX
    ti.keyframe_mindistance         = 8;  // XXX
    ti.noise_sensitivity            = pd->nsens;
    ti.sharpness                    = pd->sharp;

    theora_encode_init(&(pd->td), &ti);
    theora_info_clear(&ti);

    pd->tbuf = vframe_alloc_single();
    if (pd->tbuf) {
        ret = tc_ogg_new_extradata(pd);
        if (ret == TC_OK) {
            /* publish it */
            vob->ex_v_xdata = &(pd->xdata);
        }
    }
    return ret;
}

static int tc_theora_stop(TCModuleInstance *self)
{
    TheoraPrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "stop");

    pd = self->userdata;

    tc_ogg_del_extradata(&pd->xdata);
    tc_del_video_frame(pd->tbuf);
    theora_clear(&(pd->td));
    return TC_OK;
}

static int tc_theora_encode_internal(TheoraPrivateData *pd, int eos,
                                     TCFrameVideo *inframe, TCFrameVideo *outframe)
{
    yuv_buffer yuv;
    ogg_packet op;
    int ret;

#ifdef TC_THEORA_DEBUG
    tc_log_info(MOD_NAME, "(%s) invoked eos=%i in=%p out=%p", __func__, eos, inframe, outframe);
#endif
    // FIXME
    yuv.y_width   = pd->tbuf->v_width;
    yuv.y_height  = pd->tbuf->v_height;
    yuv.y_stride  = pd->tbuf->v_width;

    // FIXME
    yuv.uv_width  = pd->tbuf->v_width/2;
    yuv.uv_height = pd->tbuf->v_height/2;
    yuv.uv_stride = pd->tbuf->v_width/2;

    // FIXME
    yuv.y         = pd->tbuf->video_buf;
    yuv.u         = yuv.y + yuv.y_width  * yuv.y_height;
    yuv.v         = yuv.u + yuv.uv_width * yuv.uv_height;

    theora_encode_YUVin(&(pd->td), &yuv);

    tc_frame_video_setup(outframe);

    do {
        ret = theora_encode_packetout(&(pd->td), eos, &op);
        if (ret > 0) {
            tc_frame_video_add_ogg_packet(pd, outframe, &op);
            pd->packets++;
        }
    } while (ret > 0);

    return TC_OK;
}

static int tc_theora_encode(TCModuleInstance *self,
                            TCFrameVideo *inframe, TCFrameVideo *outframe)
{
    TheoraPrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "encode_video");

    pd = self->userdata;

    outframe->video_len = 0; /* always needed */

    if (pd->frames == 0) {
        /* we need one frame for buffering */
        outframe->attributes |= TC_FRAME_IS_DELAYED;    
    } else {
        tc_theora_encode_internal(pd, 0, pd->tbuf, outframe);
    }
    vframe_copy(pd->tbuf, inframe, 1);
    pd->frames++;
#ifdef TC_THEORA_DEBUG
    tc_log_info(MOD_NAME, "(%s) after encoding: packets=%lu frames=%lu",
                          __func__,
                          (unsigned long)pd->packets,
                          (unsigned long)pd->frames);
#endif
    return TC_OK;
}

static int tc_theora_flush(TCModuleInstance *self, TCFrameVideo *frame)
{
    TheoraPrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "flush");

    pd = self->userdata;

    return tc_theora_encode_internal(pd, 1, pd->tbuf, frame);
}

static int tc_theora_encode_video(TCModuleInstance *self,
                                  TCFrameVideo *inframe, TCFrameVideo *outframe)
{
    TheoraPrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "encode_video");

    pd = self->userdata;

#ifdef TC_THEORA_DEBUG
    tc_log_info(MOD_NAME, "(%s) START ENCODE VIDEO FRAME", __func__);
    tc_log_info(MOD_NAME, "(%s) invoked in=%p out=%p", __func__, inframe, outframe);
#endif
    if (inframe == NULL && pd->flush_flag) {
        return tc_theora_flush(self, outframe); // FIXME
    }
    return tc_theora_encode(self, inframe, outframe);
}


#define INSPECT_PARAM(PARM, TYPE) do { \
    if (optstr_lookup(param, # PARM)) { \
        tc_snprintf(pd->conf_str, sizeof(pd->conf_str), \
                    "%s=" TYPE, # PARM, pd->PARM); \
        *value = pd->conf_str; \
        return TC_OK; \
    } \
} while (0)

static int tc_theora_inspect(TCModuleInstance *self,
                          const char *param, const char **value)
{
    TheoraPrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "inspect");

    pd = self->userdata;

    if (optstr_lookup(param, "help")) {
        *value = tc_theora_help;
    }
    INSPECT_PARAM(quality, "%i");
    INSPECT_PARAM(nsens,   "%i");
    INSPECT_PARAM(sharp,   "%i");
    INSPECT_PARAM(quick,   "%i");

    return TC_OK;
}

#undef INSPECT_PARAM

TC_MODULE_GENERIC_INIT(tc_theora, TheoraPrivateData);

TC_MODULE_GENERIC_FINI(tc_theora);

/*************************************************************************/

static const TCCodecID tc_theora_codecs_in[] = {
    TC_CODEC_YUV420P, TC_CODEC_ERROR
};
static const TCCodecID tc_theora_codecs_out[] = { 
    TC_CODEC_THEORA, TC_CODEC_ERROR
};
TC_MODULE_CODEC_FORMATS(tc_theora);

TC_MODULE_INFO(tc_theora);

static const TCModuleClass tc_theora_class = {
    TC_MODULE_CLASS_HEAD(tc_theora),

    .init         = tc_theora_init,
    .fini         = tc_theora_fini,
    .configure    = tc_theora_configure,
    .stop         = tc_theora_stop,
    .inspect      = tc_theora_inspect,

    .encode_video = tc_theora_encode_video,
};

TC_MODULE_ENTRY_POINT(tc_theora);

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

