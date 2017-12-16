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

#include "rttrDefines.h" // IWYU pragma: keep
#include "TerrainDesc.h"
#include "DescriptionHelpers.h"
#include "WorldDescription.h"
#include "helpers/converters.h"
#include "lua/CheckedLuaTable.h"
#include "ogl/glSmartBitmap.h"

using namespace descriptionHelpers;

TerrainKind strToTerrainKind(const std::string& name)
{
    if(name == "land")
        return TerrainKind::LAND;
    else if(name == "water")
        return TerrainKind::WATER;
    else if(name == "lava")
        return TerrainKind::LAVA;
    else if(name == "snow")
        return TerrainKind::SNOW;
    else if(name == "mountain")
        return TerrainKind::MOUNTAIN;
    else
        throw GameDataLoadError("Invalid terrain kind: " + name);
}

TerrainBQ getDefaultBQ(TerrainKind kind)
{
    switch(boost::native_value(kind))
    {
        case TerrainKind::LAND: return TerrainBQ::CASTLE;
        case TerrainKind::WATER: return TerrainBQ::NOTHING;
        case TerrainKind::LAVA:
        case TerrainKind::SNOW: return TerrainBQ::DANGER;
        case TerrainKind::MOUNTAIN: return TerrainBQ::MINE;
    }
    throw GameDataError("Invalid terrain kind: " + helpers::toString(boost::underlying_cast<unsigned>(kind)));
}

ETerrain getDefaultFlags(TerrainKind kind)
{
    switch(boost::native_value(kind))
    {
        case TerrainKind::LAND: return ETerrain::Buildable;
        case TerrainKind::WATER: return ETerrain::Shippable;
        case TerrainKind::LAVA:
        case TerrainKind::SNOW: return ETerrain::Unreachable;
        case TerrainKind::MOUNTAIN: return ETerrain::Mineable;
    }
    throw GameDataError("Invalid terrain kind: " + helpers::toString(boost::underlying_cast<unsigned>(kind)));
}

uint8_t getDefaultHumidity(TerrainKind kind)
{
    switch(boost::native_value(kind))
    {
        case TerrainKind::LAND:
        case TerrainKind::WATER: return 100;
        case TerrainKind::LAVA:
        case TerrainKind::SNOW:
        case TerrainKind::MOUNTAIN: return 0;
    }
    throw GameDataError("Invalid terrain kind: " + helpers::toString(boost::underlying_cast<unsigned>(kind)));
}

TerrainDesc::TerrainDesc(CheckedLuaTable luaData, const WorldDescription& worldDesc)
{
    luaData.getOrThrow(name, "name");
    landscape = strToLandscape(luaData.getOrThrow<std::string>("landscape"));
    s2Id = luaData.getOrDefault<uint8_t>("s2Id", 0xFF);
    std::string edgeTypeName = luaData.getOrThrow<std::string>("edgeType");
    if(edgeTypeName != "none" && !worldDesc.edges.getIndex(edgeTypeName))
        throw GameDataLoadError("Invalid edge type: " + edgeTypeName);
    else
        edgeType = worldDesc.edges.getIndex(edgeTypeName);
    edgePriority = luaData.getOrDefault<int8_t>("edgePriority", 0);
    kind = strToTerrainKind(luaData.getOrDefault<std::string>("kind", "land"));
    std::string property = luaData.getOrDefault<std::string>("property", "");
    if(property.empty())
        flags = getDefaultFlags(kind);
    else if(property == "buildable")
        flags = ETerrain::Buildable;
    else if(property == "mineable")
        flags = ETerrain::Mineable;
    else if(property == "walkable")
        flags = ETerrain::Walkable;
    else if(property == "shippable")
        flags = ETerrain::Shippable;
    else if(property == "unwalkable")
        flags = ETerrain::Unwalkable;
    else if(property == "unreachable")
        flags = ETerrain::Unreachable;
    else
        throw GameDataLoadError("Invalid property '" + property + "'");
    humidity = luaData.getOrDefault("humidity", getDefaultHumidity(kind));
    luaData.getOrThrow(texturePath, "texture");
    posInTexture = luaData.getRectOrDefault("pos", Rect());
    numFrames = luaData.getOrDefault("numFrames", 1u);
    if(numFrames == 0u)
        throw GameDataLoadError("Cannot have a texture with no frames!");
    palAnimIdx = luaData.getOrDefault("palAnimIdx", 0u);
    minimapColor = luaData.getOrThrow<unsigned>("color");
}

TerrainDesc::~TerrainDesc() {}

TerrainBQ TerrainDesc::GetBQ() const
{
    if(Is(ETerrain::Buildable))
        return TerrainBQ::CASTLE;
    else if(Is(ETerrain::Mineable))
        return TerrainBQ::MINE;
    else if(Is(ETerrain::Walkable))
        return TerrainBQ::FLAG;
    else if(Is(ETerrain::Unreachable))
        return TerrainBQ::DANGER;
    else
        return TerrainBQ::NOTHING;
}

bool TerrainDesc::IsUsableByAnimals() const
{
    // If it is buildable land or mountain, animals can use it
    return (kind == TerrainKind::LAND || kind == TerrainKind::MOUNTAIN) && Is(ETerrain::Buildable);
}

bool TerrainDesc::IsVital() const
{
    return kind == TerrainKind::LAND && Is(ETerrain::Buildable);
}
