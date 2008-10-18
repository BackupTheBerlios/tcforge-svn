/*
 * tcrund.c -- transcode recording daemon.
 * (C) 2008 Francesco Romani <fromani at gmail dot com>
 *
 * This file is part of tcrund, a tv recording daemon.
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"
#include "tcrund.h"

#define MOD_NAME    "main"

typedef struct tcrundcontext_ TCRunDContext;
struct tcrundcontext_ {
    TCRecDConfig    config;
    TCCLogData      log;
    TCServer        *server;
};

static TCRunDContext TCRunD;


static int tcrund_read_config(const char *cfgfile, TCRecDConfig *config)
{
    return -1;
}

static void cleanup(void)
{
    tcclog_close(&TCRunD.log);
}

static void sighandler(int signum)
{
    tcrund_log(MOD_NAME, TCC_LOG_INFO, "got signal [%i] (%s), exiting...", signum, "");
/*    if (TCRunD.server) {
        tc_server_cleanup(TCRunD.server);
        tc_server_del(TCRunD.server);
    }
*/    return;
}




int main(int argc, char *argv[])
{
    const char *cfgfile = NULL;
    int loglevel;
    int err;

    /* parse cmdline options */

    signal(SIGINT,  sighandler);
    signal(SIGTERM, sighandler);
    atexit(cleanup);

    err = tcrund_read_config(cfgfile, &TCRunD.config);
    if (err) {
        /* TODO */
    }
#if 0
    err = tc_server_new(&TCRunD.server, &TCRunD.config);
    if (err) {
        /* TODO */
    }

    err = tc_server_run(TCRunD.server);
    if (err) {
        /* TODO */
    }
#endif
    return 0;
}

