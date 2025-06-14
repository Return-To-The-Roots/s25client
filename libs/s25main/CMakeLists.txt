# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(BZip2 1.0.6 REQUIRED)
gather_dll(BZIP2)

set(SOURCES_SUBDIRS )
macro(AddDirectory dir)
    file(GLOB SUB_FILES ${dir}/*.cpp ${dir}/*.h ${dir}/*.hpp ${dir}/*.tpp)
    set(SOURCES_SUBDIRS ${SOURCES_SUBDIRS} ${SUB_FILES})
    source_group(${dir} FILES ${SUB_FILES})
endmacro()

AddDirectory(addons)
AddDirectory(ai)
AddDirectory(ai/aijh)
AddDirectory(animation)
AddDirectory(buildings)
AddDirectory(controls)
AddDirectory(desktops)
AddDirectory(drivers)
AddDirectory(factories)
AddDirectory(figures)
AddDirectory(gameData)
AddDirectory(gameTypes)
AddDirectory(helpers)
AddDirectory(ingameWindows)
AddDirectory(lua)
AddDirectory(mapGenerator)
AddDirectory(network)
AddDirectory(nodeObjs)
AddDirectory(notifications)
AddDirectory(ogl)
AddDirectory(pathfinding)
AddDirectory(postSystem)
AddDirectory(random)
AddDirectory(resources)
AddDirectory(world)
file(GLOB SOURCES_OTHER *.cpp *.h)
source_group(other FILES ${SOURCES_OTHER})

set(s25Main_SRCS
    ${SOURCES_OTHER}
    ${SOURCES_SUBDIRS}
)

add_library(s25Main STATIC ${s25Main_SRCS})
target_include_directories(s25Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(s25Main PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(s25Main PUBLIC cxx_std_17)

target_link_libraries(s25Main PUBLIC
    siedler2
    lobby_c
    s25util::common
    s25util::network
    mygettext
    s25Common
    rttrConfig
    gamedata
    driver
    Boost::filesystem Boost::disable_autolinking
    PRIVATE BZip2::BZip2 Boost::iostreams Boost::locale Boost::nowide samplerate_cpp
)
if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    target_link_libraries(s25Main PUBLIC videoSDL2 audioSDL)
    set(USE_FLAGS "--use-port=sdl2 --use-port=sdl2_image \
        --use-port=sdl2_mixer -sSDL2_MIXER_FORMATS=ogg \
        -I/src/contrib/gl4es/include")
    set_target_properties(s25Main PROPERTIES
            COMPILE_FLAGS "${USE_FLAGS}"
            LINK_FLAGS "${USE_FLAGS} -sFULL_ES2 -L/src/contrib/gl4es/lib/ -lgl4es"
    )
    add_definitions(-DRTTR_OGL_ES=0)
else ()
    target_link_libraries(s25Main PUBLIC glad)
endif ()

if(WIN32 OR CYGWIN)
    include(CheckIncludeFiles)
    check_include_files("windows.h;dbghelp.h" RTTR_BACKTRACE_HAS_DBGHELP)
else()
    set(RTTR_BACKTRACE_HAS_DBGHELP OFF)
endif()
if(NOT WIN32)
    find_package(Backtrace)
    if(Backtrace_FOUND)
        target_include_directories(s25Main SYSTEM PRIVATE ${Backtrace_INCLUDE_DIRS})
        target_link_libraries(s25Main PRIVATE ${Backtrace_LIBRARIES})
    endif()
    set(RTTR_BACKTRACE_HAS_FUNCTION ${Backtrace_FOUND})
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/backtrace_config.h.cmake backtrace_config.h @ONLY)
target_include_directories(s25Main PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# For clock_gettime etc. this is required on some platforms/compilers
find_library(LIBRT rt)
if(LIBRT)
    target_link_libraries(s25Main PUBLIC ${LIBRT})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(s25Main PUBLIC ${CMAKE_DL_LIBS}) # For dynamic driver loading (DriverWrapper)
endif()

include(EnableWarnings)
enable_warnings(s25Main)
