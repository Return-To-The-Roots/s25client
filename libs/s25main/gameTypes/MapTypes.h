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

#ifndef MapTypes_h__
#define MapTypes_h__

#include "helpers/MaxEnumValue.h"
#include <cstdint>

/// Sichtbarkeit für ALLE Spieler
enum Visibility : uint8_t
{
    VIS_INVISIBLE = 0, /// Darkness
    VIS_FOW,           /// Fog of war
    VIS_VISIBLE        /// Visible
};
DEFINE_MAX_ENUM_VALUE(Visibility, Visibility::VIS_VISIBLE)

/// Granittyp
enum GraniteType : uint8_t
{
    GT_1 = 0,
    GT_2
};
DEFINE_MAX_ENUM_VALUE(GraniteType, GraniteType::GT_2)

/// Flaggentyp
enum FlagType : uint8_t
{
    FT_NORMAL,
    FT_LARGE,
    FT_WATER
};
DEFINE_MAX_ENUM_VALUE(FlagType, FlagType::FT_WATER)

/// Direction from a point where a road can go. Opposites are stored in neighbors
enum class RoadDir
{
    East,
    SouthEast,
    SouthWest
};
DEFINE_MAX_ENUM_VALUE(RoadDir, RoadDir::SouthWest)

/// Type of the road "owned" by a point
enum class PointRoad : unsigned char
{
    None,
    Normal,
    Donkey,
    Boat
};
DEFINE_MAX_ENUM_VALUE(PointRoad, PointRoad::Boat)

#endif // MapTypes_h__
