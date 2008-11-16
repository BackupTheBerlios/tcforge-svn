
option(ENABLE_TEST "compile testsuite")

if(ENABLE_TEST)
	find_package(LIBCHECK)
    if(LIBCHECK_FOUND)
        include_directories(${LIBCHECK_INCLUDE_DIR})
    else(LIBCHECK_FOUND)
        message(FATAL_ERROR, "libcheck not found (get it at http://check.sf.net) - can't compile testsuite")
    endif(LIBCHECK_FOUND)

    include_directories(${tcrund_SOURCE_DIR})
    include_directories(${tcrund_SOURCE_DIR}/tclibs)
    include_directories(${tcrund_SOURCE_DIR}/tclibs/libtcutil)
    include_directories(${tcrund_SOURCE_DIR}/tcrund)

    link_directories(${tcrund_BINARY_DIR}/tclibs)

    target_link_libraries(tcrund tcutil)
    set_target_properties(tcrund PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")
endif(ENABLE_TEST)

