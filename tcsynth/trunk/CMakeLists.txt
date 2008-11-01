project(tcsynth)
cmake_minimum_required(VERSION 2.6)

include(CheckIncludeFile)
include(CheckCSourceCompiles)

CHECK_INCLUDE_FILE(dlfcn.h HAVE_DLFCN_H)



if(CMAKE_COMPILER_IS_GNUCC)
    set(HAVE_GCC 1)
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

configure_file(config.h.in config.h)

add_subdirectory(tclibs)
add_subdirectory(tcsynth)

