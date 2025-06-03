// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DescIdx.h"
#include "Rect.h"
#include <cstdint>
#include <string>

struct LandscapeDesc;
struct EdgeDesc;
struct WorldDescription;
class CheckedLuaTable;

enum class TerrainBQ : uint8_t
{
    Nothing,
    Danger,
    Flag,
    Castle,
    Mine
};
enum class TerrainKind : uint8_t
{
    Land,
    Water,
    Lava,
    Snow,
    Mountain
};
/// Bitset of what can be done on that terrain
enum class ETerrain : uint8_t
{ /// Can do nothing, but also not dangerous. Don't use in code! Use !Is(Walkable) etc. instead!
    Unwalkable = 0,
    /// Dangerous, can't go near
    Unreachable = 1,
    /// Can walk on
    Walkable = 2,
    /// Ships can drive on
    Shippable = 4,
    /// Can build buildings
    Buildable = 8 | Walkable,
    /// Can build mines
    Mineable = 16 | Walkable
};

enum class ETexType : uint8_t
{
    Overlapped,
    Stacked,
    Rotated
};

struct TerrainDesc
{
    struct Triangle
    {
        PointF tip, left, right;
    };
    std::string name;
    DescIdx<LandscapeDesc> landscape;
    uint8_t s2Id;
    DescIdx<EdgeDesc> edgeType;
    /// Priority for drawing. The terrain with the higher priority draws its edge over the neighbor
    int8_t edgePriority;
    TerrainKind kind;
    int8_t palAnimIdx;
    ETerrain flags;
    /// How much water can be on the terrain [0, 100]
    uint8_t humidity;
    ETexType texType;
    std::string texturePath;
    Rect posInTexture;
    unsigned minimapColor;

    TerrainDesc(CheckedLuaTable luaData, const WorldDescription& worldDesc);
    ~TerrainDesc();
    /// Get the building quality of the terrain from the flags
    TerrainBQ GetBQ() const;
    /// Returns whether "regular" animals walk on that terrain (no water or snow animals)
    bool IsUsableByAnimals() const;
    /// Returns whether corn fields can grow on the terrain (no dessert, mountain etc)
    bool IsVital() const;
    /// Return true if the terrain has the given attribute
    bool Is(ETerrain what) const;
    /// Get the position of the up side down triangle
    Triangle GetUSDTriangle() const;
    /// Get the position of right side up down triangle
    Triangle GetRSUTriangle() const;
};

inline bool TerrainDesc::Is(ETerrain what) const
{
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(what)) == static_cast<uint8_t>(what);
}
