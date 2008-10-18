# # Process this file with automake to produce Makefile.in.

AM_CPPFLAGS = \
	$(PTHREAD_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/import

pkgdir = $(MOD_PATH)

if HAVE_V4L
MOD = import_v4l.la import_v4l2.la
endif

pkg_LTLIBRARIES = $(MOD)

import_v4l_la_SOURCES = import_v4l.c common.c video.c audio.c frequencies.c
import_v4l_la_LDFLAGS = -no-undefined -module -avoid-version

import_v4l2_la_SOURCES = import_v4l2.c
import_v4l2_la_LDFLAGS = -module -avoid-version


EXTRA_DIST = \
	audio.h \
	channels.h \
	common.h \
	counter.h \
	frequencies.h \
	vcr.h \
	video.h \
	videodev.h \
	videodev2.h
