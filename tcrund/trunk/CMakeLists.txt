project(tcrund)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/")

include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckFunctionExists)

CHECK_INCLUDE_FILE(malloc.h  HAVE_MALLOC_H)
CHECK_INCLUDE_FILE(string.h  HAVE_STRING_H)
CHECK_INCLUDE_FILE(libintl.h HAVE_LIBINTL_H)

if(CMAKE_COMPILER_IS_GNUCC)
    CHECK_C_SOURCE_COMPILES(
     "#include <stdio.h>
      int foo(char *,...) __attribute__((format(printf,1,2)));
      __attribute__((unused)) static int bar;
      int main(void)
      {
          return 0;
      }"
    HAVE_GCC_ATTRIBUTES)
endif(CMAKE_COMPILER_IS_GNUCC)

set(CMAKE_REQUIRED_DEFINITIONS -D_ISOC99_SOURCE=1)
set(CMAKE_REQUIRED_INCLUDES stdlib.h)
CHECK_FUNCTION_EXISTS(strtof       HAVE_STRTOF)

set(CMAKE_REQUIRED_DEFINITIONS -D_BSD_SOURCE=1)
set(CMAKE_REQUIRED_INCLUDES unistd.h)
CHECK_FUNCTION_EXISTS(getpagesize  HAVE_GETPAGESIZE)

set(CMAKE_REQUIRED_INCLUDES sys/time.h)
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)

CHECK_FUNCTION_EXISTS(vsscanf      HAVE_VSSCANF)

CHECK_FUNCTION_EXISTS(strlcat      HAVE_STRLCAT)

CHECK_FUNCTION_EXISTS(strlcpy      HAVE_STRLCPY)

configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)

add_subdirectory(tclibs)
add_subdirectory(tcrund)

