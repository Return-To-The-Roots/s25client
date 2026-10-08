# Copyright (C) 2005 - 2026 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Console output shared by the headless tools (ai-battle, replay-player)
add_library(headlessConsole STATIC HeadlessConsole.cpp HeadlessConsole.h)
target_link_libraries(headlessConsole PUBLIC s25Main PRIVATE Boost::nowide)
target_include_directories(headlessConsole PUBLIC .)
