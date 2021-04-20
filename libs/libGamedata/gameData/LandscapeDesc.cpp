// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LandscapeDesc.h"
#include "helpers/EnumRange.h"
#include "lua/CheckedLuaTable.h"
#include "lua/LuaHelpers.h"

LandscapeDesc::LandscapeDesc(CheckedLuaTable luaData, const WorldDescription&)
{
    static const helpers::EnumArray<std::string, LandRoadType> roadTypeNames = {
      {"normal", "upgraded", "boat", "mountain"}};
    luaData.getOrThrow(name, "name");
    luaData.getOrThrow(mapGfxPath, "mapGfx");
    lua::validatePath(mapGfxPath);
    s2Id = luaData.getOrDefault<uint8_t>("s2Id", 0xFF);
    isWinter = luaData.getOrDefault("isWinter", false);

    CheckedLuaTable roadData = luaData.getOrThrow<CheckedLuaTable>("roads");
    for(const auto i : helpers::enumRange<LandRoadType>())
    {
        CheckedLuaTable texData = roadData.getOrThrow<CheckedLuaTable>(roadTypeNames[i]);
        texData.getOrThrow(roadTexDesc[i].texturePath, "texture");
        lua::validatePath(roadTexDesc[i].texturePath);
        roadTexDesc[i].posInTexture = texData.getRectOrDefault("pos", Rect());
        texData.checkUnused();
    }
    roadData.checkUnused();
    luaData.checkUnused();
}
