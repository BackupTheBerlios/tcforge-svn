Mon Aug 22 14:03:39 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev0]
- cvs init

Mon Aug 22 17:45:40 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev1]
- basic infrastructure almost in place (lacks mainly unaligned data access)
- reorganized header files, should be ready for real work
- enter mpeg-core with core functions

Mon Aug 22 19:52:28 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev2]
- basic infrastructure should be now completed
- enter mpeg-ps, now codebase for 0.1 it's almost complete
- endian.h merged into utils.h. Endianess macro seems to cause a bit of
trouble...

Tue Aug 23 18:39:04 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev3]
- reworked main API
- reworked log subsystem: now is per-library, no longer per-instance
- mpeg-core now builds

Wed Aug 24 18:43:34 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev4]
- first milestone reached: now codebase for 0.1 compiles without any
trouble. Now starts the hard part :)

Thu Sep  1 11:25:53 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev5]
- moved common allocation code to mpeg_open/free
- some internal small changes in mpeg-ps (more to come)
- mpeg_ps_next_packet now honores stream_id parameter
- moved to switch'es to function pointers, in order to reduce code duplicated
- new example/tool, mpegvextract (need some work)
- enter full-custom configure script

Thu Sep  1 21:04:45 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev6]
- fixed some typos in tools/
- mpeg_ps_next_packet now finally honours correctly stream_id parameter
- EOF misdetection for truncated streams still unfixed
- mpeg_next_packet now (correctly) return a CONST pointer

Fri Sep  2 14:39:18 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev7]
- build system almost completed (for release 0.1)
- header cleanup

Fri Sep  2 18:31:49 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev8]
- better stream information reporting
- simple regression tester in place
- removed unused verbose setting and some redundant log messages
- more comments and documentation
- build system (small) enhancements
- initial INSTALL informations

Sat Sep  3 10:07:53 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev9]
- almost ready to first beta
- little ROADMAP for 0.1.x releases
- small sparse fixes (i386 build fixes)
- first solution to stream_id VS stream_num solution. Isn't terrible.
- documentation updates

Sat Sep  3 12:26:25 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.1.0rc1]
- documentation
- bugfix on configure

Sun Sep  4 10:51:29 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.1.0rc2]
- new standalone regression tester
- better copyright advice

Mon Sep  5 12:17:09 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.1.0rc3]
- endianess macro fixed (really?)
- endian option on ./configure
- more build system fixes

Mon Sep  5 13:39:30 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.1.0]
- 0.1.0rc3 released as 0.1.0 with no changes
- Release name: "Rise to the Challenge"

Mon Sep  5 18:58:02 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev10]
- first bits of MPEG_TYPE_ANY support

Tue Sep  6 18:53:26 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev11]
- build fixes
- renamed private data to reduce namespace pullution and confusion
- more MPEG_TYPE_ANY support
- started refactoring of mpeg_ps_open (it's a long road baby...)

Wed Sep  7 13:54:17 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev12]
- first bits of smarter probe (sequence header handling)
- initial error reporting structure in place
- mpeg_ps_open() refactoring (start)
- mpeg_pes_next_packet() now on mpeg-core

Thu Sep  8 12:28:01 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev13]
- TCVP upstream catchup
- file type autoprobe broken, so disabled it, for now.
- initial plans for 0.2.0

Fri Sep  9 08:48:32 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev14]
- getbits macro reworked (no longer optimistic stateless)
- I/O reads always returns status
- First file autoprobe works again. Almost.
- some cosmetic fixes

Sat Sep 10 09:16:49 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev15]
- video stream probe almost done
- started mpeg audio probing
- started AC3 audio probing
- mpprobe test program
- configure switch to enable more debug option
- probing regression test

Sun Sep 11 11:20:00 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.0rc1]
- configure fix for library version number
- HACKING document, merged later into README
- initial syncup of tools/* code and comments
- consistency makeup: s/libMPEG/MPEGlib/s in comments and documentation

Mon Sep 12 17:55:02 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.0]
- dev-release renaming
- doc updates (explain release naming) 
- Release name: "Let's Do This Now"

Tue Sep 13 20:26:12 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev16]
- mpeg_stream_type_t and other stuff moved to private header from public one
- probing function hooks in internal mpeg formats table
- a lot of language fixes. Thanks to Anonymous Tester ;)
- ES probing
- open function flags
- mpeg_es_open() and _probe() reworked

Wed Sep 14 09:00:19 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev17]
- ES probe() and open() reworked again
- start to estabilish and enforce error detecting/reporting policy
- enforce CodingStyle (expandtab + tabstop=4)

Wed Sep 14 22:04:05 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev18]
- doc updates (mpeglib.h)
- moved some macros to enums, introduced new data types and started to
update codebase

Thu Sep 15 16:32:50 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev19]
- heavily changed seek policy: now code uses much less seeks.
- PS: now the two stream scan routines are mutually exclusive and selectable
at compile time.
- specific code (PS/ES) now upcalls general routine (mpeg_probe) to do
a proper stream scan. This reduces seeks() and simplify code.

Thu Sep 15 23:38:41 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.1rc1]
- documentation update
- little bugfixes
- largefile defines (for i386)

Fri Sep 16 10:35:24 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.1rc2]
- IS_PCM -> IS_LPCM, integrated in mpeg_ps_probe_streams(), code only
recognize streams, do not probe it (hey, this is an RC release!)
- doc bump
- s/mpeg_errcode_t/mpeg_err_t/
- little mpeg_res_t VS mpeg_err_t cleanup

Fri Sep 16 16:51:54 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.1]
- 0.2.1rc2 released as 0.2.1 with no changes
- Release name: "Diluted"

Fri Sep 16 18:50:50 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev20]
- malloc/free hooks
- changed ROADMAP
- add more doc
- moving to svn

Sun Sep 18 13:57:16 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev21]
- finally fixed the reverse order stream detection bug?
- fixed the ASR misdetection bug? This is a really unclean solution (yet), but 
works and it's close to transcode...
- updated mpprobe, should now display correct audio codec id

Mon Sep 19 15:55:09 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.2rc1]
- typos fixed
- doc update
- tools/mpprobe enhancements
- test/pr_regtest.sh reworked

Tue Sep 27 22:43:26 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.2]
- doc updates
- Release name: "Did my Time"

Sat Oct  1 18:05:05 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev22]
- cosmetic internal changes
- mpeg_pkt_new(): little cleanup.
- mpeg_pes_next_packet(): do not discard packets of an unhandled stream_id
- mpeg_pes_next_packet(): move start code detection to (inline) ancillary
function. Code review, more modularization; common code collected (and more
to come)

Sat Oct  1 23:40:57 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev23]
- now we can deliver also packets containing pack header. 
But beware: This still need *A LOT* more work.
- little style fixes
- mpeg_pes_next_packet(): pack_header roundup. Change API; do not put
pack_header data, but store it in internal buffer (this will be changed
soon, in the next dev snapshot)

Tue Oct  4 11:14:16 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev24]
- Ok, let's face the reality: mpeg_pes_next_packet() and auxiliary stuff 
is seriously broken. Announce so in README and rewrite it heavily.
To further enforce such important change, it will renamed as
mpeg_pes_read_packet()

Tue Oct  4 23:22:17 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev24]
- Now mpeg_pes_read_packet() seems to work decently.
We *REALLY* need further testing.

Wed Oct  5 11:18:22 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev25]
- cleanup and testing
- fixed the early EOF misdetection with vob files (?)

Wed Oct  5 19:40:48 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev26]
- new test tool: mpextract (comparable to transcode's tcextract)
add missing past release names

Wed Oct  5 22:08:01 CEST 2005 - Francesco Romani <fromani@gmail.com> [dev27]
- enter the long-waited transcode test import module: import_mpeg2ng, 
based on MPEGlib capabilities.
- no more features planned, we should now enter in RC phase

Thu Oct  6 16:18:02 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.3.rc1]
- placeholders for future modules and tools
- small doc updates
- little cleanup for mpeg_pkt_t structure

Sat Oct  8 12:35:19 CEST 2005 - Francesco Romani <fromani@gmail.com> [0.2.3rc2]
- little cleanup
- documentation update

Sat Oct  8 13:59:24 CEST 2005
- fixed some minor FIXMEs
- Release name: "Revolution/Revolucion"
