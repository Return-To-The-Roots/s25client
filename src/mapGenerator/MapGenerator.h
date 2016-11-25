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

#ifndef MapGenerator_h__
#define MapGenerator_h__

#include "mapGenerator/MapStyle.h"
#include "mapGenerator/MapSettings.h"

#include <string>
#include <cstdlib>

/**
 * The MapGenerator is a utility to generate a large variaty of different worlds.
 */
class MapGenerator
{
    public:

    /**
     * Create and saves a new map.
     * @param filePath path for the ouput file
     * @param settings used to generate the random map
     */
    void Create(const std::string& filePath, const MapSettings& settings);
    
    private:
    
    /**
     * Generates a random number between min and max.
     * @param min minimum values
     * @param max maximum value
     */
    double DRand(const double min, const double max)
    {
        return min + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX/(max - min)));
    }
};

#endif // MapGenerator_h__
