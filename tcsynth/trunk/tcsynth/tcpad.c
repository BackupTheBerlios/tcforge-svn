/*
 * tcpad.c -- A/V scripting language built on top of transcode.
 * (C) 2008 Francesco Romani <fromani at gmail dot com>
 *
 * This file is part of tcpad, the A/V scripting language.
 *
 * tcpad is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * tcpad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TCPAD_H
#define TCPAD_H

#include "libtcmodule/tcmodule-core.h"
#include "framepool.h"
#include "tcfifo.h"

typedef enum tcpadstatus_ TCPadStatus;
enum tcpadstatus {
};

typedef enum tcpadtype_ TCPadType;
enum tcpadtype_ {
};

typedef struct tcpadchannel_ TCPadChannel;
struct tcpadchannel_ {
    int id;
    int status;
    TCFifo fifo;
};

enum {
    TC_PAD_MAX_CHANNELS =  2,
    TC_PAD_LINK_SRC     = -1,
    TC_PAD_LINK_DST     = +1,	
};

typedef struct tcpad_ TCPad;
struct tcpad_ {
    int id;

    TCPadChannel src[TC_PAD_MAX_CHANNELS];
    TCPadChannel dst[TC_PAD_MAX_CHANNELS];

    TCPadType type;

    TCModule *module;
};


TCPad *tc_pad_get_null(void);

TCPad *tc_pad_new_empty(TCPadType type);

TCPad *tc_pad_new_from_module(const char *modname);

int tc_pad_attach(TCPad *pad, TCModule *mod);

int tc_pad_link(TCPad *self, int mode, TCPad *pad);

TCPadStatus tc_pad_status(TCPad *pad);

int tc_pad_process(TCPad *pad);

#endif /* TCPAD_H */
