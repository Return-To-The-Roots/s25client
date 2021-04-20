# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Tests using lua

add_testcase(NAME lua
    LIBS s25Main testHelpers testUIHelper testWorldFixtures rttr::vld
    COST 15
)
