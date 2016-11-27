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

#ifndef Map_h__
#define Map_h__

#include "mapGenerator/Vec2.h"

#include "libsiedler2/src/archives.h"

#include <vector>
#include <string>

using namespace libsiedler2;
using namespace std;

typedef vector<unsigned char> VecUChar;
typedef vector<pair<uint8_t, uint8_t> > VecPair;

/**
 * Data type for reading, writing and generating maps.
 */
struct Map
{
    /**
     * Creates a new instance of map with initial player positions set to 0xFF (= not set).
     */
    Map();

    /**
     * Name of the map.
     */
    std::string name;

    /**
     * Height of the map in vertices.
     */
    uint16_t height;

    /**
     * Width of the map in vertices.
     */
    uint16_t width;

    /**
     * Landscape type of the map.
     */
    uint8_t type;
    
    /**
     * Number of players.
     */
    uint8_t players;
    
    /**
     * Positions of the players' headquarters.
     */
    Vec2 positions[7];
    
    /**
     * Name of the map author.
     */
    std::string author;
    
    /**
     * Height for each vertex of the map.
     */
    VecUChar z;
    
    /**
     * Road values for each vertex of the map.
     */
    VecUChar road;
    
    /**
     * Animal values for each vertex of the map.
     */
    VecUChar animal;
    
    /**
     * Unknown value 1 for each vertex of the map.
     */
    VecUChar unknown1;

    /**
     * Build values for each vertex of the map.
     */
    VecUChar build;
    
    /**
     * Unknown value 2 for each vertex of the map.
     */
    VecUChar unknown2;
    
    /**
     * Unknown value 3 for each vertex of the map.
     */
    VecUChar unknown3;

    /**
     * Resource values for each vertex of the map.
     */
    VecUChar resource;

    /**
     * Shading values for each vertex of the map.
     */
    VecUChar shading;
    
    /**
     * Unknown value 5 for each vertex of the map.
     */
    VecUChar unknown5;
    
    /**
     * Right-side-down texture values for each vertex of the map.
     */
    VecUChar textureRsu;
    
    /**
     * Left-side-down texture values for each vertex of the map.
     */
    VecUChar textureLsd;
    
    /**
     * Object type values for each vertex of the map.
     */
    VecUChar objectType;

    /**
     * Object info values for each vertex of the map.
     */
    VecUChar objectInfo;
    
    /**
     * Creates a new archiv for this map. 
     * @return a new archiv containing the information of this map
     */
    ArchivInfo* CreateArchiv();
};

#endif // Map_h__
