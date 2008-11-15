/*
 * auth.h -- transcode recording daemon authorization subsystem.
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

#ifndef TCRAUTH_H
#define TCRAUTH_H


typedef struct tcrauthsession_ TCRAuthSession;

enum {
    TCR_AUTH_TOKEN_LEN = 128,
    TCR_AUTH_REPLY_LEN = 128
};

typedef enum tcrauthstatus_ {
    TCR_AUTH_OK = 0,
    TCR_AUTH_ERR_METHOD,
    TCR_AUTH_ERR_PARAMS,
    TCR_AUTH_ERR_CONFIG,
    TCR_AUTH_ERR_USER,
    TCR_AUTH_ERR_SESSION_BAD,	/* invalid session identification */
    TCR_AUTH_ERR_MESSAGE_BAD,   /* invalid message identification */
    TCR_AUTH_ERR_MESSAGE_ORDER, /* out-of-order message */
} TCRAuthStatus;

typedef enum tcrauthmethod_ {
    TCR_AUTH_NULL = -1, /* the usual `invalid/null' value */
    TCR_AUTH_NONE,      /* accept everything to everyone */
    TCR_AUTH_PLAINPASS, /* plaintext session authentication */
} TCRAuthMethod;


int tcr_auth_init(TCRAuthMethod authmethod, const char *cfgfile);

int tcr_auth_fini(void);


TCRAuthSession *tcr_auth_session_new(const char *username, const char *token,
                                     char *reply, int *error);

int tcr_auth_session_del(TCRAuthSession *as, const char *sestoken);


int tcr_auth_message_new(TCRAuthSession *as, const char *sestoken,
                         const char *msgtoken, char *msgreply);


#endif /* TCR_AUTH_H */
