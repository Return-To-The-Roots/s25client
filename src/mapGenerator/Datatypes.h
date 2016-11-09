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

#ifndef Datatypes_h__
#define Datatypes_h__

#pragma once

#include <stdio.h>

struct mgMapHeader
{
    uint8_t type;     //land or water (snow, swamp and lava are not counted)
    uint16_t x;
    uint16_t y;
    uint32_t area;    //number of vertices this area has
};

struct mgPoint
{
    int z;              // height value of the point
    uint8_t rsuTexture; // texture data
    uint8_t usdTexture;
    uint8_t road;
    uint8_t objectType;
    uint8_t objectInfo;
    uint8_t animal;
    uint8_t unknown1;
    uint8_t build;
    uint8_t unknown2;
    uint8_t unknown3;
    uint8_t resource;
    uint8_t shading;
    uint8_t unknown5;
};


struct mgMap
{
    char name[20];
    uint16_t height;
    uint16_t height_old;
    uint16_t width;
    uint16_t width_old;
    uint8_t type;
    uint8_t player;
    uint16_t HQx[7];
    uint16_t HQy[7];
    char author[20];
    mgMapHeader header[250];
    struct mgPoint* vertex;
};

#endif // Datatypes_h__
