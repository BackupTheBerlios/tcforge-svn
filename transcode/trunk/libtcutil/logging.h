/*
 *  tclog.h - transcode logging infrastructure (interface)
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

#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************/

#define TC_LOG_COLOR_ENV_VAR    "TRANSCODE_LOG_NO_COLOR"
#define TC_LOG_COLOR_OPTION     "--log_no_color"


/* how much messages do you want to see? */
typedef enum tcverboselevel_ TCVerboseLevel;
enum tcverboselevel_ {
    TC_QUIET   =  0,
    TC_INFO    =  1,
    TC_DEBUG   =  2,	/* still on doubt */
    TC_STATS   =  3,
};

/* which messages are that? */
typedef enum tclogtype_ TCLogType;
enum tclogtype_ {
    TC_LOG_ERR   = 0, /* critical error condition */
    TC_LOG_WARN,      /* non-critical error condition */
    TC_LOG_INFO,      /* informative highlighted message */
    TC_LOG_MSG,       /* regular message */
    TC_LOG_MARK,      /* verbatim, don't add anything */
};

/* how to present the messages */
typedef enum tclogtarget_ TCLogTarget;
enum tclogtarget_ {
    TC_LOG_INVALID = 0,  /* the usual `error/unknown' value */
    TC_LOG_CONSOLE = 1,  /* default */
    TC_LOG_USEREXT = 127 /* use this as base for extra methods */
};

typedef enum tcdebugsource_ TCDebugSource;
enum tcdebugsource_ {
    TC_CLEANUP = 1,
    TC_FLIST,
    TC_SYNC,
    TC_COUNTER,
    TC_PRIVATE,
    TC_THREADS,
    TC_WATCH,
};

typedef struct tclogcontext_ TCLogContext;
struct tclogcontext_ {
    TCDebugSource debug_src;

    TCVerboseLevel verbose;
    int use_colors; /* flag */
    FILE *f;

    int log_count;
    int flush_thres;

    void *priv;

    int (*send)(TCLogContext *ctx, TCLogType level,
		const char *tag, const char *fmt, va_list ap);
    int (*close)(TCLogContext *ctx);
};

/*************************************************************************/

int tc_log_init(void);

int tc_log_fini(void);

/*************************************************************************/

/* log method hook */
typedef int (*TCLogMethodOpen)(TCLogContext *ctx, int *argc, char ***argv);

int tc_log_register_method(TCLogTarget target, TCLogMethodOpen open);

int tc_log_open(TCLogTarget target, TCVerboseLevel verbose,
                int *argc, char ***argv);

int tc_log_close(void);

/*
 * tc_log:
 *     main libtc logging function. Log arbitrary messages according
 *     to a printf-like format chosen by the caller.
 *
 * Parameters:
 *   verbose: YYYYYY
 *       tag: header of message, to identify subsystem originating
 *            the message. It's suggested to use __FILE__ as
 *            fallback default tag.
 *       fmt: printf-like format string. You must provide enough
 *            further arguments to fullfill format string, doing
 *            otherwise will cause an undefined behaviour, most
 *            likely a crash.
 * Return Value:
 *      0 if message succesfully logged.
 *      1 message was NOT written at all.
 *     -1 if message was written truncated.
 *        (message too large and buffer allocation failed).
 * Side effects:
 *     this function store final message in an intermediate string
 *     before to log it to destination. If such intermediate string
 *     is wider than a given amount (TC_BUF_MIN * 2 at moment
 *     of writing), tc_log needs to dinamically allocate some memory.
 *     This allocation can fail, and as result log message will be
 *     truncated to fit in avalaible static buffer.
 */
int tc_log(TCLogType type, const char *tag, const char *fmt, ...)
#ifdef HAVE_GCC_ATTRIBUTES
__attribute__((format(printf,4,5)))
#endif
;

/* 
 * When to use tc_log*() stuff?
 *
 * tc_log() family should be used for non-output status messages, like
 * the ones coming from the various modules and components of transcode.
 * For actual output use printf() (or fprintf(), etc.) as appropriate.
 * (yes, this means that transcode prints a lot of status and a very
 * few output messages).
 */

/* compatibility macros */
#define tc_error(format, args...) do { \
    tc_log(TC_LOG_ERR, PACKAGE, format , ## args); \
    exit(1); \
} while(0)
#define tc_info(format, args...) \
    tc_log(TC_LOG_INFO, PACKAGE, format , ## args)
#define tc_warn(format, args...) \
    tc_log(TC_LOG_WARN, PACKAGE, format , ## args)

/* macro goodies */
#define tc_log_error(tag, format, args...) \
    tc_log(TC_LOG_ERR, tag, format , ## args)
#define tc_log_info(tag, format, args...) \
    tc_log(TC_LOG_INFO, tag, format , ## args)
#define tc_log_warn(tag, format, args...) \
    tc_log(TC_LOG_WARN, tag, format , ## args)
#define tc_log_msg(tag, format, args...) \
    tc_log(TC_LOG_MSG, tag, format , ## args)

#define tc_log_perror(tag, string) do {                            \
    const char *__s = (string);  /* watch out for side effects */  \
    tc_log_error(tag, "%s%s%s", __s ? __s : "",                    \
                 (__s && *__s) ? ": " : "",  strerror(errno));     \
} while (0)


int tc_log_debug(TCDebugSource src, const char *tag, const char *fmt, ...)
#ifdef HAVE_GCC_ATTRIBUTES
__attribute__((format(printf,3,4)))
#endif
;


#ifdef __cplusplus
}
#endif

#endif  /* LOGGING_H */
