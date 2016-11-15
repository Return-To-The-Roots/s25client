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

#ifndef IntPair
#define IntPair std::pair<uint8_t, uint8_t>
#endif

#include <utility>

class ObjectGenerator
{
    public:
    
    static IntPair CreateTexture(int id)
    {
        return IntPair(id, id);
    }
    
    static bool IsTexture(const IntPair& texture, int id)
    {
        return texture.first == id || texture.second == id;
    }
    
    static IntPair CreateEmpty()
    {
        return IntPair(0x00, 0x00);
    }
    
    static IntPair CreateHeadquarter(int i)
    {
        return IntPair(i, 0x80);
    }

    static bool IsEmpty(const IntPair& object)
    {
        return object.first == 0x00 && object.second == 0x00;
    }
    
    static uint8_t CreateDuck()
    {
        return 0x05;
    }
    
    static uint8_t CreateSheep()
    {
        return 0x06;
    }
    
    static uint8_t CreateRandomForestAnimal()
    {
        const int animal = rand() % 4;
        switch (animal)
        {
            case 0: return 0x01; // rabbit
            case 1: return 0x02; // fox
            case 2: return 0x03; // stag
            default: return 0x04; // roe
        }
    }
    
    static uint8_t CreateRandomResource()
    {
        const int rnd = rand() % 100;
        int resource = 0x00;
        if (rnd <= 9)       resource = 0x51; // 9% gold
        else if (rnd <= 45) resource = 0x49; // 36% iron
        else if (rnd <= 85) resource = 0x41; // 40% coal
        else                resource = 0x59; // 15% granite
        return resource + rand() % 7;
    }
    
    static uint8_t CreateRandomAnimal()
    {
        const int animal = rand() % 5;
        switch (animal)
        {
            case 0: return 0x01; // rabbit
            case 1: return 0x02; // fox
            case 2: return 0x03; // stag
            case 3: return 0x04; // roe
            default: return 0x06; // sheep
        }
    }
    
    static bool IsTree(const IntPair& object)
    {
        return object.second == 0xC5 || object.second == 0xC4;
    }
    
    static IntPair CreateRandomTree()
    {        
        IntPair tree;
        
        switch (rand() % 3)
        {
            case 0: tree.first = 0x30 + rand() % 8; break;
            case 1: tree.first = 0x70 + rand() % 8; break;
            case 2: tree.first = 0xB0 + rand() % 8; break;
        }
        tree.second = 0xC4;
        return tree;
    }
    
    static IntPair CreateRandomPalm()
    {
        return (rand() % 2 == 0) ? IntPair(0x30 + rand() % 8, 0xC5) : IntPair(0xF0 + rand() % 8, 0xC4);
    }
    
    static IntPair CreateRandomMixedTree()
    {
        return (rand() % 2 == 0) ? CreateRandomTree() : CreateRandomPalm();
    }
    
    static IntPair CreateRandomStone()
    {
        return IntPair(0x01 + rand() % 6, 0xCC + rand() % 2);
    }
};

#endif // ObjectGenerator_h__
