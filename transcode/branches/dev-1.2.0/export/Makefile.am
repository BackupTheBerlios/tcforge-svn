# # Process this file with automake to produce Makefile.in.

PROFILES = \
	dvd-pal.cfg \
	dvd-ntsc.cfg \
	vcd-pal.cfg \
	vcd-ntsc.cfg \
	svcd-pal.cfg \
	svcd-ntsc.cfg \
	xvcd-pal.cfg \
	xvcd-ntsc.cfg

pkg_DATA = $(PROFILES)

install-exec-hook:
	@mkdir -p $(DESTDIR)$(PROF_PATH)
	@cp $(PROFILES) $(DESTDIR)$(PROF_PATH)

