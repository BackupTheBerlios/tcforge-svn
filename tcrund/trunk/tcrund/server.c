/*
 * server.c -- transcode remote control daemon.
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

#define _BSD_SOURCE
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <event.h>
#include <evhttp.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>

#include "tcutil.h"

#include "process.h"
#include "server.h"


#define MOD_NAME    "server"

/*************************************************************************/


enum {
    TC_SERVER_IDLE = 0,
    TC_SERVER_RECORDING
};

struct tcrserver_ {
    TCRConfig           *config;

    int                 status;

    xmlrpc_registry     *registry;
    xmlrpc_env          env;

    struct event_base   *evbase;
    struct evhttp       *http;

    TCRProcess          *current;
    uint32_t            instanceid;
};


/*************************************************************************/

static xmlrpc_value *tcr_server_run_status(xmlrpc_env *env,
                                           xmlrpc_value *paramArray,
                                           void * const userData)
{
}

static xmlrpc_value *tcr_server_run_start(xmlrpc_env *envP,
                                          xmlrpc_value *paramArrayP,
                                          void * const userData)
{
}

static xmlrpc_value *tcr_server_run_stop(xmlrpc_env *envP,
                                         xmlrpc_value *paramArrayP,
                                         void * const userData)
{
}

static void tcr_server_post_handler(struct evhttp_request *req, void *userdata)
{
}

static void tcr_server_base_handler(struct evhttp_request *req, void *userdata)
{
}

/*************************************************************************/

static int tcr_server_init(TCRServer *tcs)
{
    xmlrpc_env_init(&tcs->env);
    tcs->registry = xmlrpc_registry_new(&tcs->env);

    tcs->evbase   = event_init();
    tcs->http     = evhttp_new(tcs->evbase);
    
}

static int tcr_server_setup(TCRServer *tcs)
{
    xmlrpc_registry_add_method(&tcs->env,
                               tcs->registry,
                               NULL,
                               "TC.run.status",
                               tcr_server_run_status,
                               tcs);

    xmlrpc_registry_add_method(&tcs->env,
                               tcs->registry,
                               NULL,
                               "TC.run.start",
                               tcr_server_run_start,
                               tcs);

    xmlrpc_registry_add_method(&tcs->env,
                               tcs->registry,
                               NULL,
                               "TC.run.stop",
                               tcr_server_run_stop,
                               tcs);

    evhttp_set_cb(tcs->http, "/XML-RPC", tcr_server_post_handler, tcs);
    evhttp_set_cb(tcs->http, "/",        tcr_server_base_handler, tcs);
}

/*************************************************************************/

int tcr_server_new(TCRServer **tcs, TCRConfig *config)
{
    TCRServer *serv = NULL;
    int ret = TC_ERROR;

    if (!tcs || !config) {
        return TC_ERROR;
    }
    serv = tc_zalloc(sizeof(TCRServer));
    if (!serv) {
        goto fail;
    }

    ret = tcr_server_init(serv);
    if (ret != TC_OK) {
        goto fail_init;
    }

    ret = tcr_server_setup(serv);
    if (ret != TC_OK) {
        goto fail_setup;
    }

    serv->config = config;

    *tcs = serv;
    return TC_OK;

fail_setup:
fail_init:
    tc_free(serv);
fail:
    *tcs = NULL;
    return TC_ERROR;
}


int tcr_server_run(TCRServer *tcs)
{
    evhttp_bind_socket(tcs->http, tcs->config->host, (u_short)tcs->config->port);

    daemon(0, 0);
    event_dispatch();

    return TC_OK;
}



