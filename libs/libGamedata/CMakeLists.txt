# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(SOURCES_SUBDIRS )
macro(AddDirectory dir)
    file(GLOB SUB_FILES ${dir}/*.cpp ${dir}/*.h ${dir}/*.hpp ${dir}/*.tpp)
    set(SOURCES_SUBDIRS ${SOURCES_SUBDIRS} ${SUB_FILES})
    source_group(${dir} FILES ${SUB_FILES})
endmacro()

AddDirectory(gameData)
AddDirectory(lua)

file(GLOB SOURCES_OTHER *.cpp *.h)
source_group(other FILES ${SOURCES_OTHER})

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(LUA_LIBRARIES /src/contrib/lua-5.4.7/build/lib/liblua.a)
    set(LUA_INCLUDE_DIR /src/contrib/lua-5.4.7/build/include)
else ()
    find_package(Lua 5.1 REQUIRED)
endif ()
include(GatherDll)
gather_dll(Lua)

add_library(gamedata STATIC ${SOURCES_OTHER} ${SOURCES_SUBDIRS})
target_include_directories(gamedata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gamedata SYSTEM PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(gamedata PUBLIC
    s25util::common
    mygettext
    s25Common
    kaguya
    ${LUA_LIBRARIES} ${CMAKE_DL_LIBS}
    PRIVATE rttrConfig Boost::nowide
)
include(EnableWarnings)
enable_warnings(gamedata)
