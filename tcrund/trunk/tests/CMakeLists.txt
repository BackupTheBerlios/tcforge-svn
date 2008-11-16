
option(ENABLE_TEST "compile testsuite")

if(ENABLE_TEST)
    find_package(LIBCHECK)
    if(LIBCHECK_NOTFOUND)
        message(FATAL_ERROR, "libcheck not found (get it at http://check.sf.net) - can't compile testsuite")
    endif(LIBCHECK_NOTFOUND)

    include_directories(${LIBCHECK_INCLUDE_DIR})
    include_directories(${tcrund_SOURCE_DIR})
    include_directories(${tcrund_SOURCE_DIR}/tclibs)
    include_directories(${tcrund_SOURCE_DIR}/tclibs/libtcutil)
    include_directories(${tcrund_SOURCE_DIR}/tcrund)

    link_directories(${tcrund_BINARY_DIR}/tclibs)

    add_executable(check_tcrauth check_tcrauth.c ../tcrund/tcrauth.c)
    target_link_libraries(check_tcrauth ${LIBCHECK_LIBRARY} tcutil)
    set_target_properties(check_tcrauth PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")
endif(ENABLE_TEST)

