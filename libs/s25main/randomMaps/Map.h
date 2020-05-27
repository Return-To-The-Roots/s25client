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

#include "randomMaps/terrain/TextureMapping.h"
#include "gameTypes/MapCoordinates.h"
#include "libsiedler2/archives.h"
#include <string>
#include <vector>
#include <set>

/**
 * Data type for reading, writing and generating maps.
 */
struct Map
{
    /**
     * Create a new map of size 0x0.
     */
    Map();

    /**
     * Creates a new, empty map with the specified width and height.
     * @param size
     * @param name name of the map
     * @param author author of the map
     */
    Map(const MapExtent& size, const std::string& name, const std::string& author);

    /**
     * size of the map in vertices.
     */
    MapExtent size;

    /**
     * Name of the map.
     */
    std::string name;

    /**
     * Name of the map author.
     */
    std::string author;

    /**
     * Landscape type of the map.
     */
    uint8_t type;

    /**
     * Number of players.
     */
    uint8_t numPlayers;

    /**
     * Positions of the players' headquarters.
     */
    std::vector<MapPoint> hqPositions;
    
    /**
     * Indices for right-side-up texture harbor positions.
     */
    std::set<int> harborsRsu;

    /**
     * Indices for left-side-down texture harbor positions.
     */
    std::set<int> harborsLsd;

    /**
     * Height for each vertex of the map.
     */
    std::vector<unsigned char> z;

    /**
     * Road values for each vertex of the map.
     */
    std::vector<unsigned char> road;

    /**
     * Animal values for each vertex of the map.
     */
    std::vector<unsigned char> animal;

    /**
     * Unknown value 1 for each vertex of the map.
     */
    std::vector<unsigned char> unknown1;

    /**
     * Build values for each vertex of the map.
     */
    std::vector<unsigned char> build;

    /**
     * Unknown value 2 for each vertex of the map.
     */
    std::vector<unsigned char> unknown2;

    /**
     * Unknown value 3 for each vertex of the map.
     */
    std::vector<unsigned char> unknown3;

    /**
     * Resource values for each vertex of the map.
     */
    std::vector<unsigned char> resource;

    /**
     * Shading values for each vertex of the map.
     */
    std::vector<unsigned char> shading;

    /**
     * Unknown value 5 for each vertex of the map.
     */
    std::vector<unsigned char> unknown5;

    /**
     * Right-side-down texture values for each vertex of the map.
     */
    std::vector<TextureType> textureRsu;

    /**
     * Left-side-down texture values for each vertex of the map.
     */
    std::vector<TextureType> textureLsd;

    /**
     * Object type values for each vertex of the map.
     */
    std::vector<unsigned char> objectType;

    /**
     * Object info values for each vertex of the map.
     */
    std::vector<unsigned char> objectInfo;

    /**
     * Creates a new archiv for this map.
     * @return a new archiv containing the information of this map
     */
    libsiedler2::Archiv* CreateArchiv(TextureMapping& mapping);
};
