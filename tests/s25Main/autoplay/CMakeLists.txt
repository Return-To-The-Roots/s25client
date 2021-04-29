# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Tests running a whole simulation
# Example: Replay testing to make sure nothing introduced unexpected changes
add_testcase(NAME autoplay
    LIBS s25Main testConfig testHelpers rttr::vld
    CONFIGURATIONS Release RelWithDebInfo # This is really slow so only run when code is optimized
    COST 100
)
