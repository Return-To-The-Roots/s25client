// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TerrainDesc.h"
#include "WorldDescription.h"
#include "helpers/toString.h"
#include "lua/CheckedLuaTable.h"
#include "lua/LuaHelpers.h"

namespace {
TerrainKind strToTerrainKind(const std::string& name)
{
    if(name == "land")
        return TerrainKind::Land;
    else if(name == "water")
        return TerrainKind::Water;
    else if(name == "lava")
        return TerrainKind::Lava;
    else if(name == "snow")
        return TerrainKind::Snow;
    else if(name == "mountain")
        return TerrainKind::Mountain;
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

ETerrain getDefaultFlags(TerrainKind kind)
{
    switch(kind)
    {
        case TerrainKind::Land: return ETerrain::Buildable;
        case TerrainKind::Water: return ETerrain::Shippable;
        case TerrainKind::Lava:
        case TerrainKind::Snow: return ETerrain::Unreachable;
        case TerrainKind::Mountain: return ETerrain::Mineable;
    }
    throw GameDataError("Invalid terrain kind: " + helpers::toString(kind));
}

uint8_t getDefaultHumidity(TerrainKind kind)
{
    switch(kind)
    {
        case TerrainKind::Land:
        case TerrainKind::Water: return 100;
        case TerrainKind::Lava:
        case TerrainKind::Snow:
        case TerrainKind::Mountain: return 0;
    }
    throw GameDataError("Invalid terrain kind: " + helpers::toString(kind));
}
} // namespace

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

TerrainDesc::~TerrainDesc() = default;

TerrainBQ TerrainDesc::GetBQ() const
{
    if(Is(ETerrain::Buildable))
        return TerrainBQ::Castle;
    else if(Is(ETerrain::Mineable))
        return TerrainBQ::Mine;
    else if(Is(ETerrain::Walkable))
        return TerrainBQ::Flag;
    else if(Is(ETerrain::Unreachable))
        return TerrainBQ::Danger;
    else
        return TerrainBQ::Nothing;
}

bool TerrainDesc::IsUsableByAnimals() const
{
    // If it is buildable land or mountain, animals can use it
    return (kind == TerrainKind::Land || kind == TerrainKind::Mountain) && Is(ETerrain::Buildable);
}

bool TerrainDesc::IsVital() const
{
    return kind == TerrainKind::Land && Is(ETerrain::Buildable);
}

TerrainDesc::Triangle TerrainDesc::GetUSDTriangle() const
{
    // Inset by 0.5 on all sites to sample middle of pixel in OGL
    RectBase<float> oglRect(posInTexture.getOrigin() + PointF::all(.5f), posInTexture.getSize() - PointF::all(1));
    const PointF middlePt = oglRect.getOrigin() + oglRect.getSize() / 2.f;
    const PointF middleBottom(middlePt.x, oglRect.bottom);
    Triangle result;
    switch(texType)
    {
        case ETexType::Overlapped:
        default:
            result.tip = middleBottom;
            result.left = oglRect.getOrigin();
            result.right = PointF(oglRect.right, oglRect.top);
            break;
        case ETexType::Stacked:
            result.tip = middleBottom;
            result.left = PointF(oglRect.left, middlePt.y);
            result.right = PointF(oglRect.right, middlePt.y);
            break;
        case ETexType::Rotated:
            result.tip = PointF(oglRect.right, middlePt.y);
            result.left = middleBottom;
            result.right = PointF(oglRect.left, middlePt.y);
            break;
    }
    return result;
}

TerrainDesc::Triangle TerrainDesc::GetRSUTriangle() const
{
    // Inset by 0.5 on all sites to sample middle of pixel in OGL
    RectBase<float> oglRect(posInTexture.getOrigin() + PointF::all(.5f), posInTexture.getSize() - PointF::all(1));
    const PointF middlePt = oglRect.getOrigin() + oglRect.getSize() / 2.f;
    const PointF middleTop(middlePt.x, oglRect.top);
    Triangle result;
    switch(texType)
    {
        case ETexType::Overlapped:
        default:
            result.tip = middleTop;
            result.left = PointF(oglRect.left, oglRect.bottom);
            result.right = PointF(oglRect.right, oglRect.bottom);
            break;
        case ETexType::Stacked:
            result.tip = middleTop;
            result.left = PointF(oglRect.left, middlePt.y);
            result.right = PointF(oglRect.right, middlePt.y);
            break;
        case ETexType::Rotated:
            result.tip = PointF(oglRect.left, middlePt.y);
            result.left = PointF(oglRect.right, middlePt.y);
            result.right = middleTop;
            break;
    }
    return result;
}
