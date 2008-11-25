/*
 * tcformats.c -- codecs helper functions.
 * (C) 2005-2008 - Francesco Romani <fromani -at- gmail -dot- com>
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


#include "libtc.h"
#include "src/transcode.h"
#include "tcformats.h"

#include "import/magic.h"

#include <string.h>

/* internal usage only ***************************************************/

typedef struct {
    TCFormatID id;            /* a TC_FORMAT_* value */
    const char *name;    
    int flags;
    const char *comment;
} TCFormatInfo;


/*
 * this table is *always* accessed in RO mode, so there is no need
 * to protect it with threading locks.
 */
static const TCFormatInfo tc_formats_info[] = {
    /* audio only */
    {
      TC_FORMAT_WAV, "wav", TC_AUDIO, 
      "WAV audio, PCM format",
    },
    { 
      TC_FORMAT_CDXA, "cdxa", TC_AUDIO,
      "CDXA audio format",
    },
    /* video only */
    { 
      TC_FORMAT_YUV4MPEG, "yuv4mpeg", TC_VIDEO, 
      "YUV4MPEG lightweight container (from mjpegtools)",
    },
    {
      TC_FORMAT_PVN, "pvn", TC_VIDEO,
      "PVN video format"
    },
    /* audio + video */
    { 
      TC_FORMAT_AVI, "avi", TC_AUDIO|TC_VIDEO,
      "Audio Video Interleaved",
    },
    { 
      TC_FORMAT_ASF, "asf", TC_AUDIO|TC_VIDEO,
      "Advanced Streaming Format",
    },
    { 
      TC_FORMAT_MOV, "mov", TC_AUDIO|TC_VIDEO,
      "Quicktime's MOV format",
    },
    { 
      TC_FORMAT_OGG, "ogg", TC_AUDIO|TC_VIDEO|TC_EXTRA,
      "Xiph's ogg encapsulation",
    },
    { 
      TC_FORMAT_MPEG_PS, "mpeg-ps", TC_AUDIO|TC_VIDEO|TC_EXTRA,
      "MPEG Program Stream",
    },
    { 
      TC_FORMAT_MPEG_TS, "mpeg-ts", TC_AUDIO|TC_VIDEO|TC_EXTRA,
      "MPEG Transport Stream",
    },
    { 
      TC_FORMAT_MPEG_VOB, "vob", TC_AUDIO|TC_VIDEO|TC_EXTRA,
      "MPEG VOB encapsulation",
    },
    { 
      TC_FORMAT_MPEG_VDR, "vdr", TC_AUDIO|TC_VIDEO|TC_EXTRA,
      "VDR MPEG format",
    },
    { 
      TC_FORMAT_MPEG_MP4, "mp4", TC_AUDIO|TC_VIDEO|TC_EXTRA,
      "MP4 container (system) format",
    },
    { 
      TC_FORMAT_MXF, "mxf", TC_AUDIO|TC_VIDEO,
      "Media eXchangeFormat",
    },
    {
      TC_FORMAT_PV3, "pv3", TC_AUDIO|TC_VIDEO,
      "Earth soft PV3"
    },
    { 
      TC_FORMAT_VAG, "vag", TC_AUDIO,
      "(PS) VAG format audio"
    },
    { 
      TC_FORMAT_NUV, "nuv", TC_AUDIO|TC_VIDEO,
      "NuppelVideo format",
    },
    { 
      TC_FORMAT_FLV, "flv", TC_AUDIO|TC_VIDEO,
      "standalone Flash Video",
    },
    /* pseudo-formats */
    { TC_FORMAT_RAW, "raw", TC_AUDIO|TC_VIDEO,
      "raw (unpacked) A/V stream",
    },
    {
      TC_FORMAT_ALSA, "alsa", TC_AUDIO,
      "ALSA device audio source"
    },
    { 
      TC_FORMAT_X11, "x11", TC_VIDEO,
      "X11 frame grabbing source"
    },
    { 
      TC_FORMAT_XML, "xml", TC_AUDIO|TC_VIDEO,
      "XML custom stream representation" 
    },
    {
      TC_FORMAT_VIDEO4LINUX, "v4l", TC_VIDEO,
      "video4linux device source"
    },
    {
      TC_FORMAT_OSS, "oss", TC_AUDIO,
      "Open Sound System audio"
    },
    {
      TC_FORMAT_BKTR, "bktr", TC_VIDEO,
      "BSD brooktree caputre devices"
    },
    {
      TC_FORMAT_VNC, "vnc", TC_VIDEO,
      "VNC frame grabbing source"
    },
    { 
      TC_FORMAT_DVD, "dvd", TC_AUDIO|TC_VIDEO|TC_EXTRA,
      "dvd device data source"
    },
    /* special formats */
    { 
      TC_FORMAT_UNKNOWN, "unknown", 0, 
      "format (yet) unknown",
    },
    { 
      TC_FORMAT_NULL, "null", TC_AUDIO|TC_VIDEO,
      "discard frames",
    }, 
    { 
      TC_FORMAT_ANY, "everything", TC_AUDIO|TC_VIDEO|TC_EXTRA,
      "anything is fine",
    },
    { 
      TC_FORMAT_ERROR, "error", 0,
      "erroneous fake format",
    },
    /* this MUST be the last one */
};

/* compatibility */
int tc_magic_to_format(int magic)
{
    switch (magic) {
      case TC_MAGIC_TS:        return TC_FORMAT_MPEG_TS;
      case TC_MAGIC_YUV4MPEG:  return TC_FORMAT_YUV4MPEG;
      case TC_MAGIC_NUV:       return TC_FORMAT_NUV;
      case TC_MAGIC_DVD_PAL:   /* fallback */
      case TC_MAGIC_DVD_NTSC:  return TC_FORMAT_DVD;
      case TC_MAGIC_AVI:       return TC_FORMAT_AVI;
      case TC_MAGIC_MOV:       return TC_FORMAT_MOV;
      case TC_MAGIC_XML:       return TC_FORMAT_XML;
      case TC_MAGIC_TIFF1:     /* fallback */
      case TC_MAGIC_TIFF2:     /* fallback */
      case TC_MAGIC_JPEG:      /* fallback */
      case TC_MAGIC_BMP:       /* fallback */
      case TC_MAGIC_PNG:       /* fallback */
      case TC_MAGIC_GIF:       /* fallback */
      case TC_MAGIC_PPM:       /* fallback */
      case TC_MAGIC_PGM:       return TC_FORMAT_RAW;
      /* compressed images are considered formatless encoded frames */ 
      case TC_MAGIC_AC3:       return TC_FORMAT_RAW;
      /* unpacked AC-3 stream */
      case TC_MAGIC_MP3:       /* fallback */
      case TC_MAGIC_MP2:       return TC_FORMAT_RAW;
      /* unpacked MPEG audio streams */
      case TC_MAGIC_CDXA:      return TC_FORMAT_CDXA;
      case TC_MAGIC_OGG:       return TC_FORMAT_OGG;
      case TC_MAGIC_WAV:       return TC_FORMAT_WAV;
      case TC_MAGIC_V4L_AUDIO: return TC_FORMAT_VIDEO4LINUX;
      case TC_MAGIC_PVN:       return TC_FORMAT_PVN;
      default:                 return TC_FORMAT_ERROR; /* can't happen */
    }
    return TC_FORMAT_ERROR;
}


/*
 * TCFormatMatcher:
 *      generic format finder function family.
 *      tell if a TCFormatInfo descriptor match certains given criterias
 *      using a function-dependent method.
 *      See also 'find_tc_format' function.
 *
 * Parameters:
 *      info: a pointer to a TCFormatInfo descriptor to be examinated
 *  userdata: a pointer to data with function-dependent meaning
 * Return Value:
 *      TC_TRUE if function succeed,
 *      TC_FALSE otherwise.
 */
typedef int (*TCFormatMatcher)(const TCFormatInfo *info, const void *userdata);

/*
 * id_matcher:
 *      match a TCFormatInfo descriptor on codec's id.
 *      'userdata' must be an *address* of an uint32_t containing a TC_FORMAT_*
 *      to match.
 *
 * Parameters:
 *      as for TCFormatMatcher
 * Return Value:
 *      as for TCFormatMatcher
 */
static int id_matcher(const TCFormatInfo *info, const void *userdata)
{
    if (info == NULL || userdata == NULL) {
        return TC_FALSE;
    }
    return (*((TCFormatID*)userdata) == info->id) ?TC_TRUE :TC_FALSE;
}

/*
 * name_matcher:
 *      match a TCFormatInfo descriptor on codec's name (note: note != fourcc).
 *      'userdata' must be the C-string to match.
 *      Note: ignore case.
 *
 * Parameters:
 *      as for TCFormatMatcher
 * Return Value:
 *      as for TCFormatMatcher
 */
static int name_matcher(const TCFormatInfo *info, const void *userdata)
{
    if (info == NULL || userdata == NULL) {
        return TC_FALSE;
    }
    if(!info->name || (strcasecmp(info->name, userdata) != 0)) {
        return TC_FALSE;
    }
    return TC_TRUE;
}

/*
 * find_tc_format:
 *      find a TCFormatInfo descriptor matching certains given criterias.
 *      It scans the whole TCFormatInfos table applying the given
 *      matcher with the given data to each element, halting when a match
 *      is found
 *
 * Parameters:
 *      matcher: a TCFormatMatcher to be applied to find the descriptor.
 *     userdata: matching data to be passed to matcher together with a table
 *               entry.
 *
 * Return Value:
 *      >= 0: index of an entry in TCFormatInfo in table if an entry match
 *            the finding criteria
 *      TC_NULL_MATCH if no entry matches the given criteria
 */
static int find_tc_format(const TCFormatInfo *infos,
                          TCFormatMatcher matcher,
                          const void *userdata)
{
    int found = TC_FALSE, i = 0;

    if (infos == NULL) {
        return TC_NULL_MATCH;
    }

    for (i = 0; infos[i].id != TC_FORMAT_ERROR; i++) {
        found = matcher(&infos[i], userdata);
        if (found) {
            break;
        }
    }
    if (!found) {
        i = TC_NULL_MATCH;
    }

    return i;
}

/* public API ************************************************************/

const char* tc_format_to_string(TCFormatID format)
{
    int idx = find_tc_format(tc_formats_info, id_matcher, &format);

    if (idx == TC_NULL_MATCH) { /* not found */
        return "unknown";
    }
    return tc_formats_info[idx].name; /* can be NULL */
}

TCFormatID tc_format_from_string(const char *codec)
{
    int idx = find_tc_format(tc_formats_info, name_matcher, codec);

    if (idx == TC_NULL_MATCH) { /* not found */
        return TC_FORMAT_ERROR;
    }
    return tc_formats_info[idx].id;
}

const char* tc_format_to_comment(TCFormatID codec)
{
    int idx = find_tc_format(tc_formats_info, id_matcher, &codec);

    if (idx == TC_NULL_MATCH) { /* not found */
        return "unknown";
    }
    return tc_formats_info[idx].comment; /* can be NULL */
}


int tc_format_description(TCFormatID format, char *buf, size_t bufsize)
{
    char storage[TC_BUF_MIN] = { '\0' };
    int idx = find_tc_format(tc_formats_info, id_matcher, &format);
    int ret, flags = 0;

    if (idx == TC_NULL_MATCH) { /* not found */
        strlcpy(buf, "unknwon", bufsize);
        return -1;
    }

    flags = tc_formats_info[idx].flags;
    tc_snprintf(storage, sizeof(storage), "%s%s|%s|%s%s",
                (flags           ) ?"("     :"",
                (flags & TC_VIDEO) ?"video" :"",
                (flags & TC_AUDIO) ?"audio" :"",
                (flags & TC_EXTRA) ?"extra" :"",
                (flags           ) ?")"     :"");

    ret = tc_snprintf(buf, bufsize, "%-12s: %-20s %s",
                      tc_formats_info[idx].name,
                      storage,
                      tc_formats_info[idx].comment);
    return ret;
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
