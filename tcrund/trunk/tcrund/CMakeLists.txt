
add_executable(tcrund tcrund.c logging_file.c process.c server.c tcrauth.c)

find_package(XMLRPC)
if(XMLRPC_FOUND)
    include_directories(${XMLRPC_INCLUDE_DIR})
    target_link_libraries(tcrund ${XMLRPC_LIBRARIES})
    # why is this not included into the above list?
    target_link_libraries(tcrund xmlrpc_server)
else(XMLRPC_FOUND)
    message(FATAL_ERROR, "libxmlrpc-c not found (get it at http://xmlrpc-c.sf.net)")
endif(XMLRPC_FOUND)

find_package(EVENT)
if(EVENT_FOUND)
    include_directories(${EVENT_INCLUDE_DIR})
    target_link_libraries(tcrund ${EVENT_LIBRARY})
else(EVENT_FOUND)
    message(FATAL_ERROR, "libevent not found (get it at http://www.monkey.org/~provos/libevent/)")
endif(EVENT_FOUND)


include_directories(${tcrund_SOURCE_DIR})
include_directories(${tcrund_SOURCE_DIR}/tclibs)
include_directories(${tcrund_SOURCE_DIR}/tclibs/libtcutil)
include_directories(${tcrund_SOURCE_DIR}/tcrund)

link_directories(${tcrund_BINARY_DIR}/tclibs)
target_link_libraries(tcrund tcutil)

set_target_properties(tcrund PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")


