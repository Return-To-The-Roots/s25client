# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(testUIHelper STATIC
    uiHelper/uiHelpers.hpp
    uiHelper/uiHelpers.cpp
)
target_include_directories(testUIHelper PUBLIC .)
target_link_libraries(testUIHelper PUBLIC s25Main videoMockup testHelpers turtle)
enable_warnings(testUIHelper)
