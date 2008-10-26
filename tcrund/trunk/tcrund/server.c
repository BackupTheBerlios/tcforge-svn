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
    TCRConfig          *config;

    int                 status;

    xmlrpc_registry    *registry;
    xmlrpc_env          env;

    struct event_base  *evbase;
    struct evhttp      *http;

    TCRProcess         *current;
    uint32_t            instanceid;
};


/*************************************************************************/

static xmlrpc_value *tcr_server_login(xmlrpc_env *env,
                                      xmlrpc_value *paramArray,
                                      void * const userData)
{
    return NULL;
}

static xmlrpc_value *tcr_server_logout(xmlrpc_env *env,
                                       xmlrpc_value *paramArray,
                                       void * const userData)
{
    return NULL;
}

static xmlrpc_value *tcr_server_process_status(xmlrpc_env *envP,
                                           xmlrpc_value *paramArrayP,
                                           void * const userData)
{
    return NULL;
}

static xmlrpc_value *tcr_server_process_start(xmlrpc_env *envP,
                                          xmlrpc_value *paramArrayP,
                                          void * const userData)
{
    return NULL;
}

static xmlrpc_value *tcr_server_process_stop(xmlrpc_env *envP,
                                         xmlrpc_value *paramArrayP,
                                         void * const userData)
{
    return NULL;
}

/*************************************************************************/

static void tcr_server_post_handler(struct evhttp_request *req, void *userdata)
{
    xmlrpc_mem_block *xmlres = NULL;
    TCRServer *tcs = userdata;
    /* awful, awful casts */
    char *rawreq = (char*)EVBUFFER_DATA(req->input_buffer);
    size_t rawlen = (size_t)EVBUFFER_LENGTH(req->input_buffer);

    /* mark */
    tc_log(TC_LOG_MSG, PACKAGE,
           "(%s) processing incoming request", __func__);

    /* some input validation */
    if (!userdata) {
        tc_log(TC_LOG_ERR, PACKAGE,
               "(%s) missing context (userdata)", __func__);
        return;
    }
	if (req->type != EVHTTP_REQ_POST) {
        tc_log(TC_LOG_ERR, PACKAGE,
               "(%s) received request is not POST", __func__);
        return;
	}

    if (!rawreq || !rawlen) {
        tc_log(TC_LOG_ERR, PACKAGE,
               "(%s) received empty request", __func__);
        return;
    }

    /* ok, let's roll */
    xmlres = xmlrpc_registry_process_call(&tcs->env,
                                          tcs->registry,
                                          tcs->config->host,
                                          rawreq,
                                          rawlen);

    /* we're still in danger */
    if (!xmlres) {
        tc_log(TC_LOG_WARN, PACKAGE,
               "(%s) XML registry returned empty response", __func__);
    } else {
	    struct evbuffer *evb = evbuffer_new();

        if (!evb) {
            tc_log(TC_LOG_ERR, PACKAGE,
                   "(%s) cannot allocate response evbuffer", __func__);
        } else {
            char *rawres = XMLRPC_TYPED_MEM_BLOCK_CONTENTS(char, xmlres);

        	evbuffer_add(evb, rawres, strlen(rawres) + 1);
            /* send the terminator too */

    	    evhttp_send_reply(req, HTTP_OK, "XML response follows", evb);

    	    evbuffer_free(evb);
            XMLRPC_TYPED_MEM_BLOCK_FREE(char, xmlres);

            tc_log(TC_LOG_MSG, PACKAGE,
                   "(%s) response sent", __func__);
        }
    }
    return;
}

static void tcr_server_base_handler(struct evhttp_request *req, void *userdata)
{
    /* reroute to post handler until we find something smarter to do */
    tcr_server_post_handler(req, userdata);
}

/*************************************************************************/

static int tcr_server_init(TCRServer *tcs)
{
    xmlrpc_env_init(&tcs->env);
    tcs->registry = xmlrpc_registry_new(&tcs->env);

    tcs->evbase   = event_init();
    tcs->http     = evhttp_new(tcs->evbase);

    return TC_OK;
}

static int tcr_server_setup(TCRServer *tcs)
{
    xmlrpc_registry_add_method(&tcs->env,
                               tcs->registry,
                               NULL,
                               "TC.user.login",
                               tcr_server_login,
                               tcs);

    xmlrpc_registry_add_method(&tcs->env,
                               tcs->registry,
                               NULL,
                               "TC.user.logout",
                               tcr_server_logout,
                               tcs);

    xmlrpc_registry_add_method(&tcs->env,
                               tcs->registry,
                               NULL,
                               "TC.process.status",
                               tcr_server_process_status,
                               tcs);

    xmlrpc_registry_add_method(&tcs->env,
                               tcs->registry,
                               NULL,
                               "TC.process.start",
                               tcr_server_process_start,
                               tcs);

    xmlrpc_registry_add_method(&tcs->env,
                               tcs->registry,
                               NULL,
                               "TC.process.stop",
                               tcr_server_process_stop,
                               tcs);

    evhttp_set_cb(tcs->http, "/XML-RPC", tcr_server_post_handler, tcs);
    evhttp_set_cb(tcs->http, "/",        tcr_server_base_handler, tcs);

    return TC_OK;
}

/*************************************************************************/

int tcr_server_new(TCRServer **tcs, TCRConfig *config)
{
    TCRServer *serv = NULL;
    int ret = TC_ERROR;

    if (!tcs || !config) {
        tc_log(TC_LOG_ERR, PACKAGE,
               "(%s) received NULL arguments!", __func__);
        return TC_ERROR;
    }
    serv = tc_zalloc(sizeof(TCRServer));
    if (!serv) {
        tc_log(TC_LOG_ERR, PACKAGE,
               "(%s) cannot allocate the server context", __func__);
        goto fail;
    }

    ret = tcr_server_init(serv);
    if (ret != TC_OK) {
        tc_log(TC_LOG_ERR, PACKAGE,
               "(%s) server initialization failed", __func__);
        goto fail_init;
    }

    ret = tcr_server_setup(serv);
    if (ret != TC_OK) {
        tc_log(TC_LOG_ERR, PACKAGE,
               "(%s) server setup failed", __func__);
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

    if (!tcs->config->debug_mode) {
       daemon(0, 0);
    }

    event_dispatch();

    return TC_OK;
}

int tcr_server_stop(TCRServer *tcs)
{
    int ret = TC_ERROR;
    if (tcs) {
        event_loopbreak();
        ret = TC_OK;
    }
    return ret;
}

int tcr_server_cleanup(TCRServer *tcs)
{
    return TC_ERROR;
}

int tcr_server_del(TCRServer *tcs)
{
    return TC_ERROR;
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

