// Copyright (C) 2015 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/MaxEnumValue.h"
#include "lua/LuaHelpers.h"
#include <kaguya/type.hpp>

namespace lua {
/// Wrapper around an enum which checks the int passed on ctor to be in range of the enum
template<class T_Enum, unsigned maxValue = helpers::MaxEnumValue_v<T_Enum>>
struct SafeEnum
{
    static T_Enum cast(int value)
    {
        assertTrue(value >= 0 && static_cast<unsigned>(value) <= maxValue, "Enum value is out of range");
        return static_cast<T_Enum>(value);
    }
    T_Enum value_;
    SafeEnum(int value) : value_(cast(value)) {}
    constexpr operator T_Enum() const noexcept { return value_; };
};
} // namespace lua

namespace kaguya {
template<class T_Enum, unsigned maxValue>
struct lua_type_traits<::lua::SafeEnum<T_Enum, maxValue>> : lua_type_traits<luaInt>
{
    using get_type = ::lua::SafeEnum<T_Enum, maxValue>;
    using opt_type = optional<get_type>;

    using parent = lua_type_traits<luaInt>;
    static opt_type opt(lua_State* l, int index) noexcept
    {
        if(const auto t = parent::opt(l, index))
            return opt_type(static_cast<get_type>(*t));
        return {};
    }
    static get_type get(lua_State* l, int index) { return parent::get(l, index); }
};
} // namespace kaguya
