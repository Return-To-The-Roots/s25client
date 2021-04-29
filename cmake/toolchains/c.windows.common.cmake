# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(CMAKE_SYSTEM_NAME Windows)
# Default to Win10 if not set on command line
if(NOT CMAKE_SYSTEM_VERSION)
    set(CMAKE_SYSTEM_VERSION 10.0.0)
endif()

# Search for a matching MinGW compiler preferring the more recent w64 infix
foreach(infix w64 pc)
    set(COMPILER_PREFIX ${CMAKE_SYSTEM_PROCESSOR}-${infix}-mingw32)
    foreach(var CMAKE_C_COMPILER CMAKE_CXX_COMPILER CMAKE_RC_COMPILER)
        unset(${var} CACHE)
        unset(${var})
    endforeach()
    find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
    find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
    find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)
    if(CMAKE_C_COMPILER AND CMAKE_CXX_COMPILER AND CMAKE_RC_COMPILER)
        break()
    endif()
endforeach()

# set search prefix
set(CMAKE_FIND_ROOT_PATH /usr/${COMPILER_PREFIX})

# search for programs in the build host directories
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

list(APPEND CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES ${CMAKE_FIND_ROOT_PATH}/include)
list(APPEND CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES ${CMAKE_FIND_ROOT_PATH}/include)
