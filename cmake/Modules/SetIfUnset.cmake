# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Set the given variable to the default value if it is unset (empty)
# Note: Does nothing even if the variable is falsy (OFF, ...)
macro(set_if_unset variable default)
    if("${${variable}}" STREQUAL "")
        set(${variable} "${default}")
    endif()
endmacro()
