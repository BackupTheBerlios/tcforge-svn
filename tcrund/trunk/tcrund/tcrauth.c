/*
 * auth.c -- transcode recording daemon authorization subsystem.
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

#include "libtcutil/tcutil.h"

#include "tcrauth.h"



typedef struct tcrauthcontext_ TCRAuthContext;
struct tcrauthcontext_ {
    const char *usersfile;

    void* (*new_session)(const char *username,
                         const char *token, char *reply,
                         int *error);
    int   (*del_session)(void *privdata, const char *sestoken);
    int   (*new_message)(void *privdata, const char *sestoken,
                         const char *msgtoken, char *msgreply);
};

struct tcrauthsession_ {
    void *privdata;
};

static TCRAuthContext TCRAuth;


/**************************************************************************/
/* utils                                                                  */
/**************************************************************************/

#define RETURN_SESSION(PTR, ERRCODE) \
    if (error) { \
        *error = (ERRCODE); \
    } \
    return (PTR)
    

/**************************************************************************/
/* authentication backend: None (always succeed)                          */
/**************************************************************************/

#define TCR_AUTH_NONE_TAG       "OK"

static int tcr_auth_none_checktag(void *privdata)
{
    const char *tag = privdata;
    int ret = TCR_AUTH_ERR_PARAMS;

    if (tag && strcmp(tag, TCR_AUTH_NONE_TAG) == 0) {
        ret = TCR_AUTH_OK;
    }
    return ret;
}

static int tcr_auth_none_new_message(void *privdata, const char *sestoken,
                                     const char *msgtoken, char *msgreply)
{
    return tcr_auth_none_checktag(privdata);
}

static int tcr_auth_none_del_session(void *privdata, const char *sestoken)
{
    return tcr_auth_none_checktag(privdata);
}

static void *tcr_auth_none_new_session(const char *username,
                                       const char *token, char *reply,
                                       int *error)
{
    if (!username || !token || !reply) {
        RETURN_SESSION(NULL, TCR_AUTH_ERR_PARAMS);
    }
    RETURN_SESSION(TCR_AUTH_NONE_TAG, TCR_AUTH_OK);
}

static int tcr_auth_none_init(TCRAuthContext *ctx)
{
    int ret = TCR_AUTH_ERR_PARAMS;
    if (ctx) {
        ctx->new_session = tcr_auth_none_new_session;
        ctx->del_session = tcr_auth_none_del_session;
        ctx->new_message = tcr_auth_none_new_message;
    }
    return ret;
}


/**************************************************************************/
/* authentication backend: Plainpass (simple password checking)           */
/**************************************************************************/

/* no need for anything else */

static void *tcr_auth_plainpass_new_session(const char *username,
                                            const char *token, char *reply,
                                            int *error)
{
    int authres = TCR_AUTH_ERR_PARAMS;
    void *tag = tcr_auth_none_new_session(username, token, reply, &authres);
    /* used for param checking */

    if (tag) {
        char *password = NULL;
        TCConfigEntry pass_conf[] = {
            { "password",   &password,  TCCONF_TYPE_STRING, 0, 0, 0 },
            { NULL,         NULL,       0,                  0, 0, 0 }
        };

        tc_config_read_file(TCRAuth.usersfile, username, pass_conf, PACKAGE);
        if (password && strcmp(password, token) == 0) {
            authres = TCR_AUTH_OK;
        } else {
            authres = TCR_AUTH_ERR_USER;
        }
    }
    RETURN_SESSION(tag, authres);
}


static int tcr_auth_plainpass_init(TCRAuthContext *ctx)
{
    int ret = TCR_AUTH_ERR_PARAMS;
    if (ctx) {
        ctx->new_session = tcr_auth_plainpass_new_session;
        ctx->del_session = tcr_auth_none_del_session;
        ctx->new_message = tcr_auth_none_new_message;
    }
    return ret;
}


/**************************************************************************/
/* API helpers                                                            */
/**************************************************************************/

typedef int (*TCRAuthInitContext)(TCRAuthContext *ctx);

struct auth_data {
    TCRAuthMethod      auth;
    TCRAuthInitContext init;
};

static struct auth_data auth_methods[] = {
    { TCR_AUTH_NONE,      tcr_auth_none_init      },
    { TCR_AUTH_PLAINPASS, tcr_auth_plainpass_init },
    { TCR_AUTH_NULL,      NULL                    }
};


/**************************************************************************/
/* public API                                                             */
/**************************************************************************/

int tcr_auth_init(TCRAuthMethod authmethod, const char *usersfile)
{
    int ret = TCR_AUTH_ERR_METHOD;

    if (usersfile) {
        int i;

        TCRAuth.usersfile = usersfile;

        for (i = 0; auth_methods[i].auth != TCR_AUTH_NULL; i++) {
            if (auth_methods[i].auth == authmethod) {
                ret = auth_methods[i].init(&TCRAuth);
                break;
            }
        }
    }

    return TC_OK;
}

int tcr_auth_fini(void)
{
    return TCR_AUTH_OK;
}



TCRAuthSession *tcr_auth_session_new(const char *username, const char *token,
                                     char *reply, int *error)
{
    TCRAuthSession *as = NULL;

    if (error) {
        as = tc_zalloc(sizeof(TCRAuthSession));
        if (as) {
            as->privdata = TCRAuth.new_session(username, token, reply, error);
            if (*error != TCR_AUTH_OK) {
                tc_free(as);
                as = NULL;
            }
        }
    }
    return as;
}


int tcr_auth_session_del(TCRAuthSession *as, const char *sestoken)
{
    int err = TCR_AUTH_ERR_PARAMS;
    if (as) {
        err = TCRAuth.del_session(as->privdata, sestoken);
        if (err == TCR_AUTH_OK) {
            
        }
    }
    return err;
}


int tcr_auth_message_new(TCRAuthSession *as, const char *sestoken,
                         const char *msgtoken, char *msgreply)
{
    int err = TCR_AUTH_ERR_PARAMS;
    if (as) {
        err = TCRAuth.new_message(as->privdata, sestoken,
                                  msgtoken, msgreply);
    }
    return err;
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

