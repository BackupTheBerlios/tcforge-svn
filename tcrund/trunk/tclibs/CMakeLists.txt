
set(LIBTCUTIL_SOURCES 
    libtcutil/cfgfile.c libtcutil/tcglob.c libtcutil/iodir.c
    libtcutil/ioutils.c libtcutil/tclist.c libtcutil/logging.c
    libtcutil/memutils.c libtcutil/optstr.c libtcutil/strutils.c)

if(HAVE_GETTIMEOFDAY)
    set(LIBTCUTIL_SOURCES ${LIBTCUTIL_SOURCES} libtcutil/tctimer.c)
endif(HAVE_GETTIMEOFDAY)

if(NOT HAVE_STRLCAT)
    set(LIBTCUTIL_SOURCES ${LIBTCUTIL_SOURCES} libtcutil/strlcat.c)
endif(NOT HAVE_STRLCAT)

if(NOT HAVE_STRLCPY)
    set(LIBTCUTIL_SOURCES ${LIBTCUTIL_SOURCES} libtcutil/strlcpy.c)
endif(NOT HAVE_STRLCPY)

add_definitions(-DHAVE_CONFIG_H) # always
include_directories(${tcrund_SOURCE_DIR})
include_directories(${tcrund_SOURCE_DIR}/tclibs)
include_directories(${tcrund_SOURCE_DIR}/tclibs/libtcutil)

add_library(tcutil STATIC ${LIBTCUTIL_SOURCES})

set_target_properties(tcutil PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")

