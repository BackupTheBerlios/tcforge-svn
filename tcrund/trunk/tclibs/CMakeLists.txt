
set(LIBTCUTIL_SOURCES 
    libtcutil/cfgfile.c libtcutil/tcglob.c libtcutil/iodir.c
    libtcutil/ioutils.c libtcutil/tclist.c libtcutil/logging.c
    libtcutil/memutils.c libtcutil/optstr.c libtcutil/strlcat.c
    libtcutil/strlcpy.c libtcutil/strutils.c)

add_definitions(-DHAVE_CONFIG_H)
include_directories(${tcrund_SOURCE_DIR})
include_directories(${tcrund_SOURCE_DIR}/tclibs)
include_directories(${tcrund_SOURCE_DIR}/tclibs/libtcutil)

add_library(tcutil STATIC ${LIBTCUTIL_SOURCES})

set_target_properties(tcutil PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")

