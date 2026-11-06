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

using Triangle = TerrainDesc::Triangle;

// Size of a triangle on the screen (TR_W, TR_H)
constexpr float triWidth = 56;
constexpr float triHeight = 28;
// Corners of the triangles on the screen on flat terrain. OpenGL draws exactly these triangles.
const Triangle rsuOnScreen{{0, 0}, {-triWidth / 2, triHeight}, {triWidth / 2, triHeight}};
const Triangle usdOnScreen{{triWidth / 2, triHeight}, {0, 0}, {triWidth, 0}};
// S2 maps its texture coordinates to the corners moved 1 pixel outwards, so its triangles are 58x30 pixels
// and overlap the neighboring triangles by 1 pixel.
const Triangle rsuS2{{0, -1}, {-triWidth / 2 - 1, triHeight + 1}, {triWidth / 2 + 1, triHeight + 1}};
const Triangle usdS2{{triWidth / 2, triHeight + 1}, {-1, -1}, {triWidth + 1, -1}};
// S2 samples pixel (x, y) at (x + 1.5, y) on average, while OpenGL samples at the pixel center (x + 0.5, y + 0.5)
const PointF s2SampleOffset(1, -0.5f);

// The corners of the texture areas S2 uses are slightly inside the outer texels:
// 1/16 texel after the start and 17/256 texel before the end of an area.
// The corners of the rotated textures are (almost) at the texel centers.
constexpr float s2Start = 1 / 16.f;
constexpr float s2End = -17 / 256.f;
constexpr float s2Center = 127 / 256.f;

/// Texture coordinates S2 uses for the corners of its (larger) triangles,
/// relative to a texture rectangle of the size RTTR uses for this texture type
struct S2TexCoords
{
    PointF size;
    Triangle rsu, usd;
};

// RSU and USD use the full 32x32 texels
const S2TexCoords overlappedCoords{{32, 31},
                                   {{16, s2Start}, {s2Start, 32 + s2End}, {32 + s2End, 32 + s2End}},
                                   {{16, 32 + s2End}, {s2Start, s2Start}, {32 + s2End, s2Start}}};
// 30 texels wide, RSU in rows 1-16, USD in rows 17-32
const S2TexCoords stackedCoords{{31, 33},
                                {{15, 1 + s2Start}, {s2Start, 16 + s2End}, {30 + s2End, 16 + s2End}},
                                {{15, 32 + s2End}, {s2Start, 17 + s2Start}, {30 + s2End, 17 + s2Start}}};
// RSU is the upper half of a diamond 52 texels wide with the middle at row 26, USD the lower half with the middle
// at 27. The tip of RSU is at the left corner of the diamond, the tip of USD at the right corner.
const S2TexCoords rotatedCoords{
  {53, 54},
  {{s2Center, 26 + s2Center}, {52 + s2Center, 26 + s2Center}, {26 + s2Center, s2Center}},
  {{52 + s2Center, 27 + s2Center}, {26 + s2Center, 53 + s2Center}, {s2Center, 27 + s2Center}}};

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

/// Map pt with the affine mapping that maps the corners of from to the corners of to
PointF mapAffine(const Triangle& from, const Triangle& to, const PointF& pt)
{
    const PointF a = from.left - from.tip;
    const PointF b = from.right - from.tip;
    const PointF d = pt - from.tip;
    const float det = a.x * b.y - b.x * a.y;
    const float s = (d.x * b.y - b.x * d.y) / det;
    const float t = (a.x * d.y - d.x * a.y) / det;
    return to.tip + (to.left - to.tip) * s + (to.right - to.tip) * t;
}

/// Get the texture coordinates for the corners of the triangle on the screen,
/// so that OpenGL samples the same texels S2 does.
/// Scaled from the S2 size to the size of the rectangle and offset by its origin.
Triangle getTexTriangle(const Triangle& onScreen, const Triangle& s2Screen, const Triangle& s2Tex, const PointF& size,
                        const Rect& rect)
{
    const PointF origin(rect.getOrigin());
    const PointF scale = PointF(rect.getSize()) / size;
    const auto getTexCoord = [&](const PointF& corner) {
        return origin + mapAffine(s2Screen, s2Tex, corner + s2SampleOffset) * scale;
    };
    return {getTexCoord(onScreen.tip), getTexCoord(onScreen.left), getTexCoord(onScreen.right)};
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
    return getTexTriangle(usdOnScreen, usdS2, coords.usd, coords.size, posInTexture);
}

TerrainDesc::Triangle TerrainDesc::GetRSUTriangle() const
{
    const S2TexCoords& coords = getS2TexCoords(texType);
    return getTexTriangle(rsuOnScreen, rsuS2, coords.rsu, coords.size, posInTexture);
}
