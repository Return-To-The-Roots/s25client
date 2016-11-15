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

#ifndef Vertex_h__
#define Vertex_h__

#include <utility>
#include <stdio.h>

struct Vertex
{
    int z;              // height value of the point
    uint8_t road;
    uint8_t animal;
    uint8_t unknown1;
    uint8_t build;
    uint8_t unknown2;
    uint8_t unknown3;
    uint8_t resource;
    uint8_t shading;
    uint8_t unknown5;
    std::pair<uint8_t, uint8_t> texture; // bottom left / top right
    std::pair<uint8_t, uint8_t> object; // type, info
};

#endif // Vertex_h__
