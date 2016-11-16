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

#ifndef ObjectGenerator_h__
#define ObjectGenerator_h__

#include <utility>
#include "stdint.h"
#include "gameTypes/MapTypes.h"

typedef std::pair<uint8_t, uint8_t> IntPair;

class ObjectGenerator
{
    public:
    
    /**
     * Creates a new texture for the specified terrain type.
     * @param harbor whether or not enable harbor placement on the texture. To enable the player to
     *      place a harbor at the position of the texture, it need to be close to water. Also keep 
     *      in mind, only terrain types which allow buildings also support harbor placement.
     * @return the new texture, including two triangles for right-side-up and right-side-down
     */
    static IntPair CreateTexture(TerrainType terrain, const bool harbor = false);
    
    static bool IsTexture(const IntPair& texture, TerrainType terrain);
    
    static IntPair CreateEmpty();
    
    static IntPair CreateHeadquarter(const int i);

    static bool IsEmpty(const IntPair& object);
    
    static uint8_t CreateDuck();
    
    static uint8_t CreateSheep();
    
    static uint8_t CreateRandomForestAnimal();
    
    static uint8_t CreateRandomResource();
    
    static uint8_t CreateRandomAnimal();
    
    static bool IsTree(const IntPair& object);
    
    static IntPair CreateRandomTree();
    
    static IntPair CreateRandomPalm();
    
    static IntPair CreateRandomMixedTree();
    
    static IntPair CreateRandomStone();
    
    static uint8_t GetTextureId(TerrainType terrain);
    
    static bool IsHarborAllowed(TerrainType terrain);
};

#endif // ObjectGenerator_h__
