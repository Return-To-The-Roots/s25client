# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

include(RttrTestingCfg)
option(RTTR_EXTERNAL_BUILD_TESTING "Enable tests of external dependencies" OFF)
set(BUILD_TESTING ${RTTR_EXTERNAL_BUILD_TESTING})

if(NOT RTTR_USE_SYSTEM_LIBS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/dev-tools/CMakeLists.txt)
    set(includeDevToolsDefault ON)
else()
    set(includeDevToolsDefault OFF)
endif()

option(RTTR_INCLUDE_DEVTOOLS "Include folder with precompiled binaries for development" ${includeDevToolsDefault})
if(RTTR_INCLUDE_DEVTOOLS)
    add_subdirectory(dev-tools)
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
    set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} PARENT_SCOPE)
    set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} PARENT_SCOPE)
endif()

# Building the updater when using non-default paths (especially absolute paths)
# is not recommended as it most likely won't work
set(RTTR_BUILD_UPDATER_DEF ON)
foreach(curDirVar BINDIR DATADIR LIBDIR DOCDIR DRIVERDIR)
    if(IS_ABSOLUTE ${RTTR_${curDirVar}})
        set(RTTR_BUILD_UPDATER_DEF OFF)
    endif()
endforeach()
option(RTTR_BUILD_UPDATER "Build auto-updater. Not advised when changing the default paths as the updater is configured specifically for the official builds." ${RTTR_BUILD_UPDATER_DEF})

add_subdirectory(glad)
add_library(kaguya INTERFACE)
target_include_directories(kaguya SYSTEM INTERFACE kaguya/include)
add_subdirectory(libendian)
set(RTTR_BUILD_LOBBY_CLIENT ON)
add_subdirectory(liblobby)

add_subdirectory(libsiedler2)
add_subdirectory(libutil)
add_subdirectory(mygettext)
add_subdirectory(s25edit)
if(RTTR_BUILD_UPDATER)
    add_subdirectory(s25update)
endif()

option(RTTR_USE_SYSTEM_LIBSAMPLERATE "Use system installed libsamplerate. Fails if not found!" "${RTTR_USE_SYSTEM_LIBS}")
if(RTTR_USE_SYSTEM_LIBSAMPLERATE)
    find_package(Samplerate 0.1.9 REQUIRED)
    unset(samplerate_LIB_DEPENDS CACHE)
    add_library(samplerate INTERFACE)
    target_link_libraries(samplerate INTERFACE Samplerate::Samplerate)
else()
    include(FetchContent)
    FetchContent_Declare(
        LibSamplerate
        GIT_REPOSITORY https://github.com/libsndfile/libsamplerate
        GIT_TAG        0.2.2
    )
    set(LIBSAMPLERATE_EXAMPLES OFF CACHE INTERNAL "")
    set(LIBSAMPLERATE_INSTALL OFF CACHE INTERNAL "")
    set(old_BUILD_TESTING ${BUILD_TESTING})
    set(BUILD_TESTING OFF)
    FetchContent_MakeAvailable(LibSamplerate)
    set(BUILD_TESTING ${old_BUILD_TESTING})
endif()

# No tests for turtle
set(BUILD_TESTING OFF)
add_subdirectory(turtle)
get_target_property(inc turtle INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(turtle PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc}")
