# # Process this file with automake to produce Makefile.in.

AM_CPPFLAGS = \
	$(PTHREAD_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/libtcutil

noinst_LTLIBRARIES = libtcmodule.la

libtcmodule_la_SOURCES = \
	tcmodule.c \
	tcmoduleinfo.c

noinst_HEADERS = \
	tcmodule-core.h \
	tcmodule-data.h \
	tcmodule-info.h \
	tcmodule-plugin.h

EXTRA_DIST = \
	$(DEVKIT_HEADERS)

