# Makefile for mpeglib -- Francesco Romani <fromani@gmail.com> 1 September 2005

CC = gcc
DEFINES = -D_ISOC99_SOURCE -D_GNU_SOURCE -D_REENTRANT \
	-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 
CFLAGS = -O2 -g -std=c99 -Wall -Werror -finline-functions $(DEFINES) -fPIC -DPIC
SHFLAGS = -shared -Wl,-soname,libMPEG.so.0 -Wl,-export-dynamic

OBJECTS=mpeg-crc32.o mpeg-base.o mpeg-core.o mpeg-probe.o mpeg-es.o mpeg-ps.o

TARGETS = build-lib build-tests
INSTALL_TARGETS = install-header install-pkgconfig install-lib 
UNINSTALL_TARGETS = uninstall-header uninstall-pkgconfig uninstall-lib 

include config.mak

all: $(TARGETS)

install: $(INSTALL_TARGETS)

uninstall: $(UNINSTALL_TARGETS)

# high level targets

build-lib: libMPEG.a

build-shlib: libMPEG.so.0

build-tools: 
	@make -C tools all

build-mods: 
	@make -C transcode all

build-tests:
	@make -C test all


install-header:
	@echo installing header file\(s\) in $(PREFIX)/include ...
	@install -D -m 644 mpeglib.h $(PREFIX)/include/mpeglib.h


install-pkgconfig:
	@echo installing pkgconfig support file in $(PREFIX)/lib/pkgconfig ...
	@install -D -m 644 libMPEG.pc $(PREFIX)/lib/pkgconfig/libMPEG.pc

install-lib:
	@echo installing static library in $(PREFIX)/lib	
	@install -D -m 644 libMPEG.a $(PREFIX)/lib/libMPEG.a

install-shlib:
	@echo installing shared library in $(PREFIX)/lib	
	@install -D -m 644 libMPEG.so.0 $(PREFIX)/lib/libMPEG.so.0
	@cd $(PREFIX)/lib && ln -s libMPEG.so.0 libMPEG.so
	@cd $(PREFIX)/lib && ln -s libMPEG.so.0 libMPEG.so.$(LIBVERSION)


uninstall-header:
	@echo uninstalling header file\(s\) in $(PREFIX)/include ...
	@rm -f $(PREFIX)/include/mpeglib.h
	
uninstall-lib:
	@echo uninstalling static library in $(PREFIX)/lib
	@rm -f $(PREFIX)/lib/libMPEG.a

uninstall-shlib:
	@echo uninstalling shared library in $(PREFIX)/lib
	@rm -f $(PREFIX)/lib/libMPEG.so*
	
uninstall-pkgconfig:
	@echo uninstalling pkgconfig support file in $(PREFIX)/lib/pkgconfig ...
	@rm -f $(PREFIX)/lib/pkgconfig/libMPEG.pc

archive:
	@echo creating archive ../mpeglib-$(VERSION).tgz ...
	@cd .. && tar zc -f mpeglib-$(VERSION).tgz mpeglib/

# targets

mpeg-crc32.o: mpeg-crc32.c
	@echo "compiling $< ..."
	@$(CC) $(CFLAGS) -c -o $@ $<

mpeg-base.o: mpeg-base.c
	@echo "compiling $< ..."
	@$(CC) $(CFLAGS) -c -o $@ $<

mpeg-core.o: mpeg-core.c
	@echo "compiling $< ..."
	@$(CC) $(CFLAGS) -c -o $@ $<

mpeg-probe.o: mpeg-probe.c
	@echo "compiling $< ..."
	@$(CC) $(CFLAGS) -c -o $@ $<

mpeg-ps.o: mpeg-ps.c
	@echo "compiling $< ..."
	@$(CC) $(CFLAGS) -c -o $@ $<

mpeg-es.o: mpeg-es.c
	@echo "compiling $< ..."
	@$(CC) $(CFLAGS) -c -o $@ $<
	
libMPEG.a: $(OBJECTS)
	@echo "creating static library $@ ..."
	@ar cru $@ $(OBJECTS)
	@ranlib $@
	@echo "...done"

libMPEG.so.0: $(OBJECTS)
	@echo "creating shared library $@ ..."
	@$(CC) $(SHFLAGS) -o $@ $(OBJECTS)
	@echo "...done"

# utilities

clean:
	@echo cleaning library ...
	@rm -f $(OBJECTS) libMPEG.a libMPEG.so*
	@make -s -C tools clean
	@make -s -C transcode clean
	@make -s -C test clean
	@echo cleaning archive data \(if any\) ...
	@rm -f mpeglib-$(VERSION).tgz
	@echo cleaning coredumps \(if any\) ...
	@rm -f core*
	@echo ... all done
