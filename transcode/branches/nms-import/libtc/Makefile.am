# # Process this file with automake to produce Makefile.in.

AM_CPPFLAGS = \
	$(PTHREAD_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/libtcutil

noinst_LTLIBRARIES = libtc.la

if HAVE_IBP
XIO_FILES = libxio.c
endif

libtc_la_SOURCES = \
	framecode.c \
	ratiocodes.c \
	tc_functions.c \
	tccodecs.c \
	tcformats.c \
	tcframes.c 

noinst_HEADERS = \
	framecode.h \
	libtc.h \
	ratiocodes.h \
	tccodecs.h \
	tcformats.h \
	tcframes.h

EXTRA_DIST = \
	$(DEVKIT_HEADERS)

