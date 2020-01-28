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

#ifndef DistanceField_h__
#define DistanceField_h__

#include "randomMaps/Map.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class DistanceField
{
private:
    
    /**
     * Function to check if the map at the specified position index possesses a certain property.
     */
    bool (*hasProperty_)(const Map* map, int);
    
public:
    
    /**
     * Creates a new instance for computing distances of positions without the property to map positions which
     * possess the property.
     * @param hasProperty pointer to a function to check whether or not a property is given for an indexed position on the map
     */
    DistanceField(bool (*hasProperty)(const Map* map, int index));
    
    /**
     * Computes the distance field for the entire map.
     * @param map map to compute the distance field for
     * @return vector field of distance's for each index of the map to the nearest position with the property
     */
    std::vector<int> Compute(const Map* map);
    
    /**
     * Computes the distance field for the specified subset of map positions.
     * @param map map to compute the distance field for
     * @param subset subset of positions to compute the distance vector for
     * @return vector field of distance's for each index of the map to the nearest position with the property
     */
    std::vector<int> Compute(const Map* map, const std::vector<Position>& subset);
    
    static void Add(std::vector<int>& first, const std::vector<int>& second);
};

#endif // DistanceField_h__
