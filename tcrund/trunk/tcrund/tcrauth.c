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

#include "tcutil.h"

#include "tcrauth.h"

typedef struct tcrauthcontext_ TCRAuthContext;
struct tcrauthcontext_ {
    const char *usersfile;

    int (*new_session)(void **privdata, const char *username,
                       const char *token, char *reply);
    int (*del_session)(void *privdata);
    int (*check_message)(void *privdata, const char *sestoken,
                         const char *msgtoken, char *msgreply);
};

struct tcrauthsession_ {
    const TCRAuthContext *ctx;
    void *privdata;
};

/**************************************************************************/
/* authentication backends                                                */
/**************************************************************************/

/*
static int tcr_auth_none_open(TCRAuthContext *ctx, const char *cfgfile)
{
    return TC_ERROR;
}


static int tcr_auth_plainpass_open(TCRAuthContext *ctx, const char *cfgfile)
{
    return TC_ERROR;
}
*/

/**************************************************************************/
/* API helpers                                                            */
/**************************************************************************/

typedef int (*TCRAuthMethodOpen)(TCRAuthContext *ctx, const char *cfgfile);

struct auth_data {
    TCRAuthMethod     auth;
    TCRAuthMethodOpen open;
};

/*
static struct auth_data methods[] = {
    { TCR_AUTH_NONE,      tcr_auth_none_open      },
    { TCR_AUTH_PLAINPASS, tcr_auth_plainpass_open },
    { TCR_AUTH_NULL,      NULL                    }
};

static TCRAuthContext TCRAuth;
*/

/**************************************************************************/
/* public API                                                             */
/**************************************************************************/

int tcr_auth_init(TCRAuthMethod authmethod, const char *usersfile)
{
    return TC_OK;
}

int tcr_auth_fini(void)
{
    return TC_OK;
}



TCRAuthSession *tcr_auth_session_new(const char *username, const char *token,
                                     char *reply, int *error)
{
    return NULL;
}


int tcr_auth_session_del(TCRAuthSession *as, const char *sestoken)
{
    int ret = TC_ERROR;
    return ret;
}


int tcr_auth_message_new(TCRAuthSession *as, const char *sestoken,
                         const char *msgtoken, char *msgreply)
{
    int ret = TC_ERROR;
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

