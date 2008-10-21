/*
 *  tclog.c - transcode logging infrastructure (implementation)
 *  Written by Thomas Oestreich, Francesco Romani, Andrew Church, and others
 *
 *  This file is part of transcode, a video stream processing tool.
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include "common.h"
#include "strutils.h"
#include "logging.h"


/*************************************************************************/

#define TC_LOG_BUF_SIZE         TC_BUF_LINE

#define TC_LOG_MAX_METHODS      (8)


/*************************************************************************/

/* colors macros */
#define COL(x)              "\033[" #x ";1m"
#define COL_RED             COL(31)
#define COL_GREEN           COL(32)
#define COL_YELLOW          COL(33)
#define COL_BLUE            COL(34)
#define COL_WHITE           COL(37)
#define COL_GRAY            "\033[0m"

static const char *log_template(TCLogLevel level)
{
    /* WARNING: we MUST keep in sync templates order with TC_LOG* macros */
    static const char *tc_log_templates[] = {
        /* TC_LOG_ERR */
        "["COL_RED"%s"COL_GRAY"]"COL_RED" critical"COL_GRAY": %s\n",
        /* TC_LOG_WARN */
        "["COL_RED"%s"COL_GRAY"]"COL_YELLOW" warning"COL_GRAY": %s\n",
        /* TC_LOG_INFO */
        "["COL_BLUE"%s"COL_GRAY"] %s\n",
        /* TC_LOG_MSG */
        "[%s] %s\n",
        /* TC_LOG_MARK */
        "%s%s" /* tag placeholder must be present but tag will be ignored */
    };
    return tc_log_templates[level - TC_LOG_ERR];
}

static int tc_log_console_send(TCLogContext *ctx, TCLogLevel level,
                               const char *tag, const char *fmt, va_list ap)
{
    int is_dynbuf = TC_FALSE, truncated = TC_FALSE;
    /* flag: we must use a dynamic (larger than static) buffer? */
    char buf[TC_LOG_BUF_SIZE];
    char *msg = buf;
    size_t size = sizeof(buf);
    const char *templ = NULL;

    /* sanity check, avoid {under,over}flow; */
    level = (level < TC_LOG_ERR) ?TC_LOG_ERR :level;
    level = (level > TC_LOG_MARK) ?TC_LOG_MARK :level;
    /* sanity check, avoid dealing with NULL as much as we can */
    if (!ctx->use_colors && level != TC_LOG_MARK) {
        level = TC_LOG_MSG;
    }

    tag = (tag != NULL) ?tag :"";
    fmt = (fmt != NULL) ?fmt :"";
    /* TC_LOG_EXTRA special handling: force always empty tag */
    tag = (level == TC_LOG_MARK) ?"" :tag;
    templ = log_template(level);
    
    size = strlen(templ) + strlen(tag) + strlen(fmt) + 1;

    if (size > sizeof(buf)) {
        /* 
         * we use malloc/fprintf instead of tc_malloc because
         * we want custom error messages
         */
        msg = malloc(size);
        if (msg != NULL) {
            is_dynbuf = TC_TRUE;
        } else {
            fprintf(stderr, "(%s) CRITICAL: can't get memory in "
                    "tc_log() output will be truncated.\n",
                    __FILE__);
            /* force reset to default values */
            msg = buf;
            size = sizeof(buf) - 1;
            truncated = TC_TRUE;
        }
    } else {
        size = sizeof(buf) - 1;
    }

    /* construct real format string */
    tc_snprintf(msg, size, templ, tag, fmt);

    vfprintf(ctx->f, msg, ap);

    if (is_dynbuf) {
        free(msg);
    }

    /* ensure that all *other* messages are written */
    /* we don't (yet) honor the flush_threshold */
    fflush(ctx->f);
    return (truncated) ?-1 :0;
}

static int tc_log_console_close(TCLogContext *ctx)
{
    return TC_OK;
}


static int tc_log_console_open(TCLogContext *ctx, int *argc, char ***argv)
{
    if (ctx) {
        int ret = tc_mangle_cmdline(argc, argv, TC_LOG_COLOR_OPTION, NULL);

        if (ret == 0) {
            ctx->use_colors = TC_FALSE;
        } else {
            const char *envvar = getenv(TC_LOG_COLOR_ENV_VAR);
            if (envvar != NULL) {
                ctx->use_colors = TC_FALSE;
            }
        }

        ctx->send  = tc_log_console_send;
        ctx->close = tc_log_console_close;
        return TC_OK;
    }
    return TC_ERROR;
}


/*************************************************************************/


struct tclogmethod {
    TCLogMode       mode;
    TCLogMethodOpen open;
};


static struct tclogmethod methods[TC_LOG_MAX_METHODS] = {
    { TC_LOG_CONSOLE, tc_log_console_open },
    { TC_LOG_INVALID, NULL                }
};
static int last_method = 1;

static TCLogContext TCLog;


/*************************************************************************/

int tc_log_register_method(TCLogMode mode, TCLogMethodOpen open)
{
    int ret = TC_ERROR;

    if (last_method < TC_LOG_MAX_METHODS) {
        methods[last_method].mode = mode;
        methods[last_method].open = open;
        last_method++;

        ret = TC_OK;
    }
    return ret;
}

/*************************************************************************/

int tc_log_open(TCLogMode mode, TCVerboseLevel verbose,
                int *argc, char ***argv)
{
    int i = 0, ret = TC_ERROR;

    TCLog.verbose     = verbose;
    TCLog.use_colors  = TC_FALSE;
    TCLog.f           = stderr;
    TCLog.log_count   = 0;
    TCLog.flush_thres = 1;

    for (i = 0; methods[i].mode != TC_LOG_INVALID; i++) {
        if (mode == methods[i].mode) {
            ret = methods[i].open(&TCLog, argc, argv);
            break;
        }
    }
    return ret;
}
 

int tc_log_close(void)
{
    return TCLog.close(&TCLog);
}

int tc_log(TCLogLevel level, TCVerboseLevel verbose,
           const char *tag, const char *fmt, ...)
{
    int ret = 0;

    if (TCLog.verbose >= verbose) {
        va_list ap;

        va_start(ap, fmt);
        ret = TCLog.send(&TCLog, level, tag, fmt, ap);
        va_end(ap);
    }
    return ret;
}

/*************************************************************************/

#define TC_DEBUG_ENVVAR     "TC_DEBUG"
static TCDebugSource debug_sources = 0; /* nothing by default */

struct debugflag {
    const char *name;
    TCDebugSource flag;
};

static const struct debugflag DebugFlags[] = {
    { "CLEANUP",   TC_CLEANUP },
    { "FRAMELIST", TC_FLIST   },
    { "SYNC",      TC_SYNC    },
    { "COUNTER",   TC_COUNTER },
    { "PRIVATE",   TC_PRIVATE },
    { "THREADS",   TC_THREADS },
    { "WATCH",     TC_WATCH   },
    { NULL,        0          }
};

static int tc_log_debug_init(const char *envname)
{
    const char *envvar = getenv(envname);
    if (envvar) {
        size_t i = 0, n = 0;
        char **tokens = tc_strsplit(envvar, ',', &n);
        int j = 0;

        if (tokens) {
            for (i = 0; i < n; i++) {
                for (j = 0; DebugFlags[j].name != NULL; j++) {
                    if (strcmp(DebugFlags[j].name, tokens[i]) == 0) {
                        debug_sources |= DebugFlags[j].flag;
                 }
                }
            }

            tc_strfreev(tokens);
        }
    }
    return TC_OK;
}

int tc_log_debug(TCDebugSource src, const char *tag, const char *fmt, ...)
{
    int ret = 0;

    if (debug_sources & src) {
        va_list ap;

        va_start(ap, fmt);
        ret = TCLog.send(&TCLog, TC_LOG_MSG, tag, fmt, ap);
        /* always as message */
        va_end(ap);
    }
    return ret;
}


/*************************************************************************/


int tc_log_init(void)
{
    return tc_log_debug_init(TC_DEBUG_ENVVAR);
}

int tc_log_fini(void)
{
    return TC_OK; /* do nothing, yet */
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

