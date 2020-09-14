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

#pragma once

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
    TT_WATER_NOSHIP,
    TT_BUILDABLE_WATER,
    TT_BUILDABLE_MOUNTAIN,
    TT_LAVA2,
    TT_LAVA3,
    TT_LAVA4
};

// Keep this in sync with TerrainType
static const unsigned char NUM_TTS = TT_LAVA4 + 1;

enum EdgeType
{
    ET_NONE,
    ET_SNOW,
    ET_MOUNTAIN,
    ET_DESSERT,
    ET_MEADOW,
    ET_WATER
};
