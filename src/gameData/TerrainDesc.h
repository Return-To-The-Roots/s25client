// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef TerrainDesc_h__
#define TerrainDesc_h__

#include "DescIdx.h"
#include "Rect.h"
#include "gameTypes/LandscapeType.h"
#include <boost/core/scoped_enum.hpp>
#include <string>

struct EdgeDesc;
struct WorldDescription;
namespace kaguya {
class LuaRef;
}

BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(TerrainBQ, uint8_t){NOTHING, DANGER, FLAG, CASTLE, MINE} BOOST_SCOPED_ENUM_DECLARE_END(TerrainBQ)
  BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(TerrainKind, uint8_t){LAND, WATER, LAVA, SNOW, MOUNTAIN} BOOST_SCOPED_ENUM_DECLARE_END(TerrainKind)
  /// Bitset of what can be done on that terrain
  BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(ETerrain,
                                     uint8_t){/// Can do nothing, but also not dangerous. Don't use in code! Use !Is(Walkable) etc. instead!
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
                                              Mineable = 16 | Walkable} BOOST_SCOPED_ENUM_DECLARE_END(ETerrain)

    struct TerrainDesc
{
    std::string name;
    Landscape landscape;
    uint8_t s2Id;
    DescIdx<EdgeDesc> edgeType;
    int8_t edgePriority;
    TerrainKind kind;
    uint8_t numFrames;
    uint8_t palAnimIdx;
    ETerrain flags;
    std::string texturePath;
    Rect posInTexture;
    unsigned minimapColor;

    TerrainDesc(const kaguya::LuaRef& luaData, const WorldDescription& worldDesc);
    /// Get the building quality of the terrain from the flags
    TerrainBQ GetBQ() const;
    /// Returns whether "regular" animals walk on that terrain (no water or snow animals)
    bool IsUsableByAnimals() const;
    /// Returns whether corn fields can grow on the terrain (no dessert, mountain etc)
    bool IsVital() const;
    /// Return true if the terrain has the given attribute
    bool Is(ETerrain what) const;
};

inline bool TerrainDesc::Is(ETerrain what) const
{
    return (boost::underlying_cast<uint8_t>(flags) & boost::underlying_cast<uint8_t>(what)) == boost::underlying_cast<uint8_t>(what);
}

#endif // TerrainDesc_h__
