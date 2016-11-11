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

#pragma once

#include "mapGenerator/MapHeader.h"
#include "mapGenerator/Vertex.h"

/**
 * Data type for reading, writing and generating maps.
 */
struct Map
{
    /**
     * Name of the map.
     */
    char name[20];

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
     * X-positions for all headquaters. 0xFF if the player is not available.
     */
    uint16_t HQx[7];

    /**
     * Y-positions for all headquaters. 0xFF if the player is not available.
     */
    uint16_t HQy[7];
    
    /**
     * Name of the map author.
     */
    char author[20];
    
    /**
     * Header data for the map.
     */
    MapHeader header[250];
    
    /**
     * The actual map data per vertex.
     */
    struct Vertex* vertex;
};

#endif // Map_h__
