// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>

namespace rttr {
template<typename T_Enum>
constexpr auto enum_cast(T_Enum val)
{
    return static_cast<std::underlying_type_t<T_Enum>>(val);
}
} // namespace rttr
