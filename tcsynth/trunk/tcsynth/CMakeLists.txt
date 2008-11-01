
add_executable(tcsynth tcsynth.c)

find_package(Lua51)
if(LUA_FOUND)
    include_directories(${LUA_INCLUDE_DIR})
    target_link_libraries(tcsynth ${LUA_LIBRARIES})
else(LUA_FOUND)
    message(FATAL_ERROR, "lua 5.1 not found (get it at http://www.lua.org)")
endif(LUA_FOUND)

include_directories(${tcsynth_SOURCE_DIR})
include_directories(${tcsynth_SOURCE_DIR}/tclibs)
include_directories(${tcsynth_SOURCE_DIR}/tclibs/libtcutil)
include_directories(${tcsynth_SOURCE_DIR}/tcsynth)

link_directories(${tcsynth_BINARY_DIR}/tclibs)
target_link_libraries(tcsynth tcutil)

set_target_properties(tcsynth PROPERTIES COMPILE_FLAGS "-g -Os -Wall -Werror")


