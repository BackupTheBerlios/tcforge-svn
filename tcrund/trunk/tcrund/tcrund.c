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


/*************************************************************************/

/* global context */
typedef struct tcrcontext_ TCRContext;
struct tcrcontext_ {
    TCRConfig       config;
    TCRServer      *server;

    TCLogTarget     log_target;
    TCVerboseLevel  log_level;
    const char      *cfg_file;
    int              banner_only; /* flag */
};

static TCRContext TCRunD = {
/*  .config      intentionally left out */
    .server      = NULL,
    .cfg_file    = NULL,
    .log_level   = TC_INFO,
    .log_target  = TCR_LOG_FILE,
    .banner_only = TC_FALSE
};

/*************************************************************************/

static void tcr_set_config_defaults(TCRConfig *cfg)
{
    if (cfg) {
        cfg->port           = TCRUND_DEFAULT_PORT;
        cfg->host           = TCRUND_DEFAULT_HOST;
        cfg->logs_dir       = TCRUND_DEFAULT_LOG_DIR;
        cfg->files_dir      = TCRUND_DEFAULT_FILES_PATH;
        cfg->tc_conf_dir    = TCRUND_DEFAULT_CONF_PATH;
        cfg->out_fmt_string = TCRUND_DEFAULT_OUT_FMT_STRING;
    }
}

/* 
 * config is static data, is read rarely or just once and not concurrently.
 * private (tc_malloc'd) data is harder to mantain and doesn't offer benefits
 * here.
 */
static int tcr_read_config(const char *cfg_file, TCRConfig *cfg)
{
    TCConfigEntry tcrund_conf[] = {
        { "port",       &(cfg->port),           TCCONF_TYPE_INT, TCCONF_FLAG_RANGE, 1024, 65535 },
        { "host",       &(cfg->host),           TCCONF_TYPE_STRING, 0, 0, 0 },
        { "logs_dir",   &(cfg->logs_dir),       TCCONF_TYPE_STRING, 0, 0, 0 },
        { "files_dir",  &(cfg->files_dir),      TCCONF_TYPE_STRING, 0, 0, 0 },
        { "config_dir", &(cfg->tc_conf_dir),    TCCONF_TYPE_STRING, 0, 0, 0 },
        { "out_format", &(cfg->out_fmt_string), TCCONF_TYPE_STRING, 0, 0, 0 },
        { NULL,         NULL,                   0,                  0, 0, 0 }
    };
    int ret = TC_ERROR;
    int res = module_read_config(cfg_file, TCRUND_CONFIG_FILE_MAIN,
                                 tcrund_conf, PACKAGE);

    if (res != 0) {
        if (cfg->debug_mode) {
            tc_log(TC_LOG_INFO, PACKAGE,
                   "configuration from [%s]", cfg_file);
            tc_log(TC_LOG_INFO, PACKAGE,
                   "=================="); /* puah */
            module_print_config(tcrund_conf, PACKAGE);
        }
        ret = TC_OK;
    }
    return ret;
}

#undef RETURN_IF_FAILED

/*************************************************************************/

static void cleanup(void)
{
 /* 
    if (TCRunD.server) {
        tc_server_cleanup(TCRunD.server);
        tc_server_del(TCRunD.server);
    }
*/  
}

static void sig_reload_handler(int signum)
{
    int err = TC_OK;

    tc_log(TC_LOG_INFO, PACKAGE,
           "got signal [%i] reloading configuration file `%s'",
           signum, TCRunD.cfg_file);

    err = tcr_read_config(TCRunD.cfg_file, &TCRunD.config);
    if (err) {
        tc_log(TC_LOG_ERR, PACKAGE,
               "error reading the configuration file `%s'", TCRunD.cfg_file);
        raise(SIGTERM); /* FIXME */
    }
}


static void sig_exit_handler(int signum)
{
    tc_log(TC_LOG_INFO, PACKAGE,
           "got signal [%i] (%s), exiting...", signum, "");
    tcr_server_stop(TCRunD.server);
}

/*************************************************************************/

void version(void)
{
    fprintf(stderr, "(%s v%s) (C) 2008 Francesco Romani", PACKAGE, VERSION);
}


static void usage(void)
{
    version();

    fprintf(stderr,"\nUsage: %s -f conf [options]\n", PACKAGE);
    fprintf(stderr,"    -f conf     select configuration file to use\n");
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
          case 'q':
            VALIDATE_OPTION;
            ctx->log_level = verbose_level_from_str(optarg);
            if (ctx->log_level == -1) {
                fprintf(stderr, "[%s] unknown log level: `%s'\n", 
                        PACKAGE, optarg);
                return TC_ERROR;
            }
            break;
          case 'D':
            debug_mode = TC_TRUE;
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
        ctx->log_level = TC_STATS; /* reset */
        ctx->log_target = TC_LOG_CONSOLE;
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
    TCRConfig *cfg = &(TCRunD.config); /* shortcut */
    int err = 0;

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
            fprintf(stderr, "[%s] missing configuration file\n", PACKAGE);
            fprintf(stderr, "[%s] option -f is MANDATORY!\n", PACKAGE);
            return TCRUND_ERR_NO_CONFIG_FILE;
        }

        err = tc_log_open(TCRunD.log_target, TCRunD.log_level, &argc, &argv);
        if (err) {
            fprintf(stderr, "[%s] error opening log\n", PACKAGE);
            return TCRUND_ERR_NO_LOG;
        }
        /* now we can tc_log() safely and happily */

        signal(SIGHUP,  sig_reload_handler);
        signal(SIGINT,  sig_exit_handler);
        signal(SIGTERM, sig_exit_handler);
        atexit(cleanup);

        tcr_set_config_defaults(cfg);

        err = tcr_read_config(TCRunD.cfg_file, cfg);
        if (err) {
            tc_log(TC_LOG_ERR, PACKAGE,
                   "error reading the configuration file `%s'", TCRunD.cfg_file);
            return TCRUND_ERR_BAD_CONFIG_FILE;
        }

        err = tcr_server_new(&TCRunD.server, cfg);
        if (err) {
            tc_log(TC_LOG_ERR, PACKAGE,
                   "error setting up the server");
            return TCRUND_ERR_SERVER_SETUP;
        }

        tc_log(TC_LOG_INFO, PACKAGE, "server run");

        err = tcr_server_run(TCRunD.server);
        if (err) {
            tc_log(TC_LOG_ERR, PACKAGE,
                   "error starting the event loop");
            return TCRUND_ERR_SERVER_RUN;
        }
    }

    if (cfg->debug_mode) {
        tc_log(TC_LOG_INFO, PACKAGE, "server end");
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

