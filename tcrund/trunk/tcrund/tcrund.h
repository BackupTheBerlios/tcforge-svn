/*
 * tcrund.h -- transcode tv recording daemon.
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

#ifndef TCRUND_H
#define TCRUND_H

#include <stdint.h>
#include <stdarg.h>

#include "tcutil.h"


typedef struct tcrconfig_ TCRConfig;
struct tcrconfig_ {
    const char  *host;
    int         port;

    const char  *logpath;

    const char  *tc_config_dir;

    const char  *files_dir;
    const char  *out_fmt_string;

    int         debug_mode;
};

typedef enum tcrlogmode_ TCRLogMode;
enum tcrlogmode_ {
    TCR_LOG_FILE = TC_LOG_USEREXT,
    TCR_LOG_NULL
};

typedef enum tcrunderrorcode_ TCRunDErrorCode;
enum tcrunderrorcode_ {
    TCRUND_OK = 0,
    TCRUND_ERR_BAD_OPTIONS,
    TCRUND_ERR_BAD_CONFIG_FILE,
    TCRUND_ERR_NO_CONFIG_FILE,
    TCRUND_ERR_NO_LOG,
    TCRUND_ERR_SERVER_SETUP,
    TCRUND_ERR_SERVER_RUN,
};


int tcr_log_register_methods(void);

#endif /* TCRUND_H */

