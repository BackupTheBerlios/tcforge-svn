/*
 *  avifix.c
 *
 *  Copyright (C) Thomas Oestreich - June 2001
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "avilib/avilib.h"

#define EXE "avifix"

/* AVI_info is no longer in avilib */
void AVI_info(avi_t *avifile);

void version(void)
{
    printf("%s (%s v%s) (C) 2001-2003 Thomas Oestreich,"
                          " 2003-2008 Transcode Team\n",
                        EXE, PACKAGE, VERSION);
}

static void usage(int status)
{
    version();
    printf("\nUsage: %s [options]\n", EXE);
    printf("    -i name           AVI file name\n");
    printf("    -F string         video codec FOURCC\n");
    printf("    -f val1,val2      video frame rate (fps=val1/val2)\n");
    printf("    -N 0xnn           audio format identifier\n");
    printf("    -b bitrate        audio encoder bitrate (kbps)\n");
    printf("    -e r[,b[,c]]      audio stream parameter (samplerate,bits,channels)\n");
    printf("    -a num            audio track number [0]\n");
    printf("    -v                print version\n");
    exit(status);
}

int main(int argc, char *argv[])
{
  struct common_struct rtf;
  struct AVIStreamHeader ash, vsh;

  avi_t *avifile;

  int n, ch, brate=0, bytes, val1=0, val2=1;

  int a_rate, a_chan, a_bits;

  int fd;

  int id=0, aud=0, vid=0, rat=0, cha=0, sam=0, bit=0, fps=0;

  int track_num=0;

  long ah_off, af_off, vh_off, vf_off;

  char *str=NULL;

  char codec[5];

  char *filename=NULL;

  ac_init(AC_ALL);

  if(argc==1) usage(EXIT_FAILURE);

  while ((ch = getopt(argc, argv, "f:i:N:F:vb:e:a:?h")) != -1)
    {

      switch (ch) {

      case 'N':

	  if(optarg[0]=='-') usage(EXIT_FAILURE);
	  id = strtol(optarg, NULL, 16);

	  if(id <  0)
	      fprintf(stderr, "invalid parameter set for option -N");

	  aud = 1;
	  break;


      case 'a':

	if(optarg[0]=='-') usage(EXIT_FAILURE);
	track_num = atoi(optarg);

	if(track_num<0) usage(EXIT_FAILURE);

	break;


      case 'f':

	  if(optarg[0]=='-') usage(EXIT_FAILURE);
	  n=sscanf(optarg,"%d,%d", &val1, &val2);
	  if(n!=2 || val1 < 0 || val2 < 0) fprintf(stderr, "invalid parameter set for option -f");

	  fps = 1;
	  break;

      case 'F':

	  if(optarg[0]=='-') usage(EXIT_FAILURE);
	  str = optarg;

	  if(strlen(str) > 4 || strlen(str) == 0)
	      fprintf(stderr, "invalid parameter set for option -F");

	  vid = 1;
	  break;

      case 'i':

	  if(optarg[0]=='-') usage(EXIT_FAILURE);
	  filename = optarg;
	  break;

      case 'b':

	  if(optarg[0]=='-') usage(EXIT_FAILURE);
	  brate = atoi(optarg);
	  rat = 1;
	  break;

      case 'v':

	  version();
	  exit(0);

      case 'e':

	if(optarg[0]=='-') usage(EXIT_FAILURE);

	n=sscanf(optarg,"%d,%d,%d", &a_rate, &a_bits, &a_chan);

	switch (n) {

	case 3:
	  cha=1;
	case 2:
	  bit=1;
	case 1:
	  sam=1;
	  break;
	default:
	  fprintf(stderr, "invalid parameter set for option -e");
	}

	break;

      case 'h':
	  usage(EXIT_SUCCESS);
      default:
	  usage(EXIT_FAILURE);
      }
    }


  if(filename==NULL) usage(EXIT_FAILURE);

  printf("[%s] scanning AVI-file %s for header information\n", EXE, filename);

  // open file
  if(NULL == (avifile = AVI_open_input_file(filename,1))) {
      AVI_print_error("AVI open");
      exit(1);
  }

  AVI_info(avifile);

  //switch to requested audio_channel

  if(AVI_set_audio_track(avifile, track_num)<0) {
    fprintf(stderr, "invalid audio track\n");
  }

  /* -----------------------------------------------
   *
   * get header substructure offsets
   *
   * -----------------------------------------------*/

  // get audio information structure offset
  ah_off = AVI_audio_codech_offset(avifile);
  af_off = AVI_audio_codecf_offset(avifile);

  // get video information structure offset
  vh_off = AVI_video_codech_offset(avifile);
  vf_off = AVI_video_codecf_offset(avifile);

  AVI_close(avifile);

  if(0) printf("AH [0x%lx] AF [0x%lx] VH [0x%lx] VF [0x%lx]\n", ah_off, af_off, vh_off, vf_off);

  // open file for writing
  if((fd = open(filename, O_RDWR))<0) {
      perror("open");
      exit(1);
  }

  // check video codecs
  lseek(fd,vh_off,SEEK_SET);

  read(fd, codec, 4);
  codec[4] = 0;

  if(0) printf("found video codec (strh): %s\n", ((strlen(codec)==0)? "RGB": codec));

  lseek(fd,vf_off,SEEK_SET);

  read(fd, codec, 4);
  codec[4] = 0;

  if(0) printf("found video codec (strf): %s\n", ((strlen(codec)==0)? "RGB": codec));

  //----------------------------
  //
  //  VIDEO
  //
  //----------------------------

  if(fps) {

      lseek(fd, vh_off-4, SEEK_SET);
      if((bytes=read(fd, (char*) &vsh, sizeof(vsh))) != sizeof(vsh)) {
	  fprintf(stderr, "(%s) error reading AVI-file\n", __FILE__);
	  exit(1);
    }

      if(fps) {
	  vsh.dwRate  = (long) val1;
	  vsh.dwScale = (long) val2;
      }

      lseek(fd, vh_off-4, SEEK_SET);
      write(fd, (char*) &vsh, sizeof(vsh));
  }


  // set video codec

  if(vid) {
      lseek(fd,vh_off,SEEK_SET);

      // video
      if(strncmp(str,"RGB",3)==0) {
	  memset(codec,0,4);
	  write(fd, codec, 4);
      } else
	  write(fd, str, 4);

      lseek(fd,vf_off,SEEK_SET);

      // video
      if(strncmp(str,"RGB",3)==0) {
	  memset(codec,0,4);
	  write(fd, codec, 4);
      } else
	  write(fd, str, 4);
  }

  //----------------------------
  //
  //  AUDIO
  //
  //----------------------------

  if(aud | rat | bit | cha | sam) {

      lseek(fd, ah_off, SEEK_SET);
      if((bytes=read(fd, (char*) &ash, sizeof(ash))) != sizeof(ash)) {
	  fprintf(stderr, "(%s) error reading AVI-file\n", __FILE__);
	  exit(1);
      }


      lseek(fd, af_off, SEEK_SET);
      if((bytes=read(fd, (char*) &rtf, sizeof(rtf))) != sizeof(rtf)) {
	  fprintf(stderr, "(%s) error reading AVI-file\n", __FILE__);
	  exit(1);
    }


      if(aud) rtf.wFormatTag = (unsigned short) id;

      if(rat) {
	  rtf.dwAvgBytesPerSec = (long) 1000*brate/8;
	  ash.dwRate = (long) 1000*brate/8;
	  ash.dwScale = 1;
      }

      if(cha) rtf.wChannels = (short) a_chan;

      if(bit) rtf.wBitsPerSample = (short) a_bits;

      if(sam) rtf.dwSamplesPerSec = (long) a_rate;

      lseek(fd, ah_off ,SEEK_SET);
      write(fd, (char*) &ash, sizeof(ash));

      lseek(fd, af_off ,SEEK_SET);
      write(fd, (char*) &rtf, sizeof(rtf));

  }

  close(fd);

  // verify AVI-file header

  if(NULL == (avifile = AVI_open_input_file(filename,1))) {
      AVI_print_error("AVI open");
      exit(1);
  }

  printf("[%s] successfully updated AVI file %s\n", EXE, filename);

  AVI_info(avifile);

  AVI_close(avifile);

  return(0);
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
