# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR universal)

set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "OSX-Architectures")

# arm64 needs at least 11.0, but keep a value given by the caller
if("$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "")
    set(ENV{MACOSX_DEPLOYMENT_TARGET} "11.0")
endif()

# set compilers...
include("${CMAKE_CURRENT_LIST_DIR}/c.apple.common.cmake")
