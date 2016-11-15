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

#ifndef Generator_h__
#define Generator_h__

#pragma once

#ifndef PI
#define PI 3.14159265
#endif

#include <string>
#include "mapGenerator/Map.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/Vec2.h"

/**
 * The Generator is an abstract class which defines the basic input and output for map 
 * generation.
 */
class Generator
{
    public:
    
    /**
     * Generates a new random map with the specified settings and saves the new map to the 
     * specified file path.
     * @param filePath path for the ouput file
     * @param settings settings used for the map generation
     */
    void Create(const std::string& filePath, const MapSettings& settings);
    
    private:
    
    void SmoothTextures(Map* map);
    
    protected:
    
    /**
     * Generates a new random map with the specified settings.
     * @param settings settings used to generate the random map
     */
    virtual Map* GenerateMap(const MapSettings& settings) = 0;
    
    /**
     * Sets trees on the map around the specified center within the specified radius.
     * The further away the trees are from the center the lower the likelyhood for placement.
     * @param map map to modify the terrain for
     * @param center center point for tree placement
     * @param radius radius around the center to place trees in
     */
    void SetTrees(Map* map, const Vec2& center, const float radius);
    
    /**
     * Places a tree to the specified position if possible.
     * @param map map to modify the terrain for
     * @param position position of the tree
     */
    void SetTree(Map* map, const Vec2& position);
    
    /**
     * Sets stone on the map around the specified center within the specified radius.
     * The further away the stone is from the center the smaller it is.
     * @param map map to modify the terrain for
     * @param center center point for stone placement
     * @param radius radius around the center to place stone in
     */
    void SetStones(Map* map, const Vec2& center, const float radius);

    /**
     * Places a stone to the specified position if possible.
     * @param map map to modify the terrain for
     * @param position position of the stone
     */
    void SetStone(Map* map, const Vec2& position);
    
    /**
     * Computes a point on a circle. The circle has equally distributed points.
     * The specified index references one of those points.
     * @param index index of the point to compute (must be zero or larger but less than points)
     * @param points number of equally distributed points on the circle (must be larger than zero)
     * @param center center point of the circle
     * @param radius radius of the circle (must be a positive value)
     * @return the point on the circle with the specified index
     */
    Vec2 ComputePointOnCircle(const int index, const int points, const Vec2& center, const float radius);
};

#endif // Generator_h__
