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

void ObjectGenerator::CreateTexture(Map* map, const int index, TerrainType terrain, const bool harbor)
{
    uint8_t textureId = harbor && IsHarborAllowed(terrain) ? GetTextureId(terrain) | 0x40 : GetTextureId(terrain);
    map->textureRsu[index] = textureId;
    map->textureLsd[index] = textureId;
}
    
bool ObjectGenerator::IsTexture(Map* map, const int index, TerrainType terrain)
{
    uint8_t textureId = GetTextureId(terrain);
    return map->textureRsu[index] == textureId || map->textureLsd[index] == textureId;
}
    
void ObjectGenerator::CreateEmpty(Map* map, const int index)
{
    map->objectType[index] = 0x00;
    map->objectInfo[index] = 0x00;
}
    
void ObjectGenerator::CreateHeadquarter(Map* map, const int index, const int i)
{
    map->objectType[index] = (unsigned int)i;
    map->objectInfo[index] = 0x80;
}

bool ObjectGenerator::IsEmpty(Map* map, const int index)
{
    return (map->objectType[index] == 0x00 && map->objectInfo[index] == 0x00);
}
    
uint8_t ObjectGenerator::CreateDuck(const int likelyhood)
{
    return Rand(100) < likelyhood ? 0x05 : 0x00;
}
    
uint8_t ObjectGenerator::CreateSheep(const int likelyhood)
{
    return (Rand(100) < likelyhood) ? 0x06 : 0x00;
}
    
uint8_t ObjectGenerator::CreateRandomForestAnimal(const int likelyhood)
{
    if (Rand(100) >= likelyhood) return 0x00;
    const int animal = Rand(4);
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
    const int rnd = Rand(100);
    int resource = 0x00;
    if (rnd <= 9)       resource = 0x51; // 9% gold
    else if (rnd <= 45) resource = 0x49; // 36% iron
    else if (rnd <= 85) resource = 0x41; // 40% coal
    else                resource = 0x59; // 15% granite
    return resource + Rand(7);
}
    
uint8_t ObjectGenerator::CreateRandomAnimal(const int likelyhood)
{
    if (Rand(100) >= likelyhood) return 0x00;
    const int animal = Rand(5);
    switch (animal)
    {
        case 0: return 0x01; // rabbit
        case 1: return 0x02; // fox
        case 2: return 0x03; // stag
        case 3: return 0x04; // roe
        default: return 0x06; // sheep
    }
}

bool ObjectGenerator::IsTree(Map* map, const int index)
{
    return map->objectInfo[index] == 0xC5 || map->objectInfo[index] == 0xC4;
}
    
void ObjectGenerator::CreateRandomTree(Map* map, const int index)
{
    switch (Rand(3))
    {
        case 0: map->objectType[index] = 0x30 + Rand(8); break;
        case 1: map->objectType[index] = 0x70 + Rand(8); break;
        case 2: map->objectType[index] = 0xB0 + Rand(8); break;
    }
    map->objectInfo[index] = 0xC4;
}
    
void ObjectGenerator::CreateRandomPalm(Map* map, const int index)
{
    if (Rand(2) == 0)
    {
        map->objectType[index] = 0x30 + Rand(8);
        map->objectInfo[index] = 0xC5;
    }
    else
    {
        map->objectType[index] = 0xF0 + Rand(8);
        map->objectInfo[index] = 0xC4;
    }
}
    
void ObjectGenerator::CreateRandomMixedTree(Map* map, const int index)
{
    if (Rand(2) == 0)
    {
        CreateRandomTree(map, index);
    }
    else
    {
        CreateRandomPalm(map, index);
    }
}
    
void ObjectGenerator::CreateRandomStone(Map* map, const int index)
{
    map->objectType[index] = 0x01 + Rand(6);
    map->objectInfo[index] = 0xCC + Rand(2);
}

