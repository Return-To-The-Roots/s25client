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

#pragma once

#include <string>
#include "mapGenerator/Style.h"
#include "mapGenerator/MapSettings.h"

/**
 * The MapGenerator is a utility to generate a large variaty of different worlds.
 */
class MapGenerator
{
    public:

    /**
     * Create and saves a new map.
     * @param filePath path for the ouput file
     * @param style map style
     * @param settings used to generate the random map
     */
    void Create(const std::string& filePath, Style style, const MapSettings& settings);
};

#endif // MapGenerator_h__
