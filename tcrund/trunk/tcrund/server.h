/*
 * server.h -- transcode recording daemon.
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

#include "tcrund.h"

typedef struct tcrserver_ TCRServer;

typedef enum tcroperationresult_ TCROperationResult;
enum tcroperationresult_ {
    TCR_OPERATION_UNKNOWN      =  1, 
    TCR_OPERATION_OK           =  0,
    TCR_OPERATION_ERROR        = -1, /* polymorphic error code :^) */
    TCR_OPERATION_ALLOC_FAILED = -100,
};


int tcr_server_new(TCRServer **tcs, TCRConfig *config);

int tcr_server_run(TCRServer *tcs);

int tcr_server_del(TCRServer *tcs);

int tcr_server_cleanup(TCRServer *tcs);

int tcr_server_stop(TCRServer *tcs);

