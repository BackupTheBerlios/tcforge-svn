/*
 * tcrund.c -- transcode remote control daemon.
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

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "server.h"
#include "tcrund.h"

#define MOD_NAME    "main"


/*************************************************************************/

/* global context */
typedef struct tcrcontext_ TCRContext;
struct tcrcontext_ {
    TCRConfig       config;
    TCRServer       *server;

    TCVerboseLevel  log_level;
    TCLogMode       log_mode;
    const char      *cfg_file;
    int             banner_only; /* flag */
};

static TCRContext TCRunD = {
/*  .config      intentionally left out */
    .server      = NULL,
    .log_level   = TC_INFO,
    .log_mode    = TCR_LOG_FILE,
    .cfg_file    = NULL,
    .banner_only = TC_FALSE
};

/*************************************************************************/

static int tcr_read_config(const char *cfgfile, TCRConfig *config)
{
    return -1;
}

static void cleanup(void)
{
    ;
}

static void sighandler(int signum)
{
//    tcr_log(MOD_NAME, TCC_LOG_INFO, "got signal [%i] (%s), exiting...", signum, "");
/*    if (TCRunD.server) {
        tc_server_cleanup(TCRunD.server);
        tc_server_del(TCRunD.server);
    }
*/    return;
}

/*************************************************************************/

void version(void)
{
    fprintf(stderr, "(%s v%s) (C) 2008 Francesco Romani", PACKAGE, VERSION);
}


static void usage(void)
{
    version();

    fprintf(stderr,"\nUsage: %s -f config [options]\n", PACKAGE);
    fprintf(stderr,"    -f config   select configuration file to use\n");
    fprintf(stderr,"    -D          run in debug mode "
                                   "(don't detach from terminal)\n");
    fprintf(stderr,"    -q level    select log level\n");
    fprintf(stderr,"    -v          print version and exit\n");
    fprintf(stderr,"    -h          print this message\n");
}


static TCVerboseLevel verbose_level_from_str(const char *str)
{
    TCVerboseLevel ret = -1;
    if (str) {
        if (strcasecmp(str, "quiet") == 0) {
            ret = TC_QUIET;
        } else if (strcasecmp(str, "info") == 0) {
            ret = TC_INFO;
        } else if (strcasecmp(str, "debug") == 0) {
            ret = TC_DEBUG;
        } else if (strcasecmp(str, "stats") == 0) {
            ret = TC_STATS;
        }
    }
    return ret;
}

#define VALIDATE_OPTION \
    do { \
        if (optarg[0] == '-') { \
            usage(); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)


int parse_cmdline(TCRContext *ctx, int argc, char *argv[])
{
    int ch = -1, debug_mode = TC_FALSE;

    while ((ch = getopt(argc, argv, "Df:q:vh?")) != -1) {
        switch (ch) {
          case 'f':
            VALIDATE_OPTION;
            ctx->cfg_file = optarg;
            break;
          case 'D':
            VALIDATE_OPTION;
            debug_mode = TC_TRUE;
            break;
          case 'q':
            VALIDATE_OPTION;
            ctx->log_level = verbose_level_from_str(optarg);
            if (ctx->log_level == -1) {
                fprintf(stderr, "unknown log level: `%s'\n", optarg);
                return TC_ERROR;
            }
            break;
          case 'v':
            version();
            ctx->banner_only = TC_TRUE;
            break;
          case 'h':
            usage();
            ctx->banner_only = TC_TRUE;
            break;
          default:
            usage();
            return TC_ERROR;
            break;
        }
    }

    if (debug_mode) {
        ctx->log_mode = TC_LOG_CONSOLE;
        ctx->log_level = TC_STATS; /* reset */
        ctx->config.debug_mode = TC_TRUE;
        /* 
         * this field is NOT present into the configuration file, so it will
         * not be overwritten. Is this cheating?
         */
    }
    return TC_OK;
}

#undef VALIDATE_OPTION


/*************************************************************************/

int main(int argc, char *argv[])
{
    int err = 0, ch = -1;

    /* parse cmdline options */
    tc_log_init();
    tcr_log_register_methods();

    err = parse_cmdline(&TCRunD, argc, argv);
    if (err) {
        return TCRUND_ERR_BAD_OPTIONS;
    }
    if (!TCRunD.banner_only) {
        /* some validation... */
        if (!TCRunD.cfg_file) {
            fprintf(stderr, "missing configuration file\n");
            fprintf(stderr, "option -f is MANDATORY!\n");
            return TCRUND_ERR_NO_CONFIG_FILE;
        }

        err = tc_log_open(TCRunD.log_mode, TCRunD.log_level, &argc, &argv);
        if (err) {
            fprintf(stderr, "error opening log\n");
            return TCRUND_ERR_NO_LOG;
        }
        /* now we can tc_log() safely and happily */

        signal(SIGINT,  sighandler);
        signal(SIGTERM, sighandler);
        atexit(cleanup);

        err = tcr_read_config(TCRunD.cfg_file, &TCRunD.config);
        if (err) {
            tc_log(TC_QUIET, TC_ERROR,
                   "error reading the configuration file `%s'", TCRunD.cfg_file);
            return TCRUND_ERR_BAD_CONFIG_FILE;
        }

        err = tcr_server_new(&TCRunD.server, &TCRunD.config);
        if (err) {
            tc_log(TC_QUIET, TC_ERROR, PACKAGE,
                   "error setting up the server");
            return TCRUND_ERR_SERVER_SETUP;
        }

        err = tcr_server_run(TCRunD.server);
        if (err) {
            tc_log(TC_QUIET, TC_ERROR, PACKAGE,
                   "error starting the event loop");
            return TCRUND_ERR_SERVER_RUN;
        }
    }

    tcr_server_cleanup(TCRunD.server);
    tcr_server_del(TCRunD.server);

    return TCRUND_OK;
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

