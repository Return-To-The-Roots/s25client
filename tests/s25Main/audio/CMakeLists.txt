# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Tests using the audio system

add_testcase(NAME sounds
    LIBS s25Main audioMockup testHelpers testConfig rttr::vld
)
