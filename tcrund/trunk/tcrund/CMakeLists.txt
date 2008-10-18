
add_executable(tcrund tcrund.c)

find_package(XMLRPC)
if(XMLRPC_FOUND)
    include_directories(${XMLRPC_INCLUDE_DIR})
    target_link_libraries(tcrund ${XMLRPC_LIBRARY})
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
include_directories(${tcrund_SOURCE_DIR}/tcstubs)
include_directories(${tcrund_SOURCE_DIR}/libtc)
include_directories(${tcrund_SOURCE_DIR}/tcrund)

link_directories(${tcrund_BINARY_DIR}/libtc)


target_link_libraries(tcrund tc)
target_link_libraries(tcrund xmlrpc)
target_link_libraries(tcrund xmlrpc_util)
target_link_libraries(tcrund xmlrpc_xmltok)
target_link_libraries(tcrund xmlrpc_xmlparse)

