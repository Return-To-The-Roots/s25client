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

#ifndef RandomIslandsGenerator_h__
#define RandomIslandsGenerator_h__

#include "mapGenerator/RandomMapGenerator.h"
#include "mapGenerator/AreaDesc.h"

/**
 * Random islands map generator.
 */
class RandomIslandsGenerator : public RandomMapGenerator
{
    public:

    /**
     * Creates a new RandomIslandsGenerator.
     */
    RandomIslandsGenerator() : RandomMapGenerator(false)
    {
        _textures[0]    = TT_WATER;
        _textures[1]    = TT_WATER;
        _textures[2]    = TT_WATER;
        _textures[3]    = TT_WATER;
        _textures[4]    = TT_DESERT;
        _textures[5]    = TT_STEPPE;
        _textures[6]    = TT_SAVANNAH;
        _textures[7]    = TT_MEADOW1;
        _textures[8]    = TT_MEADOW_FLOWERS;
        _textures[9]    = TT_MEADOW2;
        _textures[10]   = TT_MEADOW2;
        _textures[11]   = TT_MOUNTAINMEADOW;
        _textures[12]   = TT_MOUNTAIN1;
        _textures[13]   = TT_MOUNTAIN1;
        _textures[14]   = TT_MOUNTAIN1;
        _textures[15]   = TT_LAVA;
        _textures[16]   = TT_LAVA;
        _textures[17]   = TT_LAVA;
        _textures[18]   = TT_LAVA;
        _textures[19]   = TT_LAVA;
        _textures[20]   = TT_LAVA;
        _textures[21]   = TT_LAVA;
        _textures[22]   = TT_LAVA;
        _textures[23]   = TT_LAVA;
        _textures[24]   = TT_LAVA;
        
        // cx, cy min, max, pHill, pTree, pStone, minZ, maxZ, minPlayerDist, maxPlayerDist
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 0.06, 14, 7, 0, 18, 15));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 5.0,  14, 7, 10, 20, 21, 22));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 100.0,  0, 0, 7,  7,  0, 4));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 100.0,  8, 0, 5, 10,  4, 15));        
    }
 };

#endif // RandomIslandsGenerator_h__
