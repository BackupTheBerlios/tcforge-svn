/*
 * logging_file.c -- file backend for transcode logging infrastructure.
 * (C) 2008 Francesco Romani <fromani at gmail dot com>
 *
 * This file is part of tcrund, the transcode remote control daemon.
 *
 * tcrund is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * tcrund is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "tcutil.h"

#include "tcrund.h"


#define TCR_LOG_FILE_OPTION     "--log_file"
#define TCR_LOG_FILE_ENVVAR     "TCRUND_LOG_FILE"
#define TCR_LOG_BUF_SIZE        (TC_BUF_LINE * 2)

/*************************************************************************/

static int tcr_log_null_send(TCLogContext *ctx, TCLogType type,
                             const char *tag, const char *fmt, va_list ap)
{
    return TC_OK;
}

static int tcr_log_null_close(TCLogContext *ctx)
{
    return TC_OK;
}

static int tcr_log_null_open(TCLogContext *ctx, int *argc, char ***argv)
{
    if (ctx) {
        ctx->send  = tcr_log_null_send;
        ctx->close = tcr_log_null_close;
        return TC_OK;
    }
    return TC_ERROR;
}


/*************************************************************************/

static const char *type_to_str(TCLogType type)
{
    static const char *tcr_log_type_str[] = {
        "ERROR",   /* TC_LOG_ERR */
        "WARNING", /* TC_LOG_WARN */
        "INFO",    /* TC_LOG_INFO */
        "MESSAGE", /* TC_LOG_MSG */
        "MARK",    /* TC_LOG_MARK */
    };
    return tcr_log_type_str[type - TC_LOG_ERR];
}

/*
 * looks pretty like tc_log_console_send,
 * but the devil is in the details...
 */
static int tcr_log_file_send(TCLogContext *ctx, TCLogType type,
                             const char *tag, const char *fmt, va_list ap)
{
    int ret = 0, is_dynbuf = TC_FALSE, truncated = TC_FALSE;
    /* flag: we must use a dynamic (larger than static) buffer? */
    char tsbuf[TC_BUF_MIN];
    char buf[TCR_LOG_BUF_SIZE];
    char *msg = buf;
    const char *tstr = type_to_str(type);
    size_t size = sizeof(buf);
    time_t now = 0;

    /* sanity check, avoid {under,over}flow; */
    type = TC_CLAMP(type, TC_LOG_ERR, TC_LOG_MARK);
    /* sanity check, avoid dealing with NULL as much as we can */
    tag = (tag != NULL) ?tag :"";
    fmt = (fmt != NULL) ?fmt :"";
    /* TC_LOG_EXTRA special handling: force always empty tag */
    tag = (type == TC_LOG_MARK) ?"" :tag; 
    
    now = time(NULL);
    strftime(tsbuf, sizeof(tsbuf), "%b %d %T", localtime(&now));

    size = strlen(tstr) + strlen(tsbuf)
                  + strlen(tag) + strlen(fmt) + 1;

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
    tc_snprintf(msg, size, "%s %s %s %s\n",
                tsbuf, tag, tstr, fmt);

    ret = vfprintf(ctx->f, msg, ap);
    ctx->log_count++;

    if (is_dynbuf) {
        free(msg);
    }

    if (ctx->log_count % ctx->flush_thres == 0) {
        /* ensure that all *other* messages are written */
        fflush(ctx->f);
    }

    /* finally encode the return code. Yeah, looks a bit cheesy. */
    if (ret < 0) {
        return 1;
    }
    if (truncated) {
        return -1;
    }
    return 0;

}

static int tcr_log_file_close(TCLogContext *ctx)
{
    int ret = TC_ERROR;

    if (ctx && ctx->f) {
        fflush(ctx->f);
        fclose(ctx->f); /* FIXME!!! */

        ctx->f = NULL;
        ret = TC_OK;
    }
    return ret;
}

static int tcr_log_file_open(TCLogContext *ctx, int *argc, char ***argv)
{
    const char *logpath = NULL;
    int err = 0, ret = TC_ERROR;

    if (!ctx) {
        return TC_ERROR;
    }
    err = tc_mangle_cmdline(argc, argv, TCR_LOG_FILE_OPTION, &logpath);
    if (err) {
        logpath = getenv(TCR_LOG_FILE_ENVVAR);
    }

    if (logpath) {
        ctx->f = fopen(logpath, "at");
        if (ctx->f) {
            ctx->send  = tcr_log_file_send;
            ctx->close = tcr_log_file_close;
            ret =  TC_OK;
        }
    }
    return ret;
}

/*************************************************************************/
/* main entry point */

int tcr_log_register_methods(void)
{
    tc_log_register_method(TCR_LOG_TARGET_FILE, tcr_log_file_open);
    tc_log_register_method(TCR_LOG_TARGET_NULL, tcr_log_null_open);
    return TC_OK;
}

int tcr_log_open(const char *logfile,
                 TCLogTarget target, TCVerboseLevel verbose)
{
    int ret = TC_ERROR;
    if (logfile) {
        /* ...Can you hear it? It's the sound of the UGLYNESS! */
        char argkey[TC_BUF_MIN] = { '\0' };
        char argval[PATH_MAX] = { '\0' };
        char *args[] = { NULL, argkey, argval, NULL };
        char **argv = args;
        int argc = 3;
 
        strlcpy(argkey, TCR_LOG_FILE_OPTION, sizeof(argkey));
        strlcpy(argval, logfile, sizeof(argval));
        ret = tc_log_open(target, verbose, &argc, &argv);
    }
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

