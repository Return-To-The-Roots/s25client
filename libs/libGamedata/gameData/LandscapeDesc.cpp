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
