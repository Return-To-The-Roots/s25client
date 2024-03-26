# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(ai-battle main.cpp HeadlessGame.cpp)
target_link_libraries(ai-battle PRIVATE s25Main Boost::program_options Boost::nowide)
