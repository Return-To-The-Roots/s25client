# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# This is a util script intended to be included by darwin toolchains
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR NOT CMAKE_SYSTEM_PROCESSOR)
    message(FATAL_ERROR "Cannot use this toolchain file directly. You have to set CMAKE_SYSTEM_* first!")
endif()

# specify the cross compiler
set(usedToolchain)
foreach(COMPILER_PREFIX i386-apple-darwin15 i686-apple-darwin10)
    foreach(var CMAKE_C_COMPILER CMAKE_CXX_COMPILER)
        unset(${var} CACHE)
        unset(${var})
    endforeach()
    find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-clang ${COMPILER_PREFIX}-gcc)
    find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-clang++-libc++ ${COMPILER_PREFIX}-clang++ ${COMPILER_PREFIX}-g++)
    if(CMAKE_C_COMPILER AND CMAKE_CXX_COMPILER)
        set(usedToolchain ${COMPILER_PREFIX})
        break()
    endif()
endforeach()

find_program(CMAKE_INSTALL_NAME_TOOL NAMES ${usedToolchain}-install_name_tool)

set(OSX_SDKS "/usr/lib/apple/SDKs/MacOSX10.11.sdk" "/usr/lib/apple/SDKs/MacOSX10.5.sdk" "/usr/lib/apple/SDKs/MacOSX10.4u.sdk")

# set SDK (use newest first)
unset(CMAKE_OSX_SYSROOT)
foreach(SDK IN LISTS OSX_SDKS)
    if(EXISTS ${SDK})
        set(CMAKE_OSX_SYSROOT ${SDK})
        break()
    endif()
endforeach()
if(NOT CMAKE_OSX_SYSROOT)
    message(FATAL_ERROR "No OSX SDK found!")
endif()
set(CMAKE_OSX_SYSROOT ${CMAKE_OSX_SYSROOT} CACHE PATH "Path to OSX SDK")
message(STATUS "Using OSX SDK at ${CMAKE_OSX_SYSROOT}")

set(CMAKE_OSX_DEPLOYMENT_TARGET "$ENV{MACOSX_DEPLOYMENT_TARGET}" CACHE STRING "Minimum OS X version to target for deployment")
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET AND CMAKE_OSX_SYSROOT MATCHES "OSX([0-9]+\\.[0-9]+)u?\\.sdk")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "${CMAKE_MATCH_1}" CACHE STRING "" FORCE)
endif()

set(ENV{MACOSX_DEPLOYMENT_TARGET} "${CMAKE_OSX_DEPLOYMENT_TARGET}")

if(NOT CMAKE_SYSTEM_VERSION)
    find_program(sw_vers_bin NAMES ${usedToolchain}-sw_vers sw_vers)
    if(NOT sw_vers_bin)
        message(FATAL_ERROR "Could not find sw_vers tool to determine OSX version")
    endif()

    execute_process(COMMAND ${sw_vers_bin} -productVersion
      OUTPUT_VARIABLE osx_version
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(osx_version MATCHES "^10\\.([0-9]+)")
        #  10.x == Mac OSX 10.6 (Snow Leopard)
        #  11.x == Mac OSX 10.7 (Lion)
        #  12.x == Mac OSX 10.8 (Mountain Lion)
        #  etc.
        math(EXPR majorVersion "4 + ${CMAKE_MATCH_1}")
        set(CMAKE_SYSTEM_VERSION "${majorVersion}.0.0")
    else()
        message(FATAL_ERROR "Could not parse SDK version: ${osx_version}")
    endif()
endif()

set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT})

# search for programs in the build host directories
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
