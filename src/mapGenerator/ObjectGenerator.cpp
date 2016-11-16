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

#include "mapGenerator/ObjectGenerator.h"
#include <cstdlib>

uint8_t ObjectGenerator::GetTextureId(TerrainType terrain)
{
    switch (terrain)
    {
        case TT_SNOW:           return 0x02;
        case TT_LAVA:           return 0x10;
        case TT_LAVA2:          return 0x10;
        case TT_LAVA3:          return 0x10;
        case TT_LAVA4:          return 0x10;
        case TT_WATER:          return 0x05;
        case TT_WATER_NOSHIP:   return 0x06;
        case TT_DESERT:         return 0x04;
        case TT_MOUNTAIN1:      return 0x01;
        case TT_MOUNTAIN2:      return 0x0B;
        case TT_MOUNTAIN3:      return 0x0C;
        case TT_MOUNTAIN4:      return 0x0D;
        case TT_SWAMPLAND:      return 0x03;
        case TT_BUILDABLE_WATER:return 0x13;
        case TT_STEPPE:         return 0x0E;
        case TT_SAVANNAH:       return 0x00;
        case TT_MEADOW1:        return 0x08;
        case TT_MEADOW2:        return 0x09;
        case TT_MEADOW3:        return 0x0A;
        case TT_MEADOW_FLOWERS: return 0x0F;
        case TT_MOUNTAINMEADOW: return 0x12;
        default:                return 0x08;
    }
}

bool ObjectGenerator::IsHarborAllowed(TerrainType terrain)
{
    switch (terrain)
    {
        case TT_STEPPE:
        case TT_SAVANNAH:
        case TT_MEADOW1:
        case TT_MEADOW2:
        case TT_MEADOW3:
        case TT_MEADOW_FLOWERS:
        case TT_MOUNTAINMEADOW:
            return true;
        default:
            return false;
    }
}

IntPair ObjectGenerator::CreateTexture(TerrainType terrain, const bool harbor)
{
    u_int8_t textureId = harbor && IsHarborAllowed(terrain) ? GetTextureId(terrain) | 0x40 : GetTextureId(terrain);
    return IntPair(textureId, textureId);
}
    
bool ObjectGenerator::IsTexture(const IntPair& texture, TerrainType terrain)
{
    u_int8_t textureId = GetTextureId(terrain);
    return texture.first == textureId || texture.second == textureId;
}
    
IntPair ObjectGenerator::CreateEmpty()
{
    return IntPair(0x00, 0x00);
}
    
IntPair ObjectGenerator::CreateHeadquarter(const int i)
{
    return IntPair(i, 0x80);
}

bool ObjectGenerator::IsEmpty(const IntPair& object)
{
    return object.first == 0x00 && object.second == 0x00;
}
    
uint8_t ObjectGenerator::CreateDuck()
{
    return 0x05;
}
    
uint8_t ObjectGenerator::CreateSheep()
{
    return 0x06;
}
    
uint8_t ObjectGenerator::CreateRandomForestAnimal()
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

uint8_t ObjectGenerator::CreateRandomResource()
{
    const int rnd = rand() % 100;
    int resource = 0x00;
    if (rnd <= 9)       resource = 0x51; // 9% gold
    else if (rnd <= 45) resource = 0x49; // 36% iron
    else if (rnd <= 85) resource = 0x41; // 40% coal
    else                resource = 0x59; // 15% granite
    return resource + rand() % 7;
}
    
uint8_t ObjectGenerator::CreateRandomAnimal()
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

bool ObjectGenerator::IsTree(const IntPair& object)
{
    return object.second == 0xC5 || object.second == 0xC4;
}
    
IntPair ObjectGenerator::CreateRandomTree()
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
    
IntPair ObjectGenerator::CreateRandomPalm()
{
    return (rand() % 2 == 0) ? IntPair(0x30 + rand() % 8, 0xC5) : IntPair(0xF0 + rand() % 8, 0xC4);
}
    
IntPair ObjectGenerator::CreateRandomMixedTree()
{
    return (rand() % 2 == 0) ? CreateRandomTree() : CreateRandomPalm();
}
    
IntPair ObjectGenerator::CreateRandomStone()
{
    return IntPair(0x01 + rand() % 6, 0xCC + rand() % 2);
}

