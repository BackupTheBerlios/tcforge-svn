/*
 *  Copyright (C) 2005 Francesco Romani <fromani@gmail.com>
 * 
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 *  copies of the Software, and to permit persons to whom the Software is 
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in 
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include <stdio.h>

#include "mpeglib.h"

#define PROGNAME		"mpegvextract"

int 
main(int argc, char *argv[]) {
	/** 
	 * step 1: declare the basic data structure used by MPEGlib
	 */
	mpeg_t *MPEG = NULL;
	mpeg_file_t *MFILE = NULL;
	const mpeg_pkt_t *pes = NULL;
	int pkts = 0;
	
	if(argc != 2) {
		fprintf(stderr, "mpegvextract extracts the first video stream from"
				"a given mpeg file and emits it on stdout\n");
		fprintf(stderr, "usage: %s mpegfile\n", PROGNAME);
		exit(EXIT_SUCCESS);
	}

	/* 
	 * step 2: open data source.
	 * mpeg_file_open is the default FILE wrapper used by MPEGlib and will
	 * be always avalaible. It acts just like a plain old fopen() and support
	 * only local files
	 */
	MFILE = mpeg_file_open(argv[1], "r");
	if(!MFILE) {
		fprintf(stderr, "unable to open: %s\n", argv[1]);
		exit(1);
	}
	
	/**
	 * step 3: create a new MPEG descriptor. We need basically a pointer
	 * to a FILE wrapper (mpeg_file_t) and a flag which indicates the kind
	 * of MPEG file to be open.
	 */
	MPEG = mpeg_open(MPEG_TYPE_ANY, MFILE, MPEG_DEFAULT_FLAGS, NULL);
	if(!MPEG) {
		mpeg_file_close(MFILE);
		fprintf(stderr, "mpeg_open() failed\n");
		exit(1);
	}

	/*
	 * step 4a: this is the packet extraction main loop
	 */
	while(1) {
		/*
		 * step 4b: get next packet of desired video streams.
		 *
		 * PLEASE NOTE: you must give to this function the stream ID of
		 * desired stream, not the stream number. Stream ID is the real
		 * ID under the MPEG terminology, and can be found using 'stream_id'
		 * field of mpeg_stream_t descriptor (see mpeg_get_stream_info()).
		 * stream NUM is simply the progressive number of any stream which
		 * MPEGlib assign internally when scans the MPEG file.
		 * You can always traslate stream NUM to stream ID using the bundled
		 * MPEG_STREAM_* macros.
		 * 
		 * Actually MPEGlib has some limitations. When looking for packets
		 * belonging to a given stream, MPEGlib silently discard ALL extraneous
		 * packets. So caller can't get old packets of different A/V streams.
		 * There is a workaround for this. It's safe to have multiple
		 * mpeg_file_t and mpeg descriptors pointing to the same (real) file,
		 * each one extracts packets for a different A/V strem.
		 * This is undoubtfully unconfortable and will be changed in future
		 * releases.
		 */
		pes = mpeg_read_packet(MPEG, MPEG_STREAM_VIDEO(0));
		/*
		 * step 4c: mpeg_read_packet returns a pointer to a mpeg_pkt_t
		 * which contains the real packet data. It's unsafe to change content
		 * of this structure, so it's redurned in RO mode.
		 * mpeg_read_packet returns NULL when read fails or stream ends.
		 */
		if(!pes) {
			break;
		}
		// XXX
		fwrite(pes->data, pes->size, 1, stdout); 
		/*
		 * step 4d: you should return back packet resources using mpeg_pkt_del()
		 * on pointer returned by read_packet. Never use free() directly, it may lead
		 * to undefined behaviours
		 */
		mpeg_pkt_del(pes);
		pkts++;
	}
	
	/*
	 * step 5: finalize the MPEG descriptor. Weird things can happen if we do not
	 * follow the REVERSE order of finalization strictly.
	 */
	mpeg_close(MPEG);

	/*
	 * step 6: closing the FILE wrapper
	 */
	mpeg_file_close(MFILE);

	fprintf(stderr, "extracted %i MPEG video packets\n", pkts);
	
	/*
	 * all done! :)
	 */
	return 0;
}
