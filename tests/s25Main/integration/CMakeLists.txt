# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Tests testing more than single components
# e.g. creating a whole world
# Lua related tests are extra
add_testcase(NAME integration
    LIBS s25Main testHelpers testWorldFixtures testUIHelper rttr::vld
    COST 50
)
