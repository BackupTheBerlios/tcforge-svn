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

#define PROGNAME		"mpegprobe"

int 
main(int argc, char *argv[]) {
	/* 
	 * step 1: declare the basic data structure used by MPEGlib
	 */
	int num = 0, i = 0;
	mpeg_t *MPEG = NULL;
	mpeg_file_t *MFILE = NULL;
	const mpeg_stream_t *mps = NULL;
	
	if(argc != 2) {
		fprintf(stderr, "mpegprobe prints some informations about a given"
				"mpeg file using MPEGlib detection routines\n");
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
	
	/*
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
	 * step 4a: now we print what we have found. How many streams we have?
	 */
	num = mpeg_get_stream_number(MPEG);
	printf("(%s) found %i A/V streams\n", __FILE__, num);
	for(i = 0; i < num; i++) {
		/*
		 * step 4b: get a descriptor for stream #i. This is obviously a constant
		 * reference. We do not free() them.
		 */
		mps = mpeg_get_stream_info(MPEG, i);
		if(mps != NULL) {
			printf("(%s) stream #%02i:\n", __FILE__, i);
			/*
			 * step 4c: MPEGlib provides a support function to print main
			 * stream informations on a standard FILE. a value of '0' for
			 * a fields means 'not detected'
			 */
			mpeg_print_stream_info(mps, stdout);
		} else {
			printf("(%s) WARNING: can't get stream information for stream #%02i\n",
				__FILE__, i);
		}
	}
	
	/*
	 * step 5: finalize the MPEG descriptor. Weird things can happen if we do not
	 * follow the REVERSE order of finalization strictly.
	 */
	mpeg_close(MPEG);

	/*
	 * step 5: closing the FILE wrapper
	 */
	mpeg_file_close(MFILE);

	/**
	 * all done! :)
	 */
	return 0;
}
