/*
 *  extract_mpeg2.c
 *
 *  Standalone modify: 
 *  (C) Francesco Romani <fromani@gmail.com> - September 2005
 *
 *  Copyright (C) Thomas Östreich - June 2001
 *  Copyright (C) 1999-2001 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 *  This file is part of transcode, a video stream  processing tool
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define BUFFER_SIZE 262144
static uint8_t buffer[BUFFER_SIZE];
static FILE *in_file, *out_file;

/* yes, I'm not so confident with such code. It works, and this is all. */

static void ps_loop (void)
{
    static int mpeg1_skip_table[16] = {
	     1, 0xffff,      5,     10, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
    };

    uint8_t * buf;
    uint8_t * end;
    uint8_t * tmp1=NULL;
    uint8_t * tmp2=NULL;
    int complain_loudly, packets = 0;

    complain_loudly = 1;
    buf = buffer;
    
    do {
      end = buf + fread (buf, 1, buffer + BUFFER_SIZE - buf, in_file);
      buf = buffer;

      //scan buffer
      while (buf + 4 <= end) {
	
	// check for valid start code
	if (buf[0] || buf[1] || (buf[2] != 0x01)) {
	  if (complain_loudly) {
	    fprintf (stderr, "(%s) missing start code at %#lx\n",
		     __FILE__, ftell (in_file) - (end - buf));
	    if ((buf[0] == 0) && (buf[1] == 0) && (buf[2] == 0))
	      fprintf (stderr, "(%s) incorrect zero-byte padding detected - ignored\n", __FILE__);
	    complain_loudly = 0;
	  }
	  buf++;
	  continue;
	}// check for valid start code 
	
	
	switch (buf[3]) {
	  
	case 0xb9:	/* program end code */
	  return;
	  
	case 0xba:	/* pack header */

	  /* skip */
	  if ((buf[4] & 0xc0) == 0x40)	        /* mpeg2 */
	    tmp1 = buf + 14 + (buf[13] & 7);
	  else if ((buf[4] & 0xf0) == 0x20)	/* mpeg1 */
	    tmp1 = buf + 12;
	  else if (buf + 5 > end)
	    goto copy;
	  else {
	    fprintf (stderr, "(%s) weird pack header\n", __FILE__);
	    exit(1);
	  }
	  
	  if (tmp1 > end)
	    goto copy;
	  buf = tmp1;
	  break;
	  
	case 0xe0:	/* video */
	case 0xe1:	/* video */
	case 0xe2:	/* video */
	case 0xe3:	/* video */
	case 0xe4:	/* video */
	case 0xe5:	/* video */
	case 0xe6:	/* video */
	case 0xe7:	/* video */
	case 0xe8:	/* video */
	case 0xe9:	/* video */

	  tmp2 = buf + 6 + (buf[4] << 8) + buf[5];
	  if (tmp2 > end)
	    goto copy;
	  if ((buf[6] & 0xc0) == 0x80)	/* mpeg2 */
	    tmp1 = buf + 9 + buf[8];
	  else {	/* mpeg1 */
	    for (tmp1 = buf + 6; *tmp1 == 0xff; tmp1++)
	      if (tmp1 == buf + 6 + 16) {
		fprintf (stderr, "(%s) too much stuffing\n", __FILE__);
		buf = tmp2;
		break;
	      }
	    if ((*tmp1 & 0xc0) == 0x40)
	      tmp1 += 2;
	    tmp1 += mpeg1_skip_table [*tmp1 >> 4];
	  }
	  
	  if (tmp1 < tmp2) {
            packets++;
	    if (fwrite (tmp1, tmp2-tmp1, 1, out_file) != 1)
	      exit(0); /* decoder has exited */
	  }
	  buf = tmp2;
	  break;

	default:
	  if (buf[3] < 0xb9) fprintf (stderr, "(%s) broken stream - skipping data\n", __FILE__);

	  /* skip */
	  tmp1 = buf + 6 + (buf[4] << 8) + buf[5];
	  if (tmp1 > end)
	    goto copy;
	  buf = tmp1;
	  break;

	} //start code selection
      } //scan buffer
      
      if (buf < end) {
      copy:
	/* we only pass here for mpeg1 ps streams */
	memmove (buffer, buf, end - buf);
      }
      buf = buffer + (end - buf);
      
    } while (end == buffer + BUFFER_SIZE);

    fprintf(stderr, "(%s) delivered %i packets (just a guess)\n", 
		    __FILE__, packets);
}

int main() {
	in_file = stdin;
	out_file = stdout;

	ps_loop();

	return 0;
}
