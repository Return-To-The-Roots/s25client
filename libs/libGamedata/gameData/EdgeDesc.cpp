// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "EdgeDesc.h"
#include "WorldDescription.h"
#include "lua/CheckedLuaTable.h"
#include "lua/LuaHelpers.h"

EdgeDesc::EdgeDesc(CheckedLuaTable luaData, const WorldDescription& worldDesc)
{
    luaData.getOrThrow(name, "name");
    landscape = worldDesc.landscapes.getIndex(luaData.getOrThrow<std::string>("landscape"));
    if(!landscape)
        throw GameDataError("Invalid landscape type: " + luaData.getOrThrow<std::string>("landscape"));
    luaData.getOrThrow(texturePath, "texture");
    lua::validatePath(texturePath);
    posInTexture = luaData.getRectOrDefault("pos", Rect());
    luaData.checkUnused();
}
