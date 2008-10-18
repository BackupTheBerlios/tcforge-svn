
set(LIBTC_SOURCES cfgfile.c framecode.c getopt.c getopt1.c iodir.c optstr.c
                  ratiocodes.c strlcat.c strlcpy.c tc_functions.c tcglob.c)

add_definitions(-DHAVE_CONFIG_H)
include_directories(${tcrund_SOURCE_DIR})
include_directories(${tcrund_SOURCE_DIR}/tcstubs)
include_directories(${tcrund_SOURCE_DIR}/tcstubs/src)
include_directories(${tcrund_SOURCE_DIR}/libtc)
add_library(tc STATIC ${LIBTC_SOURCES})

