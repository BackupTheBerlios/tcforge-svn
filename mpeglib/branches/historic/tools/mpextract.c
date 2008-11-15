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
#include <stdlib.h>
#include <string.h>

#include "mpeglib.h"

#define PROGNAME	"mpextract"

#define STREAM_MAX	(255)

extern char *optarg;
extern int optind, opterr, optopt;

void
version(void) {
    fprintf(stderr, "%s (MPEGlib v\"%s\") (C) 2005 Francesco Romani"
            " (et al)\n\n", PROGNAME, VERSION);
}

void 
usage(void) {
    version();
    fprintf(stderr, PROGNAME" read multimedia file from medium, extract or "
	    "demultiplex\nrequested stream and print to standard output.\n"
            "syntax and output are intentionally similar (or identical)\n"
            "to transcode's tcextract.\n"
            "It's used mainly for debug of MPEGlib\n\n");
    fprintf(stderr, "Usage: %s [options] [-]\n", PROGNAME);
    fprintf(stderr, "\t-i name    input file name [stdin]\n");
    fprintf(stderr, "\t-a track   track number [0]\n");
    fprintf(stderr, "\t-x codec   select source pseudo-codec to extract [mpeg2]\n");
    fprintf(stderr, "\t           give 'help' to get list of supported stream\n");
    fprintf(stderr, "\t-d         emit debug messages from MPEGlib [no]\n");
    fprintf(stderr, "\t-v         print version\n");
    fprintf(stderr, "\t-h         print this message\n");
    fflush(stderr);
}

int
pseudo_codec_to_id(const char *pseudo, int num) {
	int ret = 0;
	
	if(!strcmp(pseudo, "help")) {
		version();
		fprintf(stderr, "supported pseudo-codecs:\n");
		fprintf(stderr, "\t mpeg2    mpeg2 video\n");
		fprintf(stderr, "\t mp3      mpeg audio\n");
		fprintf(stderr, "\t ac3      AC3 audio\n");
		fprintf(stderr, "\t a52      A52 audio "
				"(actually an alias for 'ac3')\n");
		fprintf(stderr, "\t ps1      MPEG private stream (subtitles)"
				" [not yet suported]\n");
		fflush(stderr);
	} else if(!strcmp(pseudo, "mpeg2")) {
		ret = MPEG_STREAM_VIDEO(num);
	} else if(!strcmp(pseudo, "mp3")) {
		ret = MPEG_STREAM_AUDIO(num);
	} else if(!strcmp(pseudo, "ac3") || !strcmp(pseudo, "a52")) {
		ret = MPEG_STREAM_AC3(num);
	} else {
		ret = -1; /*bad pseudo codec */
	}
	return ret;
}

int 
main(int argc, char *argv[]) {
	/** 
	 * step 1: declare the basic data structure used by MPEGlib
	 */
	mpeg_t *MPEG = NULL;
	mpeg_file_t *MFILE = NULL;
	const mpeg_pkt_t *pes = NULL;
	
	int pkts = 0, stream_num = 0, stream_id, ch;
	int use_stdin = TRUE, be_quiet = TRUE;
	const char *fname = NULL, *pseudo = "mpeg2";
	
	while((ch = getopt(argc, argv, "i:x:a:dvh")) != -1) {
		switch(ch) {
	        case 'i':
        	    if(!optarg || ! strlen(optarg)) {
                	usage();
	                exit(EXIT_FAILURE);
        	    }
	            use_stdin = FALSE;
        	    fname = optarg;
	            break;
		case 'a':
        	    if(!optarg || ! strlen(optarg)) {
                	usage();
	                exit(EXIT_FAILURE);
        	    }
		    stream_num = atoi(optarg);
		    if(stream_num < 0 || stream_num > STREAM_MAX) {
                	usage();
	                exit(EXIT_FAILURE);
        	    }
		    break;
		case 'x':
        	    if(!optarg || ! strlen(optarg)) {
                	usage();
	                exit(EXIT_FAILURE);
        	    }
		    pseudo = optarg;
		    break;
        	case 'd':
	            be_quiet = FALSE;
        	    break;
	        case 'v':
        	    version();
	            exit(EXIT_SUCCESS);
        	    break;
	        case 'h':
        	    usage();
	            exit(EXIT_SUCCESS);
        	    break;
	        default:
        	    usage();
	            exit(EXIT_FAILURE);
        	    break;
		}
	}

	/* last check, be compatible with tcextract */
	if(argc == 1) {
		usage();
	        exit(EXIT_FAILURE);
	}

	stream_id = pseudo_codec_to_id(pseudo, stream_num);
	if(stream_id == 0) { /* pseudo == 'help' */
		exit(EXIT_SUCCESS);
	} else if(stream_id < 0) {
		usage();
		exit(EXIT_FAILURE);
	}
	
	if(be_quiet) {
		mpeg_set_logging(mpeg_log_null, stderr);
	}
	
	/* 
	 * step 2: open data source.
	 * mpeg_file_open is the default FILE wrapper used by MPEGlib and will
	 * be always avalaible. It acts just like a plain old fopen() and support
	 * only local files
	 */
	if(use_stdin) {
	        MFILE = mpeg_file_open_link(stdin);
	} else {
        	MFILE = mpeg_file_open(fname, "r");
	        if(!MFILE) {
        		fprintf(stderr, "unable to open: %s\n", argv[1]);
			exit(EXIT_FAILURE);
        	}
	}

	/**
	 * step 3: create a new MPEG descriptor. We need basically a pointer
	 * to a FILE wrapper (mpeg_file_t) and a flag which indicates the kind
	 * of MPEG file to be open.
	 */
	MPEG = mpeg_open(MPEG_TYPE_ANY, MFILE, MPEG_DEFAULT_FLAGS, NULL);
	if(!MPEG) {
		mpeg_file_close(MFILE);
		fprintf(stderr, "mpeg_open() failed (maybe you don't used -i?)\n");
		exit(1);
	}

	/*
	 * step 4a: this is the packet extraction main loop
	 */
	while(1) {
		/*
		 * step 4b: get next packet of desired streams
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
		pes = mpeg_read_packet(MPEG, stream_id);
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

	if(!be_quiet) {
		fprintf(stderr, "extracted %i packets\n", pkts);
	}
	
	/*
	 * all done! :)
	 */
	return 0;
}

