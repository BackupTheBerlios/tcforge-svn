# # Process this file with automake to produce Makefile.in.

AM_CPPFLAGS = \
	$(PTHREAD_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/libtc \
	-I$(top_srcdir)/libtcutil

noinst_LTLIBRARIES = libtcext.la

libtcext_la_SOURCES = ext.c

DEVKIT_HEADERS = \
	tc_avcodec.h \
	tc_lzo.h \
	tc_ogg.h

EXTRA_DIST = \
	$(DEVKIT_HEADERS)

