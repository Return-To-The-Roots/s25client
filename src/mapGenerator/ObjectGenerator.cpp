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
#include "libsiedler2/src/enumTypes.h"

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
    uint8_t textureId = harbor && IsHarborAllowed(terrain)
        ? TerrainData::GetTextureIdentifier(terrain) | HARBOR_MASK
        : TerrainData::GetTextureIdentifier(terrain);
    
    map->textureRsu[index] = textureId;
    map->textureLsd[index] = textureId;
}
    
bool ObjectGenerator::IsTexture(Map* map, const int index, TerrainType terrain)
{
    return map->textureRsu[index] == TerrainData::GetTextureIdentifier(terrain)
        || map->textureLsd[index] == TerrainData::GetTextureIdentifier(terrain);
}
    
void ObjectGenerator::CreateEmpty(Map* map, const int index)
{
    map->objectType[index] = OT_Empty;
    map->objectInfo[index] = OI_Empty;
}
    
void ObjectGenerator::CreateHeadquarter(Map* map, const int index, const unsigned int i)
{
    map->objectType[index] = i;
    map->objectInfo[index] = OI_HeadquarterMask;
}

bool ObjectGenerator::IsEmpty(Map* map, const int index)
{
    return (map->objectType[index] == OT_Empty &&
            map->objectInfo[index] == OI_Empty);
}
    
uint8_t ObjectGenerator::CreateDuck(const int likelyhood)
{
    return Rand(100) < likelyhood ? A_Duck : A_None;
}
    
uint8_t ObjectGenerator::CreateSheep(const int likelyhood)
{
    return (Rand(100) < likelyhood) ? A_Sheep : A_None;
}
    
uint8_t ObjectGenerator::CreateRandomForestAnimal(const int likelyhood)
{
    if (Rand(100) >= likelyhood)
    {
        return A_None;
    }
    
    switch (Rand(5))
    {
        case 0:
            return A_Rabbit;
        case 1:
            return A_Fox;
        case 2:
            return A_Stag;
        case 3:
            return A_Deer;
        default:
            return A_Deer2;
    }
}

uint8_t ObjectGenerator::CreateRandomAnimal(const int likelyhood)
{
    if (Rand(100) >= likelyhood)
    {
        return A_None;
    }

    switch (Rand(7))
    {
        case 0:
            return A_Rabbit;
        case 1:
            return A_Fox;
        case 2:
            return A_Stag;
        case 3:
            return A_Deer;
        case 4:
            return A_Sheep;
        case 5:
            return A_Donkey;
        default:
            return A_Deer2;
    }
}

uint8_t ObjectGenerator::CreateRandomResource(const unsigned int ratioGold,
                                              const unsigned int ratioIron,
                                              const unsigned int ratioCoal,
                                              const unsigned int ratioGranite)
{
    const int rnd = Rand(100);
    if (rnd <= ratioGold)
        return R_Gold + Rand(8);
    else if (rnd <= ratioGold + ratioIron)
        return R_Iron + Rand(8);
    else if (rnd <= ratioGold + ratioIron + ratioCoal)
        return R_Coal + Rand(8);
    else if (rnd <= ratioGold + ratioIron + ratioCoal + ratioGranite)
        return R_Granite + Rand(8);
    else
        return R_None;
}


bool ObjectGenerator::IsTree(Map* map, const int index)
{
    return map->objectInfo[index] == OI_TreeOrPalm ||
           map->objectInfo[index] == OI_Palm;
}
    
void ObjectGenerator::CreateRandomTree(Map* map, const int index)
{
    switch (Rand(3))
    {
        case 0:
            map->objectType[index] = OT_Tree1_Begin + Rand(OT_Tree1_End - OT_Tree1_Begin + 1);
            break;
        case 1:
            map->objectType[index] = OT_Tree2_Begin + Rand(OT_Tree2_End - OT_Tree2_Begin + 1);
            break;
        case 2:
            map->objectType[index] = OT_TreeOrPalm_Begin + Rand(OT_TreeOrPalm_End - OT_TreeOrPalm_Begin + 1);
            break;
    }
    map->objectInfo[index] = OI_TreeOrPalm;
}
    
void ObjectGenerator::CreateRandomPalm(Map* map, const int index)
{
    if (Rand(2) == 0)
    {
        map->objectType[index] = OT_TreeOrPalm_Begin + Rand(OT_TreeOrPalm_End - OT_TreeOrPalm_Begin + 1);
        map->objectInfo[index] = OI_Palm;
    }
    else
    {
        map->objectType[index] = OT_Palm_Begin + Rand(OT_Palm_End - OT_Palm_Begin + 1);
        map->objectInfo[index] = OI_TreeOrPalm;
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
    map->objectType[index] = OT_Stone_Begin + Rand(OT_Stone_End - OT_Stone_Begin + 1);
    map->objectInfo[index] = Rand(2) == 0 ? OI_Stone1 : OI_Stone2;
}

