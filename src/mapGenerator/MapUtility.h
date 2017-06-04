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

#ifndef MapUtility_h__
#define MapUtility_h__

#include "mapGenerator/Map.h"

/**
 * Utility for map generation.
 */
class MapUtility
{
    public:
    
    /**
     * Smoothes the textures of the map for better visual appearance by post-processing
     * the specified map. Single textures which are surounded by other textures are replaced
     * by their neighboring textures. 
     * Also, the height values of snow- and mountain-textures are increased and mountain-
     * meadow textures without neighboring mountain-textures are replaced by simple meadow
     * textures.
     * @param map map to smooth textures for
     */
    void Smooth(Map& map);
    
    /**
     * Creates a hill at the specified center with the specified height.
     * @param map map to create the hill on
     * @param center center point of the hill (highest elevation)
     * @param z maximum height (elevation) of the hill
     */
    void SetHill(Map& map, const Point<int>& center, int z);
    
    /**
     * Sets up a harbor position at the specified center. The surounding area is flattened 
     * an textures are replaced to enable harbor building.
     * @param map map to modify the terrain for
     * @param center center point for the harbor position
     * @param waterLevel the height level of the surounding water
     */
    void SetHarbour(Map& map, const Point<int>& center, int waterLevel);
    
    /**
     * Places a tree to the specified position if possible.
     * @param map map to modify the terrain for
     * @param position position of the tree
     */
    void SetTree(Map& map, const Point<int>& position);
    
    /**
     * Sets stone on the map around the specified center within the specified radius.
     * The further away the stone is from the center the smaller it is.
     * @param map map to modify the terrain for
     * @param center center point for stone placement
     * @param radius radius around the center to place stone in
     */
    void SetStones(Map& map, const Point<int>& center, double radius);

    /**
     * Places a stone to the specified position if possible.
     * @param map map to modify the terrain for
     * @param position position of the stone
     */
    void SetStone(Map& map, const Point<int>& position);
    
    /**
     * Computes the size of a terrain body starting from the specified position.
     * @param map map to evaluate
     * @param p position of the intial area
     * @param max the maximum number of tiles to check for (performance)
     * @return the number of vertices in a connected terrain area around the 
     * intial position
     */
    unsigned int GetBodySize(Map& map, const Point<int>& p, unsigned int max);
    
    /**
     * Computes a point on a circle. The circle has equally distributed points.
     * The specified index references one of those points.
     * @param index index of the point to compute (must be zero or larger but less than points)
     * @param points number of equally distributed points on the circle (must be larger than zero)
     * @param center center point of the circle
     * @param radius radius of the circle (must be a positive value)
     * @return the point on the circle with the specified index
     */
    Point<int> ComputePointOnCircle(int index,
                                    int points,
                                    const Point<int>& center,
                                    double radius);
};

#endif // MapUtility_h__
