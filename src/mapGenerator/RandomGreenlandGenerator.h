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

#ifndef RandomGreenlandGenerator_h__
#define RandomGreenlandGenerator_h__

#include "mapGenerator/RandomMapGenerator.h"
#include "mapGenerator/AreaDesc.h"

/**
 * Random greenland map generator.
 */
class RandomGreenlandGenerator : public RandomMapGenerator
{
    public:

    /**
     * Creates a new RandomGreenlandGenerator.
     */
    RandomGreenlandGenerator() : RandomMapGenerator(false)
    {
        // cx, cy min, max, pHill, pTree, pStone, minZ, maxZ, minPlayerDist, maxPlayerDist
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 8.0,  14, 7, 5, 10, 15));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 0.2,   0, 0, 0, 19, 15));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 0.05,  0, 0, 0, 23, 15));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 100.0, 0, 0, 7,  7,  0, 4));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 100.0, 8, 0, 5, 10,  4, 15));
    }
 };

#endif // RandomGreenlandGenerator_h__
