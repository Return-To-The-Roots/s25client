// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef CheckedLuaTable_h__
#define CheckedLuaTable_h__

#include "Rect.h"
#include "gameData/WorldDescription.h"
#include <kaguya/kaguya.hpp>
#include <set>
#include <string>
#include <vector>

/// Lua table wrapper that provides checked access to its keys and reports unused keys on destruction
class CheckedLuaTable
{
    kaguya::LuaTable table;
    std::set<std::string> accessedKeys_;
    bool checked;

public:
    CheckedLuaTable(const kaguya::LuaTable& luaTable);
    ~CheckedLuaTable();
    /// Check and report unused entries
    bool checkUnused(bool throwError = true);

    /// Return the value from lua or throw an error
    template<typename T>
    T getOrThrow(const std::string& fieldName);
    template<typename T>
    void getOrThrow(T& outVal, const std::string& fieldName);
    /// Return the value or use the default if it doesn't exist
    template<typename T>
    T getOrDefault(const std::string& fieldName, const T& defaultValue);
    /// Get a Rect from lua or return the default. TODO: Add kaguya conversion function and remove this
    Rect getRectOrDefault(const std::string& fieldName, const Rect& defaultValue);
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

template<typename T>
inline T CheckedLuaTable::getOrThrow(const std::string& fieldName)
{
    accessedKeys_.insert(fieldName);
    kaguya::LuaRef value = table[fieldName];
    if(value.isNilref())
        throw GameDataLoadError("Required field '" + fieldName + "' not found");
    if(value.isConvertible<T>())
        return value;
    else
        throw GameDataLoadError("Field '" + fieldName + "' has the wrong type");
}

template<typename T>
inline void CheckedLuaTable::getOrThrow(T& outVal, const std::string& fieldName)
{
    outVal = getOrThrow<T>(fieldName);
}

template<typename T>
inline T CheckedLuaTable::getOrDefault(const std::string& fieldName, const T& defaultValue)
{
    accessedKeys_.insert(fieldName);
    kaguya::LuaRef value = table[fieldName];
    if(value.isNilref())
        return defaultValue;
    if(value.isConvertible<T>())
        return value;
    else
        throw GameDataLoadError("Field '" + fieldName + "' has the wrong type");
}

inline Rect CheckedLuaTable::getRectOrDefault(const std::string& fieldName, const Rect& defaultValue)
{
    std::vector<unsigned> luaRect = getOrDefault(fieldName, std::vector<unsigned>());
    if(!luaRect.empty())
    {
        if(luaRect.size() != 4u)
            throw GameDataLoadError("You need 4 values for attribute '" + fieldName + "'");
        return Rect(luaRect[0], luaRect[1], luaRect[2], luaRect[3]);
    } else
        return defaultValue;
}

#endif // CheckedLuaTable_h__
