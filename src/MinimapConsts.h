// $Id: MinimapConsts.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef MINIMAP_CONSTS_H_
#define MINIMAP_CONSTS_H_

// Farben für die einzelnen Terrains (ARGB)
const unsigned TERRAIN_COLORS[3][16] =
{
    // Grünland
    {
        0xFFFFFFFF,//TT_SNOW = 0,
        0xFFc09c7c,//TT_DESERT,
        0xFF649014,//TT_SWAMPLAND,
        0xFF48780c,//TT_MEADOW_FLOWERS,
        0xFF9c8058,//TT_MOUNTAIN1,
        0xFF9c8058,//TT_MOUNTAIN2,
        0xFF9c8058,//TT_MOUNTAIN3,
        0xFF8c7048,//TT_MOUNTAIN4,
        0xFF649014,//TT_SAVANNAH,
        0xFF48780c,//TT_MEADOW1,
        0xFF649014,//TT_MEADOW2,
        0xFF407008,//TT_MEADOW3,
        0xFF88b028,//TT_STEPPE,
        0xFF9c8058,//TT_MOUNTAINMEADOW,
        0xFF1038a4,//TT_WATER,
        0xFFc02020 //TT_LAVA
    },

    // Ödland
    {
        0xFF860000,//TT_SNOW is lava, too. was 0xFFFFFFFF
        0xFF9c7c64,//TT_DESERT,
        0xFF001820,//TT_SWAMPLAND,
        0xFF444850,//TT_MEADOW_FLOWERS,
        0xFF706c54,//TT_MOUNTAIN1,
        0xFF706454,//TT_MOUNTAIN2,
        0xFF684c24,//TT_MOUNTAIN3,
        0xFF684c24,//TT_MOUNTAIN4,
        0xFF444850,//TT_SAVANNAH,
        0xFF5c5840,//TT_MEADOW1,
        0xFF646048,//TT_MEADOW2,
        0xFF646048,//TT_MEADOW3,
        0xFF88b028,//TT_STEPPE,
        0xFF001820,//TT_MOUNTAINMEADOW,
        0xFF454520,//TT_WATER       was 0xFF1038a4,//TT_WATER,
        0xFFC32020 //TT_LAVA        was 0xFFc02020 //TT_LAVA
    },

    // Winterwelt
    {
        0xFF00286C,//TT_SNOW = 0,  was 0xFFFFFFFF,//TT_SNOW = 0,
        0xFF0070b0,//TT_DESERT,
        0xFF00286c,//TT_SWAMPLAND,
        0xFF7c84ac,//TT_MEADOW_FLOWERS,
        0xFF54586c,//TT_MOUNTAIN1,
        0xFF60607c,//TT_MOUNTAIN2,
        0xFF686c8c,//TT_MOUNTAIN3,
        0xFF686c8c,//TT_MOUNTAIN4,
        0xFFa0accc,//TT_SAVANNAH,
        0xFFb0a494,//TT_MEADOW1,
        0xFF88a874,//TT_MEADOW2,
        0xFFa0accc,//TT_MEADOW3,
        0xFF88b15e,//TT_STEPPE,
        0xFF94a0c0,//TT_MOUNTAINMEADOW,
        0xFF1038a4,//TT_WATER,
        0xFFc02020 //TT_LAVA
    },

};

/// Farbe für Bäume
const unsigned TREE_COLOR = 0xFF003c14;
/// Variierung der Helligkeit der Bäume
const int VARY_TREE_COLOR = 20;
/// Farbe für Granit
const unsigned GRANITE_COLOR = 0xFFA2A2A2;
/// Variierung der Helligkeit der Granitblöcke
const int VARY_GRANITE_COLOR = 20;
/// Farbe für Gebäude
const unsigned BUILDING_COLOR = 0xFFFFFFFF;
/// Färbe für Straßen
const unsigned ROAD_COLOR = 0xFFAAAAAA;

/// Skalierung in x-Richtung bei der Anzeige der Map
const double MINIMAP_SCALE_X = 1.5;


#endif
