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

#ifndef GridUtility_h__
#define GridUtility_h__

#include "gameTypes/MapCoordinates.h"
#include <vector>

class GridUtility
{
public:
    
    static Position GetPosition(int index, const MapExtent& size);
    
    static Position Clamp(const Position& p, const MapExtent& size);
    
    static Position Delta(const Position& p1, const Position& p2, const MapExtent& size);
    
    static double Distance(const Position& p1, const Position& p2, const MapExtent& size);
    
    static double DistanceNorm(const Position& p1, const Position& p2, const MapExtent& size);
    
    static std::vector<Position> Positions(const MapExtent& size);
    
    static std::vector<Position> Neighbors(const Position& p, const MapExtent& size);
    
    static std::vector<Position> Collect(const Position& p, const MapExtent& size, double distance);
    
    static std::vector<Position> Collect(const Position& p, const MapExtent& size, const std::vector<bool>& property);

    static std::vector<Position> NeighborsOfRsuTriangle(const Position& p,
                                                        const MapExtent& size);

    static std::vector<Position> NeighborsOfLsdTriangle(const Position& p,
                                                        const MapExtent& size);
};

#endif // GridUtility_h__
