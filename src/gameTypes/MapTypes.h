// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "Point.h"

/// Datentyp für Map-Koordinaten
typedef unsigned short MapCoord;
typedef Point<MapCoord> MapPoint;

/// Terrainarten
enum TerrainType
{
    TT_SNOW = 0,
    TT_DESERT,
    TT_SWAMPLAND,
    TT_MEADOW_FLOWERS,
    TT_MOUNTAIN1,
    TT_MOUNTAIN2,
    TT_MOUNTAIN3,
    TT_MOUNTAIN4,
    TT_SAVANNAH,
    TT_MEADOW1,
    TT_MEADOW2,
    TT_MEADOW3,
    TT_STEPPE,
    TT_MOUNTAINMEADOW,
    TT_WATER,
    TT_LAVA,
    TT_WATER2,
    TT_BUILDABLE_WATER,
    TT_BUILDABLE_MOUNTAIN
};

// Keep this in sync with TerrainType
static const unsigned char TT_COUNT = TT_BUILDABLE_MOUNTAIN + 1;

enum EdgeType
{
    ET_NONE,
    ET_SNOW,
    ET_MOUNTAIN,
    ET_DESSERT,
    ET_MEADOW,
    ET_WATER
};

/// Sichtbarkeit für ALLE Spieler
enum Visibility
{
    VIS_INVISIBLE = 0, /// Darkness
    VIS_FOW, /// Fog of war
    VIS_VISIBLE /// Visible
};

/// Granittyp
enum GraniteType
{
    GT_1 = 0,
    GT_2
};

/// Flaggentyp
enum FlagType
{
    FT_NORMAL,
    FT_LARGE,
    FT_WATER
};
#endif // MapTypes_h__
