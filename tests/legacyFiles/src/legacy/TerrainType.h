// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
