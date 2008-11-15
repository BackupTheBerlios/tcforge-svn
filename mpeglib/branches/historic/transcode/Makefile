#

CC = gcc
DEFINES = -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
INCLUDES = -I"$(TCPREFIX)" -I"$(TCPREFIX)/src" -I"$(TCPREFIX)/import" -I"$(TCPREFIX)/aclib" -I../
MPEG2_CFLAGS = `pkg-config libmpeg2 --cflags` `pkg-config libmpeg2convert --cflags`
CFLAGS = $(INCLUDES) -O2 -g -Wall -Werror $(DEFINES) $(MPEG2_CFLAGS) -shared -fPIC
LIBS = `pkg-config libmpeg2 --libs` `pkg-config libmpeg2convert --libs`

include ../config.mak

DEFINES += -DVERSION=\"$(VERSION)\" 

PROGS = import_mpeg.so
 
all: $(PROGS)

# targets

import_mpeg.so: import_mpeg.c ../libMPEG.a
	@echo building $@ ...
	@$(CC) $(CFLAGS) $(LIBS) -o $@ import_mpeg.c ../libMPEG.a

# utilities

clean:
	@echo cleaning modules ...
	@rm -f $(PROGS) core*

install:
	@echo "now you can copy module(s) on your transcode's module directory"
	@echo "default: /usr/local/lib/transcode (if transcode was compiled from sources)"
	@echo "default  /usr/lib/transcode (if transcode was installed using a package)"

