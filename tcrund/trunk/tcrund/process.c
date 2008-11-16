/*
 * process.c -- transcode process control code.
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

#include "process.h"


struct tcrprocess_ {
    int cmd_fd;
};

TCRProcess *tcr_process_new_from_args(char *args[])
{
    return NULL;
}

TCRProcess *tcr_process_new_from_conf(const char *cfgfile)
{
    return NULL;
}

int tcr_process_run(TCRProcess *proc)
{
    return TC_ERROR;
}

int tcr_process_send_command(TCRProcess *proc, TCRCommandCode cmd,
                             char *args[], char *outbuf)
{
    return TC_ERROR;
}

int tcr_process_status(TCRProcess *proc)
{
    return TC_ERROR;
}

int tcr_process_del(TCRProcess *proc)
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

