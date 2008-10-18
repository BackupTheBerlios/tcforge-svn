/*
 * server.c -- transcode recording daemon.
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

#define _BSD_SOURCE
#include <unistd.h>

#include <event.h>
#include <evhttp.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>

#include "server.h"


#define MOD_NAME    "server"

/*************************************************************************/


enum {
    TC_SERVER_IDLE = 0,
    TC_SERVER_RECORDING
};

struct tcserver_ {
    TCRecDConfig        *config;

    int                 status;

    xmlrpc_registry     *registry;
    xmlrpc_env          env;

    struct event_base   *evbase;
    struct evhttp       *http;

    TCProcess           *current;
    uint32_t            instanceid;
};


static TCServer tcserver; /* static instance forced by xmlrpc-c */

/*************************************************************************/

static xmlrpc_value *tc_server_record_status(xmlrpc_env *envP,
                                             xmlrpc_value *paramArrayP,
                                             void *serverInfo ATTR_UNUSED,
                                             void *channelInfo ATTR_UNUSED)
{
}

static xmlrpc_value *tc_server_record_start(xmlrpc_env *envP,
                                            xmlrpc_value *paramArrayP,
                                            void *serverInfo ATTR_UNUSED,
                                            void *channelInfo ATTR_UNUSED)
{
}

static xmlrpc_value *tc_server_record_stop(xmlrpc_env *envP,
                                           xmlrpc_value *paramArrayP,
                                           void *serverInfo ATTR_UNUSED,
                                           void *channelInfo ATTR_UNUSED)
{
}

static void tc_server_post_handler(struct evhttp_request *req, void *userdata)
{
}

static void tc_server_base_handler(struct evhttp_request *req, void *userdata)
{
}

/*************************************************************************/

static int tc_server_init(TCServer *tcs)
{
    xmlrpc_env_init(&tcs->env);
    tcs->registry = xmlrpc_registry_new(&tcs->env);

    tcs->evbase   = event_init();
    tcs->http     = evhttp_new(tcs->evbase);
    
}

static int tc_server_setup(TCServer *tcs)
{
    struct xmlrpc_tc_server_info3 methodStatus = {
        .tc_serverName     = "TC.record.status",
        .tc_serverFunction = tc_server_record_status,
    };
    struct xmlrpc_tc_server_info3 methodStart = {
        .tc_serverName     = "TC.record.start",
        .tc_serverFunction = tc_server_record_start,
    };
    struct xmlrpc_tc_server_info3 methodStatus = {
        .tc_serverName     = "TC.record.stop",
        .tc_serverFunction = tc_server_record_stop,
    };

    xmlrpc_registry_add_method3(&env, registryP, &methodStatus);
    xmlrpc_registry_add_method3(&env, registryP, &methodStart);
    xmlrpc_registry_add_method3(&env, registryP, &methodStop);

    evhttp_set_cb(tcs->http, "/XML-RPC", tc_server_post_handler, tcs);
    evhttp_set_cb(tcs->http, "/",        tc_server_base_handler, tcs);
}

/*************************************************************************/

int tc_server_new(TCServer **tcs, TCRecDConfig *config)
{
    TCServer *serv = NULL;
    int ret = TC_ERROR;

    if (!tcs || !config) {
        return TC_ERROR;
    }
    serv = tc_zalloc(sizeof(TCServer));
    if (!serv) {
        goto fail;
    }

    ret = tc_server_init(serv);
    if (ret != TC_OK) {
        goto fail_init;
    }

    ret = tc_server_setup(serv);
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


int tc_server_run(TCServer *tcs)
{
    evhttp_bind_socket(tcs->http, tcs->config->host, (u_short)tcs->config->port);

    daemon(0, 0);
    event_dispatch();

    return TC_OK;
}



