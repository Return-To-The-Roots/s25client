// Copyright (c) 2017 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#ifndef LuaTraits_h__
#define LuaTraits_h__

#include "SafeEnum.h"
#include <kaguya/kaguya.hpp>
#include <utility>

namespace kaguya {

template<typename T1, typename T2>
struct lua_type_traits<std::pair<T1, T2>>
{
    using get_type = std::pair<T1, T2>;
    using push_type = const std::pair<T1, T2>&;

    struct checkTypeForEach
    {
        checkTypeForEach(bool& valid) : valid_(valid) {}
        bool& valid_;
        bool operator()(const LuaStackRef& k, const LuaStackRef& v)
        {
            if(k.isConvertible<size_t>())
            {
                size_t idx = k;
                if(idx == 1)
                    valid_ = v.isConvertible<T1>();
                else if(idx == 2)
                    valid_ = v.isConvertible<T2>();
                else
                    valid_ = false;
            } else
                valid_ = false;
            return valid_;
        }
    };
    struct strictCheckTypeForEach
    {
        strictCheckTypeForEach(bool& valid) : valid_(valid) {}
        bool& valid_;
        bool operator()(const LuaStackRef& k, const LuaStackRef& v)
        {
            if(k.isType<size_t>())
            {
                size_t idx = k;
                if(idx == 1)
                    valid_ = v.isType<T1>();
                else if(idx == 2)
                    valid_ = v.isType<T2>();
                else
                    valid_ = false;
            } else
                valid_ = false;
            return valid_;
        }
    };

    static bool checkType(lua_State* l, int index)
    {
        const LuaStackRef table(l, index);
        if(table.type() != LUA_TTABLE || table.size() != 2)
            return false;
        bool valid = true;
        table.foreach_table_breakable<LuaStackRef, LuaStackRef>(checkTypeForEach(valid));
        return valid;
    }
    static bool strictCheckType(lua_State* l, int index)
    {
        const LuaStackRef table(l, index);
        if(table.type() != LUA_TTABLE || table.size() != 2)
            return false;
        bool valid = true;
        table.foreach_table_breakable<LuaStackRef, LuaStackRef>(strictCheckTypeForEach(valid));
        return valid;
    }
    static get_type get(lua_State* l, int index)
    {
        const LuaStackRef table(l, index);
        if(table.type() != LUA_TTABLE || table.size() != 2)
            throw LuaTypeMismatch();
        return get_type(table[1], table[2]);
    }
    static int push(lua_State* l, push_type v)
    {
        lua_createtable(l, 2, 0);
        LuaStackRef table(l, -1);
        table.setRawField(1, v.first);
        table.setRawField(2, v.second);
        return 1;
    }
};

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

#endif // LuaTraits_h__
