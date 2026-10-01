# Copyright (C) 2005 - 2026 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(replay-player main.cpp HeadlessReplay.cpp HeadlessReplay.h)
target_link_libraries(replay-player PRIVATE s25Main Boost::program_options Boost::nowide)

if(WIN32)
    include(GatherDll)
    gather_dll_copy(replay-player)
endif()
