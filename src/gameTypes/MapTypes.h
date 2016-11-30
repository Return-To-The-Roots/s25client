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
//-V:MapPoint:801 
//-V:MapPoint:813 

/// Terrainarten
enum TerrainType
{
    TT_SNOW                 = 0x02,
    TT_DESERT               = 0x04,
    TT_SWAMPLAND            = 0x03,
    TT_MEADOW_FLOWERS       = 0x0F,
    TT_MOUNTAIN1            = 0x01,
    TT_MOUNTAIN2            = 0x0B,
    TT_MOUNTAIN3            = 0x0C,
    TT_MOUNTAIN4            = 0x0D,
    TT_SAVANNAH             = 0x00,
    TT_MEADOW1              = 0x08,
    TT_MEADOW2              = 0x09,
    TT_MEADOW3              = 0x0A,
    TT_STEPPE               = 0x0E,
    TT_MOUNTAINMEADOW       = 0x12,
    TT_WATER                = 0x05,
    TT_LAVA                 = 0x10,
    TT_WATER_NOSHIP         = 0x06,
    TT_BUILDABLE_WATER      = 0x13,
    TT_BUILDABLE_MOUNTAIN   = 0x22,
    TT_LAVA2                = 0x14,
    TT_LAVA3                = 0x15,
    TT_LAVA4                = 0x16
};

// Keep this in sync with TerrainType
static const unsigned char TT_COUNT = 22;

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
