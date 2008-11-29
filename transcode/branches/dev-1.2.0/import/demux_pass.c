/*
 *  demux_pass.c
 *
 *  Copyright (C) Thomas Oestreich - May 2002
 *
 *  This file is part of transcode, a video stream processing tool
 *
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "transcode.h"
#include "libtc/libtc.h"
#include "tcinfo.h"

#include "ioaux.h"
#include "aux_pes.h"
#include "seqinfo.h"
#include "demuxer.h"
#include "packets.h"

int gop, gop_pts, gop_cnt;

/* ------------------------------------------------------------
 *
 * demuxer pass_through mode
 *
 * ------------------------------------------------------------*/

void tcdemux_pass_through(info_t *ipipe, int *pass)
{

    int k, id=0;

    int j, i, bytes;
    char *buffer=NULL;

    int payload_id=0, select=PACKAGE_ALL;

    int unit_seek=0, unit, track=0, is_track=0;

    int resync_seq1=0, resync_seq2=INT_MAX, seq_dump, seq_seek;
    int keep_seq = 0;

    int has_pts_dts=0;

    int flag_flush        = 0;
    int flag_eos          = 0;
    int flag_append_audio = 0;
    int flag_notify       = 1;
    int flag_avsync       = 1;
    int flag_skip         = 0;
    int flag_sync_reset   = 0;
    int flag_sync_active  = 0;

    const char *logfile;

    unsigned int packet_size=VOB_PACKET_SIZE;

    double fps;

    // allocate space
    if((buffer = tc_zalloc(packet_size))==NULL) {
      tc_log_perror(__FILE__, "out of memory");
      exit(1);
    }

    // copy info parameter to local variables

    verbose     = ipipe->verbose;
    unit_seek   = ipipe->ps_unit;
    resync_seq1 = ipipe->ps_seq1;
    resync_seq2 = ipipe->ps_seq2;
    track       = ipipe->track;
    select      = ipipe->select;
    fps         = ipipe->fps;
    logfile     = ipipe->name;
    keep_seq    = ipipe->keep_seq;

    j=0;  //packet counter
    i=0;  //skipped packets counter
    k=0;  //unit counter

    // will be switched on as soon start of sequences to flush is reached
    flag_flush=0;

    flag_notify=1;

    flag_avsync=0;
    flag_append_audio=0;

    if(keep_seq) flag_sync_active=1;

    unit=unit_seek;

    seq_seek = resync_seq1;
    seq_dump = resync_seq2 - resync_seq1;

    ++unit_seek;
    ++seq_seek;

    tc_log_msg(__FILE__, "0=0x%x 1=0x%x 2=0x%x 3=0x%x 4=0x%x",
	       pass[0], pass[1], pass[2], pass[3], pass[4]);

    for(;;) {


      /* ------------------------------------------------------------
       *
       * (I) read a 2048k block
       *
       * ------------------------------------------------------------*/

      if((bytes=tc_pread(ipipe->fd_in, buffer, packet_size)) != packet_size) {

	//program end code?
	if(bytes==4) {
	  if(scan_pack_header(buffer, MPEG_PROGRAM_END_CODE)) {
	    if(verbose & TC_DEBUG)
	      tc_log_msg(__FILE__, "(pid=%d) program stream end code detected",
			 getpid());
	    break;
	  }
	}

	if(bytes)
	  tc_log_warn(__FILE__, "invalid program stream packet size (%d/%d)",
		      bytes, packet_size);

	break;
      }

      /* ------------------------------------------------------------
       *
       * (II) packet header ok?
       *
       * ------------------------------------------------------------*/


      if(!scan_pack_header(buffer, TC_MAGIC_VOB)) {

	if(flag_notify && (verbose & TC_DEBUG))
	  tc_log_warn(__FILE__, "(pid=%d) invalid packet header detected",
		      getpid());

	// something else?

	if(scan_pack_header(buffer, MPEG_VIDEO) | scan_pack_header(buffer, MPEG_AUDIO)) {

	    if(verbose & TC_STATS)
		tc_log_msg(__FILE__, "(pid=%d) MPEG system stream detected",
			   getpid());

	    if(scan_pack_header(buffer, MPEG_VIDEO)) payload_id=PACKAGE_VIDEO;
	    if(scan_pack_header(buffer, MPEG_AUDIO)) payload_id=PACKAGE_AUDIO_MP3;

	    // no further processing
	    goto flush_packet;
	} else {

	    tc_log_warn(__FILE__, "(pid=%d) '0x%02x%02x%02x%02x' not yet supported",
			getpid(), buffer[0] & 0xff, buffer[1] & 0xff,
			buffer[2] & 0xff, buffer[3] & 0xff);
	    break;
	}
      } else {

	//MPEG1?
	if ((buffer[4] & 0xf0) == 0x20) {

	  payload_id=PACKAGE_MPEG1;
	  flag_flush=1;

	  if(verbose & TC_STATS)
	    tc_log_msg(__FILE__, "(pid=%d) MPEG-1 video stream detected",
		       getpid());

	  // no further processing
	  goto flush_packet;
	}
      }


      /* ------------------------------------------------------------
       *
       * (III) analyze packet contents
       *
       * ------------------------------------------------------------*/


      // proceed with a valid package, assume defaults

      flag_skip=0;          //do not skip
      has_pts_dts=0;        //no pts_dts stamp
      payload_id=0;         //payload unknown
      flag_sync_reset=0;    //no reset of syncinfo

      id = buffer[17] & 0xff;  //payload id byte

      //MPEG 2?
      if ((buffer[4] & 0xc0) == 0x40) {

	// do not change any flags

	if(verbose & TC_STATS) {
	  //display info only once
	  tc_log_msg(__FILE__, "(pid=%d) MPEG-2 video stream detected",
		     getpid());
	}
      } else {

	//MPEG1
	if ((buffer[4] & 0xf0) == 0x20) {

	  payload_id=PACKAGE_MPEG1;

	  if(verbose & TC_STATS) {
	    //display info only once
	    tc_log_msg(__FILE__, "(pid=%d) MPEG-1 video stream detected",
		       getpid());
	  }
	} else {

	  payload_id=PACKAGE_PASS;

	  if(verbose & TC_DEBUG)
	    tc_log_warn(__FILE__, "(pid=%d) unknown stream packet id detected",
			getpid());
	}

	//flush all MPEG1 stuff
	goto flush_packet;
      }

      /* ------------------------------------------------------------
       *
       * (IV) audio payload
       *
       * ------------------------------------------------------------*/

      // check payload id
      // process this audio packet?

      // sync to AC3 audio mode?
      if(id == P_ID_AC3) payload_id = PACKAGE_PRIVATE_STREAM;

      // check for subid here:

      is_track = id;

      if(payload_id == PACKAGE_PRIVATE_STREAM) {

	  //position of track code
	  uint8_t *_buf=buffer+14;
	  uint8_t *_tmp=_buf + 9 + _buf[8];

	  is_track = (*_tmp);
      }

      /* ------------------------------------------------------------
       *
       * (VII) evaluate scan results - flush packet
       *
       * ------------------------------------------------------------*/

    flush_packet:

      if(is_track == pass[0] || is_track == pass[1] || is_track == pass[2]
	 || is_track == pass[3] || is_track == pass[4] ) {

	  if(tc_pwrite(ipipe->fd_out, buffer, packet_size) != packet_size) {
	      tc_log_perror(__FILE__, "write program stream packet");
	      exit(1);
	  }
      }

      //aborting?
      if(flag_eos) break;

      //total packs (2k each) counter
      ++j;

    } // process next packet/block

    if(buffer!=NULL) free(buffer);

    return;
}

