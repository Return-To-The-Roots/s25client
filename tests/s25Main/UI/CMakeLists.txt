# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Tests using the video system

add_subdirectory(uiHelper)

add_testcase(NAME UI
    LIBS s25Main testUIHelper rttr::vld testWorldFixtures
    COST 45
)
