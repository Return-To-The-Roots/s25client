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

#include "commonDefines.h" // IWYU pragma: keep
#include "TerrainDesc.h"
#include "WorldDescription.h"
#include "helpers/converters.h"
#include "lua/CheckedLuaTable.h"
#include "lua/LuaHelpers.h"

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
        throw GameDataError("Invalid terrain kind: " + name);
}

ETexType strToTexType(const std::string& name)
{
    if(name == "overlapped")
        return ETexType::Overlapped;
    else if(name == "stacked")
        return ETexType::Stacked;
    else if(name == "rotated")
        return ETexType::Rotated;
    else
        throw GameDataError("Invalid texture type: " + name);
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
    landscape = worldDesc.landscapes.getIndex(luaData.getOrThrow<std::string>("landscape"));
    if(!landscape)
        throw GameDataError("Invalid landscape type: " + luaData.getOrThrow<std::string>("landscape"));
    s2Id = luaData.getOrDefault<uint8_t>("s2Id", 0xFF);
    std::string edgeTypeName = luaData.getOrThrow<std::string>("edgeType");
    if(edgeTypeName != "none" && !worldDesc.edges.getIndex(edgeTypeName))
        throw GameDataError("Invalid edge type: " + edgeTypeName);
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
        throw GameDataError("Invalid property '" + property + "'");
    humidity = luaData.getOrDefault("humidity", getDefaultHumidity(kind));
    luaData.getOrThrow(texturePath, "texture");
    lua::validatePath(texturePath);
    posInTexture = luaData.getRectOrDefault("pos", Rect());
    texType = strToTexType(luaData.getOrDefault<std::string>("texType", "overlapped"));
    palAnimIdx = luaData.getOrDefault<int8_t>("palAnimIdx", -1);
    luaData.getOrThrow(minimapColor, "color");

    luaData.checkUnused();
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

TerrainDesc::Triangle TerrainDesc::GetUSDTriangle() const
{
    Triangle result;
    PointF middleBottom((posInTexture.left + posInTexture.right) / 2.f, posInTexture.bottom + 0.f);
    PointF leftMiddle;
    if(texType == ETexType::Stacked || texType == ETexType::Rotated)
    {
        // Integer rounding up
        leftMiddle = PointF(Position(posInTexture.left, (posInTexture.top + posInTexture.bottom + 1) / 2));
        // When cutting the 2 triangles from a square, we want a 45 deg angle (height = 0.5*width) to avoid using outside pixels
        // So if we can make those equal by removing half a pixel to the bottom middle position, we do this
        if(middleBottom.x - leftMiddle.x == middleBottom.y - 0.5f - leftMiddle.y)
            middleBottom.y -= 0.5f;
    }
    switch(boost::native_value(texType))
    {
        case ETexType::Overlapped:
        default:
            result.tip = middleBottom;
            result.left = PointF(posInTexture.getOrigin());
            result.right = PointF(Position(posInTexture.right, posInTexture.top));
            break;
        case ETexType::Stacked:
            result.tip = middleBottom;
            result.left = leftMiddle;
            result.right = PointF(posInTexture.right + 0.f, result.left.y);
            if(result.tip.x - result.left.x == result.tip.y - 0.5f - result.left.y)
                result.tip.y -= 0.5f;
            break;
        case ETexType::Rotated:
            result.tip = PointF(posInTexture.right + 0.f, leftMiddle.y);
            result.left = middleBottom;
            result.right = leftMiddle;
            break;
    }
    return result;
}

TerrainDesc::Triangle TerrainDesc::GetRSUTriangle() const
{
    Triangle result;
    PointF middleTop((posInTexture.left + posInTexture.right) / 2.f, posInTexture.top + 0.f);
    PointF leftMiddle;
    if(texType == ETexType::Stacked || texType == ETexType::Rotated)
    {
        // Integer rounding down
        leftMiddle = PointF(Position(posInTexture.left, (posInTexture.top + posInTexture.bottom) / 2));
        // When cutting the 2 triangles from a square, we want a 45 deg angle (height = 0.5*width) to avoid using outside pixels
        // So if we can make those equal by adding half a pixel to the top middle position, we do this
        if(middleTop.x - leftMiddle.x == leftMiddle.y - (middleTop.y + 0.5f))
            middleTop.y += 0.5f;
    }
    switch(boost::native_value(texType))
    {
        case ETexType::Overlapped:
        default:
            result.tip = middleTop;
            result.left = PointF(posInTexture.left + 0.5f, posInTexture.bottom + 0.f);
            result.right = PointF(posInTexture.right - 0.5f, posInTexture.bottom + 0.f);
            break;
        case ETexType::Stacked:
            result.tip = middleTop;
            result.left = leftMiddle;
            result.right = PointF(posInTexture.right + 0.f, result.left.y);
            if(result.tip.x - result.left.x == result.left.y - result.tip.y - 0.5f)
                result.tip.y += 0.5f;
            break;
        case ETexType::Rotated:
            result.tip = leftMiddle;
            result.left = PointF(posInTexture.right + 0.f, result.tip.y);
            result.right = middleTop;
            break;
    }
    return result;
}
