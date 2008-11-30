/*
 * process.h -- transcode process control code.
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

#ifndef PROCESS_H
#define PROCESS_H

enum {
     TCR_PROC_CMD_OUT_BUF_LEN	= 256
};

typedef enum tcrcommandcode_ TCRCommandCode;
enum tcrcommandcode_ {
     TCR_COMMAND_CODE_UNKNOWN   =  0,
     TCR_COMMAND_CODE_STOP      =  1,
     TCR_COMMAND_CODE_STATUS    =  2,
};

typedef enum tcrprocesserror_ TCRProcessError;
enum tcrprocesserror_ {
     TCR_ERROR_GENERIC = 64
};

typedef struct tcrprocess_ TCRProcess;

TCRProcess *tcr_process_new_from_conf(const char *cfgfile);

int tcr_process_run(TCRProcess *proc, const char *rundir);

int tcr_process_send_command(TCRProcess *proc, TCRCommandCode cmd,
                             char *args[], char *outbuf);

int tcr_process_status(TCRProcess *proc);

int tcr_process_del(TCRProcess *proc);

#endif /* PROCESS_H */
