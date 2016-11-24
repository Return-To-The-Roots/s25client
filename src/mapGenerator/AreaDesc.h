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

#ifndef AreaDesc_h__
#define AreaDesc_h__

#include "defines.h" // IWYU pragma: keep

struct AreaDesc
{
    AreaDesc(double cx, double cy, double minDist, double maxDist, double pHill, int pTree, int pStone,
             int minZ, int maxZ, int minPlayerDist, int maxPlayerDist = -1)
    : centerX(cx), centerY(cy), minDistance(minDist), maxDistance(maxDist),
      likelyhoodHill(pHill), likelyhoodTree(pTree), likelyhoodStone(pStone),
      minElevation(minZ), maxElevation(maxZ), minPlayerDistance(minPlayerDist), maxPlayerDistance(maxPlayerDist)
    {
    }
    
    /**
     * X-coordinate of the center point of the area relative to the width of the map.
     */
    double centerX;

    /**
     * Y-coordinate of the center point of the area relative to the height of the map.
     */
    double centerY;

    /**
     * Minimum distance of the area to the center point of the map.
     * One unit equals the half of the map length (= min(width, height)).
     */
    double minDistance;

    /**
     * Maximums distance of the area to the center point of the map.
     * One unit equals the half of the map length (= min(width, height)).
     */
    double maxDistance;
    
    /**
     * Likelyhood (0.0 - 100.0) to generate a hill on a tile within this area.
     */
    double likelyhoodHill;
    
    /**
     * Likelyhood (0-100) to generate a tree within this area.
     */
    int likelyhoodTree;

    /**
     * Likelyhood (0-100) to generate a pile of stone within this area.
     */
    int likelyhoodStone;
    
    /**
     * Minimum elevation of the area.
     */
    int minElevation;
    
    /**
     * Maximum elevation of the area.
     */
    int maxElevation;
    
    /**
     * Minimum distance of the area to any of the players' headquarters in tiles.
     */
    int minPlayerDistance;

    /**
     * Maximum distance of the area to any of the players' headquarters in tiles.
     * The default is "-1" to ignore maximum distance.
     */
    int maxPlayerDistance;
    
    /**
     * Checks whether ot not the specified point is within this area.
     * @param x-coordinate of the point to check
     * @param y-coordinate of the point to check
     * @param playerDistance distance to the nearest player position
     * @param width width of the map
     * @param height height of the map
     * @return true of the point is within the of the area, false otherwise
     */
    bool IsInArea(int x, int y, const double playerDistance, const int width, const int height);
};

#endif // AreaDesc_h__
