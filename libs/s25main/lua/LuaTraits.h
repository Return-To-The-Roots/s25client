// Copyright (c) 2017 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <kaguya/lua_ref.hpp>
#include <kaguya/traits.hpp>
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
    static int push(lua_State* l, push_type v) { return util::push_args(l, v.first, v.second); }
};
} // namespace kaguya
