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

#ifndef HarborGenerator_h__
#define HarborGenerator_h__

#include "randomMaps/terrain/CoastTile.h"
#include "randomMaps/algorithm/Brush.h"
#include "randomMaps/Map.h"

struct HarborParams
{
    HarborParams(Map& map) : map(map) {}
    Map& map;
    unsigned char waterLevel;
    TextureType texture;
};

struct TextureParams
{
    TextureParams(Map& map, TextureType texture) : map(map), number(0), texture(texture) {}
    Map& map;
    int number;
    TextureType texture;
};

/**
 * The harbor generator creates harbor positions in suitable locations on a map.
 */
class HarborGenerator
{
private:

    /**
     * Brush to ensure the harbor has water access.
     * @param params parameters to prepare the landscape around the harbor
     * @param index index of the map vertex to prepare
     * @param rightSideUp whether to prepare the right-side-up texture (true) of
     *      the vertex or the left-side-down texture (false)
     */
    static void EnsureWaterAccess(HarborParams& params, int index, bool rsu);
    
    /**
     * Brush method for creating a harbor position.
     * @param params parameters to prepare the landscape around the harbor
     * @param index index of the map vertex to prepare
     * @param rightSideUp whether to prepare the right-side-up texture (true) of
     *      the vertex or the left-side-down texture (false)
     */
    static void EnsureHarborIsBuildable(HarborParams& params, int index, bool rsu);

    /**
     * Brush method for preparing tiles around the harbor position.
     * @param params parameters to prepare the landscape around the harbor
     * @param index index of the map vertex to prepare
     * @param rightSideUp whether to prepare the right-side-up texture (true) of
     *      the vertex or the left-side-down texture (false)
     */
    static void EnsureTerrainIsBuildable(HarborParams& params, int index, bool rsu);
    
    /**
     * Flattens the land around the harbor position.
     * @param params parameters to prepare the landscape around the harbor
     * @param index index of the map vertex to prepare
     * @param rightSideUp whether to prepare the right-side-up texture (true) of
     *      the vertex or the left-side-down texture (false)
     */
    static void EnsureTerrainIsFlat(HarborParams& params, int index, bool rsu);
    
    /**
     * Counts the number of textures specified in the params.
     */
    static void TextureTileCounter(TextureParams& params, int index, bool rsu);
    
    /**
     * Gets the corresponding brush shape for the specified direction vector.
     * @param vector direction vector
     */
    BrushSettings GetBrushShape(Map& map, const Position& vector);
    
public:
    
    /**
     * Creates harbor positions in suitable spots on the specified map.
     * @param map map to place harbor positions on
     * @param pos position of the harbor
     */
    void Build(Map& map, const CoastTile& pos);
};

#endif // HarborGenerator_h__
