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
#define TC_VERBOSE_MARK		0x0000FFFF
typedef enum tcverboselevel_ TCVerboseLevel;
enum tcverboselevel_ {
    /* let's confine ourselves into the lower half of an uint32_t */
    TC_QUIET =  0UL,      /* HAS to be zero */
    TC_INFO  = (1UL << 0),
    TC_DEBUG = (1UL << 1),
    TC_STATS = (1UL << 2),
    /***/
};

/* which messages are that? */
#define TC_LOG_LEVEL_MASK	0xFFFF0000
typedef enum tcloglevel_ TCLogLevel;
enum tcloglevel_ {
    /* let's confine ourselves into the upper half of an uint32_t */
    TC_LOG_ERR   = (1UL << 16), /* critical error condition */
    TC_LOG_WARN  = (1UL << 17), /* non-critical error condition */
    TC_LOG_INFO  = (1UL << 18), /* informative highlighted message */
    TC_LOG_MSG   = (1UL << 19), /* regular message */
    
    TC_LOG_EXTRA = (1UL << 28), /* must always be the last */
    /* 
     * on this special log level is guaranteed that message will be logged
     * verbatim: no tag, no colours, anything
     */
};

/* how to present the messages */
typedef enum tclogmode_ TCLogMode;
enum tclogmode_ {
    TC_LOG_INVALID = 0, /* the usual `error/unknown' value */
    TC_LOG_CONSOLE = 1  /* default */
};

typedef struct tclogcontext_ TCLogContext;
struct tclogcontext_ {
    TCVerboseLevel verbose;
    int use_colors; /* flag */
    FILE *f;

    int log_count;
    int flush_thres;

    void *priv;

    int (*send)(TCLogContext *ctx, TCLogLevel level,
		const char *tag, const char *fmt, va_list ap);
    int (*close)(TCLogContext *ctx);
};

/*************************************************************************/

int tc_log_init(void);

int tc_log_fini(void);

/*************************************************************************/

/* log method hook */
typedef int (*TCLogMethodOpen)(TCLogContext *ctx, int *argc, char ***argv);

int tc_log_register_method(TCLogMode mode, TCLogMethodOpen open);

int tc_log_open(TCLogMode mode, TCVerboseLevel verbose,
                int *argc, char ***argv);

int tc_log_close(void);

/*
 * tc_log:
 *     main libtc logging function. Log arbitrary messages according
 *     to a printf-like format chosen by the caller.
 *
 * Parameters:
 *     level: XXXXXX
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
int tc_log(TCLogLevel level, TCVerboseLevel verbose,
           const char *tag, const char *fmt, ...)
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
    tc_log(TC_LOG_ERR, TC_QUIET, PACKAGE, format , ## args); \
    exit(1); \
} while(0)
#define tc_info(format, args...) \
    tc_log(TC_LOG_INFO, TC_QUIET, PACKAGE, format , ## args)
#define tc_warn(format, args...) \
    tc_log(TC_LOG_WARN, TC_QUIET, PACKAGE, format , ## args)

/* macro goodies */
#define tc_log_error(tag, format, args...) \
    tc_log(TC_LOG_ERR, TC_QUIET, tag, format , ## args)
#define tc_log_info(tag, format, args...) \
    tc_log(TC_LOG_INFO, TC_QUIET, tag, format , ## args)
#define tc_log_warn(tag, format, args...) \
    tc_log(TC_LOG_WARN, TC_QUIET, tag, format , ## args)
#define tc_log_msg(tag, format, args...) \
    tc_log(TC_LOG_MSG, TC_QUIET, tag, format , ## args)

#define tc_log_perror(tag, string) do {                            \
    const char *__s = (string);  /* watch out for side effects */  \
    tc_log_error(tag, "%s%s%s", __s ? __s : "",                    \
                 (__s && *__s) ? ": " : "",  strerror(errno));     \
} while (0)


/*************************************************************************/

typedef enum tcdebugsource_ TCDebugSource;
enum tcdebugsource_ {
    TC_CLEANUP = (1UL << 0),
    TC_FLIST   = (1UL << 1),
    TC_SYNC    = (1UL << 2),
    TC_COUNTER = (1UL << 3),
    TC_PRIVATE = (1UL << 4),
    TC_THREADS = (1UL << 5),
    TC_WATCH   = (1UL << 6),
};

int tc_log_debug(TCDebugSource src, const char *tag, const char *fmt, ...)
#ifdef HAVE_GCC_ATTRIBUTES
__attribute__((format(printf,3,4)))
#endif
;


#ifdef __cplusplus
}
#endif

#endif  /* LOGGING_H */
