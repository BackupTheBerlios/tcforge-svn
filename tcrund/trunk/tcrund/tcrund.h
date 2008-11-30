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


#define TCRUND_PROTOCOL_VERSION         1
#define TCRUND_SERVER_VERSION           1


#define TCRUND_DEFAULT_HOST		"127.0.0.1"
#define TCRUND_DEFAULT_PORT		39000
#define TCRUND_DEFAULT_LOG_DIR		"/var/log/tcrund/"
#define TCRUND_DEFAULT_CONF_PATH	"/etc/tcrund/"
#define TCRUND_DEFAULT_FILES_PATH	"/srv/tcrund/"
#define TCRUND_DEFAULT_OUT_FMT_STRING	"out" // FIXME

#define TCRUND_CONFIG_FILE_MAIN		"general"

#define TCRUND_DEFAULT_CONFIG_PATH      "/etc/tcrund/tcrund.cfg"
#define TCRUND_DEFAULT_ACCOUNT_PATH     "/etc/tcrund/users.cfg"
#define TCRUND_DEFAULT_LOG_PATH         "/var/log/tcrund.log"


typedef struct tcrconfig_ TCRConfig;
struct tcrconfig_ {
    /* those WILL BE present into configuration file */
    char *host;
    int  port;

    char *logs_dir;

    char *tc_conf_dir;

    char *files_dir;
    char *out_fmt_string;

    /* those will NOT be present into configuration file */
    int  debug_mode;
};

typedef enum tcrlogtarget_ TCRLogTarget;
enum tcrlogmode_ {
    TCR_LOG_TARGET_FILE = TC_LOG_TARGET_USEREXT,
    TCR_LOG_TARGET_NULL
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

/* logging */

int tcr_log_register_methods(void);
int tcr_log_open(const char *logfile,
                 TCLogTarget target, TCVerboseLevel verbose);


#endif /* TCRUND_H */

