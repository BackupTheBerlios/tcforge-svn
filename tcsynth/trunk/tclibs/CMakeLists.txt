
set(ACLIB_SOURCES
    aclib/accore.c aclib/average.c aclib/imgconvert.c
    aclib/img_rgb_packed.c aclib/img_yuv_mixed.c
    aclib/img_yuv_packed.c aclib/img_yuv_planar.c
    aclib/img_yuv_rgb.c aclib/memcpy.c aclib/rescale.c)

set(LIBTCUTIL_SOURCES 
    libtcutil/cfgfile.c libtcutil/tcglob.c libtcutil/iodir.c
    libtcutil/ioutils.c libtcutil/tclist.c libtcutil/logging.c
    libtcutil/memutils.c libtcutil/optstr.c libtcutil/strlcat.c
    libtcutil/strlcpy.c libtcutil/strutils.c)

set(LIBTCMODULE_SOURCES
    libtcmodule/tcmodule.c libtcmodule/tcmoduleinfo.c)

set(LIBTCAUDIO_SOURCES
    libtcaudio/tcaudio.c)

set(LIBTCVIDEO_SOURCES
    libtcvideo/tcvideo.c libtcvideo/zoom.c)

add_definitions(-DHAVE_CONFIG_H)
include_directories(${tcsynth_SOURCE_DIR})
include_directories(${tcsynth_SOURCE_DIR}/tclibs)
include_directories(${tcsynth_SOURCE_DIR}/tclibs/src) # ugh

include_directories(${tcsynth_SOURCE_DIR}/tclibs/aclib)
include_directories(${tcsynth_SOURCE_DIR}/tclibs/libtc)
include_directories(${tcsynth_SOURCE_DIR}/tclibs/libtcutil)
include_directories(${tcsynth_SOURCE_DIR}/tclibs/libtcmodule)
include_directories(${tcsynth_SOURCE_DIR}/tclibs/libtcaudio)
include_directories(${tcsynth_SOURCE_DIR}/tclibs/libtcvideo)
include_directories(${tcsynth_SOURCE_DIR}/tclibs/tccore)

add_library(ac STATIC ${ACLIB_SOURCES})
add_library(tcutil STATIC ${LIBTCUTIL_SOURCES})
add_library(tcmodule STATIC ${LIBTCMODULE_SOURCES})
add_library(tcaudio STATIC ${LIBTCAUDIO_SOURCES})
add_library(tcvideo STATIC ${LIBTCVIDEO_SOURCES})

set_target_properties(ac PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")
set_target_properties(tcutil PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")
set_target_properties(tcmodule PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")
set_target_properties(tcaudio PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")
set_target_properties(tcvideo PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")

