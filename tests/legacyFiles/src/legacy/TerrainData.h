// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LandscapeType.h"
#include "Rect.h"
#include "TerrainType.h"
#include "gameData/TerrainDesc.h"
#include <array>

/// Static class returning properties of terrain types
class TerrainData
{
public:
    // Disallow construction, only static methods
    TerrainData() = delete;

    /// Returns the terrain type for a given map terrain index
    static TerrainType MapIdx2Terrain(unsigned char mapIdx);
    /// Returns the position in the texture image
    static Rect GetPosInTexture(TerrainType t);
    /// Returns whether the terrains texture is animated
    static bool IsAnimated(TerrainType t);
    /// Gets the number of frames in the animation
    static unsigned GetNumFrames(TerrainType t);
    /// Gets the texture identifier for the terrain in the SWD file format
    static unsigned char GetTextureIdentifier(TerrainType t);
    /// Gets the start color for the animation (extraction)
    static unsigned char GetStartColor(TerrainType t);
    /// Returns the color for the terrain
    static unsigned GetColor(Landscape landsCape, TerrainType t);
    /// Gets the edge type for a terrain type
    static EdgeType GetEdgeType(Landscape landsCape, TerrainType t);
    /// Gets the edge type that t1 draws over t2
    /// 0: None, 1: Snow, 2: Mountain, 3: Desert, 4: Meadow, 5: Water
    static unsigned char GetEdgeType(Landscape landsCape, TerrainType t1, TerrainType t2);
    static void PrintEdgePrios();
    static const std::array<int, NUM_TTS>& GetEdgePrios(Landscape landsCape);
    /// Returns whether the given map terrain index is a harbour spot
    static bool IsHarborSpot(unsigned char mapIdx) { return (mapIdx & 0x40) != 0; }
    /// Returns whether the given terrain type can be used (is not deadly)
    static bool IsUseable(TerrainType t);
    /// Returns whether the given terrain type can be used by a ship (non-blocking water)
    static bool IsUsableByShip(TerrainType t);
    /// Returns whether "regular" animals walk on that terrain (no water or snow animals)
    static bool IsUsableByAnimals(TerrainType t);
    /// Returns whether corn fields can grow on the terrain (no dessert, mountain etc)
    static bool IsVital(TerrainType t);
    /// Returns whether the given terrain is any kind of water
    static bool IsWater(TerrainType t);
    /// Returns whether the given terrain is any kind of lava
    static bool IsLava(TerrainType t);
    /// Returns whether the given terrain is any kind of snow
    static bool IsSnow(Landscape lt, TerrainType t);
    /// Returns whether the given terrain is a mountain
    static bool IsMountain(TerrainType t);
    /// Returns whether the given terrain is a mineable mountain
    static bool IsMineable(TerrainType t);
    /// Returns what kind of buildings can be build on that terrain
    static TerrainBQ GetBuildingQuality(TerrainType t);
};
