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

#include "gameTypes/MapCoordinates.h"
#include <vector>

class VertexUtility
{
public:
    /**
     * Computes the position of the vertex with the specified index on the map.
     * @param index vertex index
     * @param size of the map
     * @return the position of the vertex on the map
     */
    static Position GetPosition(int index, const MapExtent& size);

    /**
     * Computes the index of a vertex on the map. If the position is outside of the map's boundary, the
     * coordinates are continuing on the other side of the map again (fluent boundaries).
     * @param p position of the vertex on the map
     * @param size of the map (MUST be of the power of 2)
     * @return the index of the vertex
     */
    static int GetIndexOf(Position p, const MapExtent& size);

    /**
     * Computes indices of neighboring vertices within the specified radius around the position.
     * @param p center position to collect neighboring vertices around
     * @param size of the map
     * @param radius radius for collecting neighbor vertices
     * @return vector of indices for neighbor vertices
     */
    static std::vector<int> GetNeighbors(const Position& p, const MapExtent& size, int radius);

    /**
     * Computes the distance between two vertices.
     * @param p1 position of the first vertex
     * @param p2 position of the second vertex
     * @param size of the map
     * @return the distance between the two vertices
     */
    static double Distance(const Position& p1, const Position& p2, const MapExtent& size);
};
