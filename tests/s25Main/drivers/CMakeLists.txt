# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Testing driver system
add_testcase(NAME drivers
    LIBS s25Main testHelpers rttr::vld
)

add_dependencies(Test_drivers drivers)
