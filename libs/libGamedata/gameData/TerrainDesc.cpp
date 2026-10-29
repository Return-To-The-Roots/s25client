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
/// Texture coordinates of the triangles in a texture rectangle of the size the original uses for this type
struct S2TexCoords
{
    PointF size;
    TerrainDesc::Triangle rsu, usd;
};

// S2 draws each triangle 1 pixel larger with the texture coordinates from its own table and samples pixel (x, y)
// on average at (x + 1.5, y), while OpenGL samples at the pixel center (x + 0.5, y + 0.5).
// So the corners get the texture coordinates the original has at corner + (1, -0.5) on flat terrain.
// This samples the same texels as S2 does. Using the rectangle corners inset by half a pixel instead
// would draw the textures about 1 pixel further right than S2.
// All values are inside the rectangle, so we never sample from outside the texture (black lines, #896).
const S2TexCoords overlappedCoords{{32, 31},
                                   {{16.549f, 0.594f}, {1.162f, 30.340f}, {31.934f, 30.340f}},
                                   {{16.549f, 30.340f}, {1.162f, 0.594f}, {31.934f, 0.594f}}};
const S2TexCoords stackedCoords{{31, 33},
                                {{15.515f, 1.310f}, {1.093f, 15.190f}, {29.934f, 15.190f}},
                                {{15.515f, 31.190f}, {1.093f, 17.310f}, {29.934f, 17.310f}}};
const S2TexCoords rotatedCoords{{53, 54},
                                {{0.698f, 25.831f}, {49.650f, 26.250f}, {24.546f, 1.146f}},
                                {{50.098f, 27.698f}, {26.250f, 52.383f}, {1.146f, 27.279f}}};

const S2TexCoords& getS2TexCoords(ETexType texType)
{
    switch(texType)
    {
        case ETexType::Overlapped:
        default: return overlappedCoords;
        case ETexType::Stacked: return stackedCoords;
        case ETexType::Rotated: return rotatedCoords;
    }
}

/// Scale the triangle from a rectangle of the given size to the rectangle in the texture
TerrainDesc::Triangle scaleToRect(const TerrainDesc::Triangle& triangle, const PointF& size, const Rect& rect)
{
    const PointF origin(rect.getOrigin());
    const PointF scale = PointF(rect.getSize()) / size;
    return {origin + triangle.tip * scale, origin + triangle.left * scale, origin + triangle.right * scale};
}
} // namespace

TerrainDesc::TerrainDesc(CheckedLuaTable luaData, const WorldDescription& worldDesc)
{
    luaData.getOrThrow(name, "name");
    landscape = worldDesc.landscapes.getIndex(luaData.getOrThrow<std::string>("landscape"));
    if(!landscape)
        throw GameDataError("Invalid landscape type: " + luaData.getOrThrow<std::string>("landscape"));
    s2Id = luaData.getOrDefault<uint8_t>("s2Id", 0xFF);
    auto edgeTypeName = luaData.getOrThrow<std::string>("edgeType");
    if(edgeTypeName != "none" && !worldDesc.edges.getIndex(edgeTypeName))
        throw GameDataError("Invalid edge type: " + edgeTypeName);
    else
        edgeType = worldDesc.edges.getIndex(edgeTypeName);
    edgePriority = luaData.getOrDefault<int8_t>("edgePriority", 0);
    kind = strToTerrainKind(luaData.getOrDefault<std::string>("kind", "land"));
    auto property = luaData.getOrDefault<std::string>("property", "");
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
    const S2TexCoords& coords = getS2TexCoords(texType);
    return scaleToRect(coords.usd, coords.size, posInTexture);
}

TerrainDesc::Triangle TerrainDesc::GetRSUTriangle() const
{
    const S2TexCoords& coords = getS2TexCoords(texType);
    return scaleToRect(coords.rsu, coords.size, posInTexture);
}
