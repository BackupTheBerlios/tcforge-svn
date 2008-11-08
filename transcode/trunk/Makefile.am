# # Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = foreign

if USE_DLDARWIN
LIBDLDARWIN = libdldarwin
endif

if HAVE_PVM3
PVM3 = pvm3
endif

SUBDIRS = \
	aclib \
	avilib \
	$(LIBDLDARWIN) \
	libtcutil \
	libtcmodule \
	libtcext \
	libtc \
	libtcaudio \
	libtcvideo \
	encode \
	export \
	filter \
	import \
	multiplex \
	$(PVM3) \
	src \
	testsuite \
	tools \
	docs

EXTRA_DIST = STYLE transcode.spec

docsdir = $(datadir)/doc/transcode
docs_DATA = ChangeLog

tcincludedir = $(includedir)/schroedinger-$(VERSION)

DEVKIT_PKGCONFIG = transcode-$(VERSION).pc

transcode-$(VERSION).pc: transcode.pc
	cp transcode.pc transcode-$(VERSION).pc

tcconfig.h: config.h
	cp config.h tcconfig.h

CLEANFILES = transcode-$(VERSION).pc tcconfig.h

devkit: tcconfig.h
	mkdir -p $(tcincludedir)
	cp tcconfig.h $(tcincludedir)

