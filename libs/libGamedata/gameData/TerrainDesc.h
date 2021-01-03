// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
    using PointF = Point<float>;
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
