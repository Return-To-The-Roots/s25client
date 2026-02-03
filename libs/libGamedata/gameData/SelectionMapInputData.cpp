// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SelectionMapInputData.h"
#include "lua/CheckedLuaTable.h"

namespace kaguya {
template<>
struct lua_type_traits<MissionSelectionInfo>
{
    using get_type = MissionSelectionInfo;
    using push_type = const MissionSelectionInfo&;

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
                    valid_ = v.isConvertible<unsigned>();
                else if(idx == 2 || idx == 3)
                    valid_ = v.isConvertible<int>();
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
                    valid_ = v.isType<unsigned>();
                else if(idx == 2 || idx == 3)
                    valid_ = v.isType<int>();
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
        if(table.type() != LUA_TTABLE || table.size() != 3)
            return false;
        bool valid = true;
        table.foreach_table_breakable<LuaStackRef, LuaStackRef>(checkTypeForEach(valid));
        return valid;
    }
    static bool strictCheckType(lua_State* l, int index)
    {
        const LuaStackRef table(l, index);
        if(table.type() != LUA_TTABLE || table.size() != 3)
            return false;
        bool valid = true;
        table.foreach_table_breakable<LuaStackRef, LuaStackRef>(strictCheckTypeForEach(valid));
        return valid;
    }
    static get_type get(lua_State* l, int index)
    {
        const LuaStackRef table(l, index);
        if(table.type() != LUA_TTABLE || table.size() != 3)
            throw LuaTypeMismatch();
        return get_type{table[1], Position(table[2], table[3])};
    }
    static int push(lua_State* l, push_type v)
    {
        return util::push_args(l, v.maskAreaColor, v.ankerPos.x, v.ankerPos.y);
    }
};

template<>
struct lua_type_traits<Position>
{
    using get_type = Position;
    using push_type = const Position&;

    struct checkTypeForEach
    {
        checkTypeForEach(bool& valid) : valid_(valid) {}
        bool& valid_;
        bool operator()(const LuaStackRef& k, const LuaStackRef& v)
        {
            if(k.isConvertible<size_t>())
            {
                size_t idx = k;
                if(idx == 1 || idx == 2)
                    valid_ = v.isConvertible<int>();
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
                if(idx == 1 || idx == 2)
                    valid_ = v.isType<int>();
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
    static int push(lua_State* l, push_type v) { return util::push_args(l, v.x, v.y); }
};

template<>
struct lua_type_traits<ImageResource>
{
    using get_type = ImageResource;
    using push_type = const ImageResource&;

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
                    valid_ = v.isConvertible<std::string>();
                else if(idx == 2)
                    valid_ = v.isConvertible<unsigned>();
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
                    valid_ = v.isType<std::string>();
                else if(idx == 2)
                    valid_ = v.isType<unsigned>();
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
    static int push(lua_State* l, push_type v) { return util::push_args(l, v.filePath, v.index); }
};
} // namespace kaguya

SelectionMapInputData::SelectionMapInputData(const kaguya::LuaRef& table)
{
    CheckedLuaTable luaMapSelection(table);
    luaMapSelection.getOrThrow(background, "background");
    luaMapSelection.getOrThrow(map, "map");
    luaMapSelection.getOrThrow(missionMapMask, "missionMapMask");
    luaMapSelection.getOrThrow(marker, "marker");
    luaMapSelection.getOrThrow(conquered, "conquered");
    luaMapSelection.getOrThrow(mapOffsetInBackground, "backgroundOffset");
    luaMapSelection.getOrThrow(disabledColor, "disabledColor");
    luaMapSelection.getOrThrow(missionSelectionInfos, "missionSelectionInfos");
    luaMapSelection.checkUnused();
}
