# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "OSX-Architectures")

# set compilers...
include("${CMAKE_CURRENT_LIST_DIR}/c.apple.common.cmake")
