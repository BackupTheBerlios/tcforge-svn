/*
 * multiplex_ogg.c -- write OGG streams using libogg.
 * (C) 2007-2008 Francesco Romani <fromani at gmail dot com>
 *
 * This file is part of transcode, a video stream processing tool
 *
 * transcode is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * transcode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "src/transcode.h"

#include "libtcutil/optstr.h"
#include "libtcutil/cfgfile.h"

#include "libtcmodule/tcmodule-plugin.h"

#include "libtcext/tc_ogg.h"

#define MOD_NAME    "multiplex_ogg.so"
#define MOD_VERSION "v0.1.0 (2008-01-01)"
#ifdef HAVE_SHOUT
#define MOD_CAP     "create an ogg stream using libogg and broadcast using libshout"
#else  /* not HAVE_SHOUT */
#define MOD_CAP     "create an ogg stream using libogg"
#endif

#define MOD_FEATURES \
    TC_MODULE_FEATURE_MULTIPLEX|TC_MODULE_FEATURE_VIDEO|TC_MODULE_FEATURE_AUDIO

#define MOD_FLAGS \
    TC_MODULE_FLAG_RECONFIGURABLE

//#define TC_OGG_DEBUG 1 // until 0.x.y at least

/*************************************************************************/

#ifdef HAVE_SHOUT
#include <shout/shout.h>
#endif

typedef struct tcshout_ TCShout;
struct tcshout_ {
    void *sh;

    int (*open)(TCShout *tcsh);
    int (*close)(TCShout *tcsh);
    int (*send)(TCShout *tcsh, const unsigned char *data, size_t len);
    void (*free)(TCShout *tcsh);
};


#ifdef HAVE_SHOUT

#define TC_SHOUT_BUF 512
#define TC_SHOUT_CONFIG_FILE "shout.cfg"

#define RETURN_IF_SHOUT_ERROR(MSG) do { \
    if (ret != SHOUTERR_SUCCESS) { \
        tc_log_error(MOD_NAME, "%s: %s", (MSG), shout_get_error(shout)); \
	    return TC_ERROR; \
    } \
} while (0)


static int tc_shout_configure(TCShout *tcsh, const char *id)
{
    char *hostname = NULL;
    char *mount = NULL;
    char *url = NULL;
    char *password = NULL;
    char *description = NULL;
    char *genre = NULL;
    char *name = NULL;
    int port = 0, public = 1;

    TCConfigEntry shout_conf[] = {
        { "host", &hostname, TCCONF_TYPE_STRING, 0, 0, 0 },
        { "port", &port, TCCONF_TYPE_INT, TCCONF_FLAG_RANGE, 1, 65535 },
        { "password", &password, TCCONF_TYPE_STRING, 0, 0, 0 },
        { "mount",  &mount, TCCONF_TYPE_STRING, 0, 0, 0 },
        { "public", &public, TCCONF_TYPE_FLAG, 0, 0, 1 },
        { "description", &description, TCCONF_TYPE_STRING, 0, 0, 0 },
        { "genre", &genre, TCCONF_TYPE_STRING, 0, 0, 0 },
        { "name", &name, TCCONF_TYPE_STRING, 0, 0, 0 },
        { "url", &url, TCCONF_TYPE_STRING, 0, 0, 0 },
        { NULL, 0, 0, 0, 0, 0 }
    };

    if (tcsh->sh) {
        shout_t *shout =  tcsh->sh;
        int ret = SHOUTERR_SUCCESS;

        if (verbose) {
            tc_log_info(MOD_NAME,
                        "reading configuration data for stream '%s'...", id);
        }
        module_read_config(TC_SHOUT_CONFIG_FILE, id, shout_conf, MOD_NAME);

	    shout_set_format(shout, SHOUT_FORMAT_VORBIS); /* always true in here */
	    shout_set_public(shout, public); /* first the easy stuff */

        if (verbose) {
            tc_log_info(MOD_NAME, "sending to [%s:%i%s] (%s)",
                        hostname, port, mount,
                        (public) ?"public" :"private");
        }
        ret = shout_set_host(shout, hostname);
        RETURN_IF_SHOUT_ERROR("invalid SHOUT hostname");

	    ret =  shout_set_port(shout, port);
        RETURN_IF_SHOUT_ERROR("invalid SHOUT port");

        ret = shout_set_mount(shout, mount);
        RETURN_IF_SHOUT_ERROR("invalid SHOUT mount");

    	ret = shout_set_password(shout, password);
        RETURN_IF_SHOUT_ERROR("invalid SHOUT password");

    	if (description)
	    	shout_set_description(shout, description);

	    if (genre)
		    shout_set_genre(shout, genre);

    	if (name)
	    	shout_set_name(shout, name);

    	if (url)
	    	shout_set_url(shout, url);
    }
    return TC_OK;
}
#endif

static int tc_shout_real_open(TCShout *tcsh)
{
#ifdef HAVE_SHOUT
    shout_t *shout = tcsh->sh;
    int ret = shout_open(shout);
    RETURN_IF_SHOUT_ERROR("connecting to SHOUT server");
#endif
    return TC_OK;
}

static int tc_shout_real_close(TCShout *tcsh)
{ 
#ifdef HAVE_SHOUT
    shout_t *shout = tcsh->sh;
    shout_close(shout);
#endif
    return TC_OK;
}

static int tc_shout_real_send(TCShout *tcsh, const unsigned char *data, size_t len)
{ 
#ifdef HAVE_SHOUT
    shout_t *shout = tcsh->sh;
    int ret = shout_send(shout, data, len);
    RETURN_IF_SHOUT_ERROR("sending data to SHOUT server");
    shout_sync(shout);
#endif
    return TC_OK;
}

static void tc_shout_real_free(TCShout *tcsh)
{
#ifdef HAVE_SHOUT
    shout_free(tcsh->sh);
    tcsh->sh = NULL;
#endif
}


static int tc_shout_real_new(TCShout *tcsh, const char *id)
{ 
    int ret = TC_OK;

    tcsh->sh    = NULL;

    tcsh->open  = tc_shout_real_open;
    tcsh->close = tc_shout_real_close;
    tcsh->send  = tc_shout_real_send;
    tcsh->free  = tc_shout_real_free;
        
#ifdef HAVE_SHOUT
    tcsh->sh = shout_new();

    if (tcsh->sh) {
        ret = tc_shout_configure(tcsh, id);
    }
#endif
    return ret;
}

/*************************************************************************/

static int tc_shout_null_open(TCShout *tcsh)
{ 
    return TC_OK;
}

static int tc_shout_null_close(TCShout *tcsh)
{ 
    return TC_OK;
}

static int tc_shout_null_send(TCShout *tcsh, const unsigned char *data, size_t len)
{ 
    return TC_OK;
}

static void tc_shout_null_free(TCShout *tcsh)
{ 
    ; /* do nothing ... */
}


static int tc_shout_null_new(TCShout *tcsh, const char *id)
{
    if (tcsh) {

        tcsh->open  = tc_shout_null_open;
        tcsh->close = tc_shout_null_close;
        tcsh->send  = tc_shout_null_send;
        tcsh->free  = tc_shout_null_free;
        
        tcsh->sh    = NULL;

        return TC_OK;
    }
    return TC_ERROR;
}


/*************************************************************************/


static const char tc_ogg_help[] = ""
    "Overview:\n"
    "    this module create an OGG stream using libogg.\n"
    "Options:\n"
    "    stream  enable shout streaming using given label as identifier\n"
    "    help    produce module overview and options explanations\n";

static const TCCodecID tc_ogg_codecs_in[] = {
    TC_CODEC_THEORA, TC_CODEC_VORBIS, TC_CODEC_ERROR
};

typedef struct oggprivatedata_ OGGPrivateData;
struct oggprivatedata_ {
    ogg_stream_state vs; /* video stream */
    ogg_stream_state as; /* audio stream */
    FILE* outfile;

    TCShout tcsh;
    int shouting;        /* flag */
};

/*************************************************************************/

static int tc_ogg_feed_video(ogg_stream_state *os, TCFrameVideo *f)
{
    int16_t *pkt_num = (int16_t*)f->video_buf;
    int i, packets = *pkt_num;
    uint8_t *data = f->video_buf + sizeof(*pkt_num);
    ogg_packet op;
    
    for (i = 0; i < packets; i++) {
        ac_memcpy(&op, data, sizeof(op));
        data += sizeof(op);
        op.packet = data;
        data += op.bytes;
        
        ogg_stream_packetin(os, &op);
    }
    return i;
}

static int tc_ogg_feed_audio(ogg_stream_state *os, TCFrameAudio *f)
{
    int16_t *pkt_num = (int16_t*)f->audio_buf;
    int i, packets = *pkt_num;
    uint8_t *data = f->audio_buf + sizeof(*pkt_num);
    ogg_packet op;
    
    for (i = 0; i < packets; i++) {
        ac_memcpy(&op, data, sizeof(op));
        data += sizeof(op);
        op.packet = data;
        data += op.bytes;
        
        ogg_stream_packetin(os, &op);
    }
    return i;
}

/*************************************************************************/

static int is_supported(const TCCodecID *codecs, TCCodecID wanted)
{
    int i = 0, found  = TC_FALSE;
    for (i = 0; !found && codecs[i] != TC_CODEC_ERROR; i++) {
        if (codecs[i] == wanted) {
            found = TC_TRUE;
        }
    }
    return found;
}

static int tc_ogg_send(ogg_stream_state *os, FILE *f, TCShout *tcsh,
                       int (*ogg_send)(ogg_stream_state *os, ogg_page *og))
{
    int32_t bytes = 0;
    ogg_page og;
    int ret;

#ifdef TC_OGG_DEBUG
    tc_log_info(MOD_NAME, "(%s) begin", __func__);
#endif
    while (TC_TRUE) {
        ret = ogg_send(os, &og);
        if (ret == 0) {
            break;
        }
        fwrite(og.header, 1, og.header_len, f);
        tcsh->send(tcsh, og.header, og.header_len);
        bytes += og.header_len;
        fwrite(og.body,   1, og.body_len,   f);
        tcsh->send(tcsh, og.body, og.body_len);
        bytes += og.body_len;

#ifdef TC_OGG_DEBUG
        tc_log_info(MOD_NAME, "(%s) sent hlen=%lu blen=%lu gpos=%lu pkts=%i",
                    __func__,
                    (unsigned long)og.header_len,
                    (unsigned long)og.body_len,
                    (unsigned long)ogg_page_granulepos(&og),
                                   ogg_page_packets(&og));
#endif
    }
#ifdef TC_OGG_DEBUG
    tc_log_info(MOD_NAME, "(%s) end", __func__);
#endif
    return bytes;
}

static int tc_ogg_flush(ogg_stream_state *os, FILE *f, TCShout *tcsh)
{
    return tc_ogg_send(os, f, tcsh, ogg_stream_flush);
}

static int tc_ogg_write(ogg_stream_state *os, FILE *f, TCShout *tcsh)
{
    return tc_ogg_send(os, f, tcsh, ogg_stream_pageout);
}

#define RETURN_IF_ERROR(ret) do { \
    if ((ret) == TC_ERROR) { \
        return ret; \
    } \
} while (0)

#define SETUP_STREAM_HEADER(OS, XD, F, TCSH) do { \
    int ret; \
    if ((XD)) { \
        ogg_stream_packetin((OS), &((XD)->header)); \
        ret = tc_ogg_flush((OS), (F), (TCSH)); \
        RETURN_IF_ERROR(ret); \
    } \
} while (0)

#define SETUP_STREAM_METADATA(OS, XD, F, TCSH) do { \
    int ret; \
    if ((XD)) { \
        ogg_stream_packetin((OS), &((XD)->comment)); \
        ogg_stream_packetin((OS), &((XD)->code)); \
        ret = tc_ogg_flush((OS), (F), (TCSH)); \
        RETURN_IF_ERROR(ret); \
    } \
} while (0)

#define RETURN_IF_NOT_SUPPORTED(XD, MSG) do { \
    if ((XD)) { \
        if (!is_supported(tc_ogg_codecs_in, (XD)->magic)) { \
            tc_log_error(MOD_NAME, (MSG)); \
            return TC_ERROR; \
        } \
    } \
} while (0)

static int tc_ogg_setup(OGGPrivateData *pd,
                        OGGExtraData *vxd, OGGExtraData *axd)
{
    RETURN_IF_NOT_SUPPORTED(vxd, "unrecognized video extradata");
    RETURN_IF_NOT_SUPPORTED(axd, "unrecognized audio extradata");

    SETUP_STREAM_HEADER(&(pd->vs),   vxd, pd->outfile, &(pd->tcsh));
    SETUP_STREAM_HEADER(&(pd->as),   axd, pd->outfile, &(pd->tcsh));

    SETUP_STREAM_METADATA(&(pd->vs), vxd, pd->outfile, &(pd->tcsh));
    SETUP_STREAM_METADATA(&(pd->as), axd, pd->outfile, &(pd->tcsh));

    return TC_OK;
}


/*************************************************************************/

static int tc_ogg_inspect(TCModuleInstance *self,
                          const char *param, const char **value)
{
    TC_MODULE_SELF_CHECK(self, "inspect");

    if (optstr_lookup(param, "help")) {
        *value = tc_ogg_help;
    }

    return TC_OK;
}

static int tc_ogg_configure(TCModuleInstance *self,
                            const char *options, vob_t *vob)
{
    char shout_id[128] = { '\0' };
    OGGPrivateData *pd = NULL;
    int ret, aserial, vserial, streamed = 0;
  
    TC_MODULE_SELF_CHECK(self, "configure");

    pd = self->userdata;

    pd->shouting = 0;

    if (options) {
        int dest = optstr_get(options, "stream", "%127s", shout_id);
        shout_id[127] = '\0'; /* paranoia keep us alive */
        if (dest == 1) {
            /* have a shout_id? */
            streamed = 1;
        }
    }

    if (streamed) {
        ret = tc_shout_real_new(&pd->tcsh, shout_id);
    } else {
        ret = tc_shout_null_new(&pd->tcsh, shout_id);
    }
    if (ret != TC_OK) {
        tc_log_error(MOD_NAME, "failed initializing SHOUT streaming support");
        return TC_ERROR;
    }
    pd->shouting = 1;

    srand(time(NULL));
    /* need two inequal serial numbers */
    vserial = rand();
    aserial = rand();
    if (aserial == vserial) {
        vserial++;
    }
    ogg_stream_init(&(pd->vs), vserial);
    ogg_stream_init(&(pd->as), aserial);

    pd->outfile = fopen(vob->video_out_file, "wb");
    if (!pd->outfile) {
        tc_log_perror(MOD_NAME, "open output file");
        return TC_ERROR;
    }

    ret = pd->tcsh.open(&pd->tcsh);
    if (ret != TC_OK) {
        tc_log_error(MOD_NAME, "opening SHOUT connection");
        return TC_ERROR;
    }
    return tc_ogg_setup(pd, vob->ex_v_xdata, vob->ex_a_xdata);
}

static int tc_ogg_stop(TCModuleInstance *self)
{
    OGGPrivateData *pd = NULL;

    TC_MODULE_SELF_CHECK(self, "inspect");

    pd = self->userdata;

    ogg_stream_clear(&(pd->vs));
    ogg_stream_clear(&(pd->as));
    if (pd->outfile) {
        int err = fclose(pd->outfile);
        if (err) {
            return TC_ERROR;
        }
        pd->outfile = NULL;
    }

    if (pd->shouting) {
        pd->tcsh.close(&pd->tcsh);
        pd->tcsh.free(&pd->tcsh);
        pd->shouting = 0;
    }
    return TC_OK;
}

/* FIXME: merge */
#define HAVE_VFRAME(VF) ((VF) && ((VF)->video_len > 0))
#define HAVE_AFRAME(AF) ((AF) && ((AF)->audio_len > 0))

static int tc_ogg_multiplex(TCModuleInstance *self,
                            TCFrameVideo *vframe, TCFrameAudio *aframe)
{
    int has_vid = HAVE_VFRAME(vframe), has_aud = HAVE_AFRAME(aframe);
    OGGPrivateData *pd = NULL;
    int ret = 0;

    TC_MODULE_SELF_CHECK(self, "inspect");

    pd = self->userdata;

    /* yes, this stinks. It needs to be improved. */

    if (has_aud && !has_vid) {
        tc_ogg_feed_audio(&(pd->as), aframe);
        ret = tc_ogg_write(&(pd->as), pd->outfile, &(pd->tcsh));
    } else if(has_vid && !has_aud) {
        tc_ogg_feed_video(&(pd->vs), vframe);
        ret = tc_ogg_write(&(pd->vs), pd->outfile, &(pd->tcsh));
    } else if (has_vid && has_aud) {
        /* which is earlier; the end of the audio page or the end of the
           video page? Flush the earlier to stream */
        double ats = TC_FRAME_GET_TIMESTAMP_DOUBLE(aframe);
        double vts = TC_FRAME_GET_TIMESTAMP_DOUBLE(vframe);
        int aret = 0, vret = 0;
 
        if (vts < ats) {
            tc_ogg_feed_video(&(pd->vs), vframe);
            vret = tc_ogg_write(&(pd->vs), pd->outfile, &(pd->tcsh));
            RETURN_IF_ERROR(vret);

            tc_ogg_feed_audio(&(pd->as), aframe);
            aret = tc_ogg_write(&(pd->as), pd->outfile, &(pd->tcsh));
            RETURN_IF_ERROR(aret);
        } else {
            tc_ogg_feed_audio(&(pd->as), aframe);
            aret = tc_ogg_write(&(pd->as), pd->outfile, &(pd->tcsh));
            RETURN_IF_ERROR(aret);

            tc_ogg_feed_video(&(pd->vs), vframe);
            vret = tc_ogg_write(&(pd->vs), pd->outfile, &(pd->tcsh));
            RETURN_IF_ERROR(vret);
        }
        ret = aret + vret;
    }
#ifdef TC_OGG_DEBUG
    tc_log_info(MOD_NAME, "(%s) tc_ogg_multiplex(has_vid=%i, has_aud=%i)->%i",
                __func__, has_vid, has_aud, ret);
#endif
    return ret;
}

TC_MODULE_GENERIC_INIT(tc_ogg, OGGPrivateData);

TC_MODULE_GENERIC_FINI(tc_ogg);

/*************************************************************************/

static const TCFormatID tc_ogg_formats_out[] = { 
    TC_FORMAT_OGG, TC_FORMAT_ERROR
};
/* a multiplexor is at the end of pipeline */
TC_MODULE_MPLEX_FORMATS_CODECS(tc_ogg);

TC_MODULE_INFO(tc_ogg);

static const TCModuleClass tc_ogg_class = {
    TC_MODULE_CLASS_HEAD(tc_ogg),

    .init         = tc_ogg_init,
    .fini         = tc_ogg_fini,
    .configure    = tc_ogg_configure,
    .stop         = tc_ogg_stop,
    .inspect      = tc_ogg_inspect,

    .multiplex    = tc_ogg_multiplex,
};

TC_MODULE_ENTRY_POINT(tc_ogg);

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

