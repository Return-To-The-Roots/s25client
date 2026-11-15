# Copyright (C) 2005 - 2026 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(ai-battle main.cpp HeadlessGame.cpp)
target_link_libraries(ai-battle PRIVATE s25Main headlessConsole Boost::program_options Boost::nowide)

if(WIN32)
    include(GatherDll)
    gather_dll_copy(ai-battle)
endif()
