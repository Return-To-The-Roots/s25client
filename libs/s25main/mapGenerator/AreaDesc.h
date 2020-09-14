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

#pragma once

#include "Point.h"
#include "gameTypes/MapCoordinates.h"

/**
 * Descriptor class for specific areas on a random map. Each area describes a particular
 * landscape style used for map generation in a particular location.
 */
struct AreaDesc
{
    /**
     * Creates a new area description.
     * @param center center of the area (unit: 0.0 and 1.0 is the edge of the map)
     * @param minDist minimum distance to the area center (unit: 1.0 equals length of the map)
     * @param maxDist maximum distance to the area center (unit: 1.0 equals length of the map)
     * @param pHill likelihood (in percentage) to place a hill on tile
     * @param pTree likelihood (in percentage) to place a tree on a non-water tile
     * @param pStone likelihood (in percentage) to place stone on a non-water tile
     * @param minZ minimum height of the area (used for hill generation)
     * @param maxZ maximum height of the area (used for hill generation)
     * @param minPlayerDist minimum distance to each player in tiles
     * @param maxPlayerDist maximum distance to each player in tiles (default: -1 means unlimited)
     */
    AreaDesc(const Point<double>& center, double minDist, double maxDist, double pHill, unsigned pTree, unsigned pStone, unsigned minZ,
             unsigned maxZ, int minPlayerDist, int maxPlayerDist = -1);

    /**
     * Center point of the area relative to map width and height.
     */
    Point<double> center;

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
    unsigned likelyhoodTree;

    /**
     * Likelyhood (0-100) to generate a pile of stone within this area.
     */
    unsigned likelyhoodStone;

    /**
     * Minimum elevation of the area.
     */
    unsigned minElevation;

    /**
     * Maximum elevation of the area.
     */
    unsigned maxElevation;

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
     * Checks whether the specified point is within this area.
     * @param point the point to check
     * @param playerDistance distance to the nearest player position in tiles
     * @param size of the map
     * @return true of the point is within the of the area, false otherwise
     */
    bool IsInArea(const Position& point, double playerDistance, const MapExtent& size);
};
