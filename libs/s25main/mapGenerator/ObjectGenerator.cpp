// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "mapGenerator/RandomConfig.h"
#include "gameData/TerrainDesc.h"
#include "libsiedler2/enumTypes.h"

bool ObjectGenerator::IsHarborAllowed(DescIdx<TerrainDesc> terrain)
{
    return config.worldDesc.get(terrain).Is(ETerrain::Buildable)
           && config.worldDesc.get(terrain).kind == TerrainKind::LAND;
}

void ObjectGenerator::CreateTexture(Map& map, int index, DescIdx<TerrainDesc> terrain, bool harbor)
{
    uint8_t textureId = config.worldDesc.get(terrain).s2Id;
    if(harbor && IsHarborAllowed(terrain))
        textureId |= libsiedler2::HARBOR_MASK;
    map.textureRsu[index] = textureId;
    map.textureLsd[index] = textureId;
}

bool ObjectGenerator::IsTexture(const Map& map, int index, DescIdx<TerrainDesc> terrain)
{
    return map.textureRsu[index] == config.worldDesc.get(terrain).s2Id
           || map.textureLsd[index] == config.worldDesc.get(terrain).s2Id;
}

void ObjectGenerator::CreateEmpty(Map& map, int index)
{
    map.objectType[index] = libsiedler2::OT_Empty;
    map.objectInfo[index] = libsiedler2::OI_Empty;
}

void ObjectGenerator::CreateHeadquarter(Map& map, int index, unsigned i)
{
    map.objectType[index] = i;
    map.objectInfo[index] = libsiedler2::OI_HeadquarterMask;
}

bool ObjectGenerator::IsEmpty(const Map& map, int index)
{
    return (map.objectType[index] == libsiedler2::OT_Empty && map.objectInfo[index] == libsiedler2::OI_Empty);
}

libsiedler2::Animal ObjectGenerator::CreateDuck(int likelihood)
{
    return config.Rand(100) < likelihood ? libsiedler2::Animal::Duck : libsiedler2::Animal::None;
}

libsiedler2::Animal ObjectGenerator::CreateSheep(int likelihood)
{
    return config.Rand(100) < likelihood ? libsiedler2::Animal::Sheep : libsiedler2::Animal::None;
}

libsiedler2::Animal ObjectGenerator::CreateRandomForestAnimal(int likelihood)
{
    if(config.Rand(100) >= likelihood)
    {
        return libsiedler2::Animal::None;
    }

    switch(config.Rand(5))
    {
        case 0: return libsiedler2::Animal::Rabbit;
        case 1: return libsiedler2::Animal::Fox;
        case 2: return libsiedler2::Animal::Stag;
        case 3: return libsiedler2::Animal::Deer;
        default: return libsiedler2::Animal::Deer2;
    }
}

libsiedler2::Animal ObjectGenerator::CreateRandomAnimal(int likelihood)
{
    if(config.Rand(100) >= likelihood)
    {
        return libsiedler2::Animal::None;
    }

    switch(config.Rand(7))
    {
        case 0: return libsiedler2::Animal::Rabbit;
        case 1: return libsiedler2::Animal::Fox;
        case 2: return libsiedler2::Animal::Stag;
        case 3: return libsiedler2::Animal::Deer;
        case 4: return libsiedler2::Animal::Sheep;
        case 5: return libsiedler2::Animal::Donkey;
        default: return libsiedler2::Animal::Deer2;
    }
}

uint8_t ObjectGenerator::CreateRandomResource(unsigned ratioGold, unsigned ratioIron, unsigned ratioCoal,
                                              unsigned ratioGranite)
{
    auto rnd = (unsigned)config.Rand(ratioGold + ratioIron + ratioCoal + ratioGranite);

    if(rnd < ratioGold)
        return libsiedler2::R_Gold + config.Rand(8);
    else if(rnd < ratioGold + ratioIron)
        return libsiedler2::R_Iron + config.Rand(8);
    else if(rnd < ratioGold + ratioIron + ratioCoal)
        return libsiedler2::R_Coal + config.Rand(8);
    else
        return libsiedler2::R_Granite + config.Rand(8);
}

bool ObjectGenerator::IsTree(const Map& map, int index)
{
    return map.objectInfo[index] == libsiedler2::OI_TreeOrPalm || map.objectInfo[index] == libsiedler2::OI_Palm;
}

void ObjectGenerator::CreateRandomTree(Map& map, int index)
{
    switch(config.Rand(3))
    {
        case 0: map.objectType[index] = config.Rand(libsiedler2::OT_Tree1_Begin, libsiedler2::OT_Tree1_End + 1); break;
        case 1: map.objectType[index] = config.Rand(libsiedler2::OT_Tree2_Begin, libsiedler2::OT_Tree2_End + 1); break;
        case 2:
            map.objectType[index] = config.Rand(libsiedler2::OT_TreeOrPalm_Begin, libsiedler2::OT_TreeOrPalm_End + 1);
            break;
    }
    map.objectInfo[index] = libsiedler2::OI_TreeOrPalm;
}

void ObjectGenerator::CreateRandomPalm(Map& map, int index)
{
    if(config.Rand(2) == 0)
    {
        map.objectType[index] = config.Rand(libsiedler2::OT_TreeOrPalm_Begin, libsiedler2::OT_TreeOrPalm_End + 1);
        map.objectInfo[index] = libsiedler2::OI_Palm;
    } else
    {
        map.objectType[index] = config.Rand(libsiedler2::OT_Palm_Begin, libsiedler2::OT_Palm_End + 1);
        map.objectInfo[index] = libsiedler2::OI_TreeOrPalm;
    }
}

void ObjectGenerator::CreateRandomMixedTree(Map& map, int index)
{
    if(config.Rand(2) == 0)
    {
        CreateRandomTree(map, index);
    } else
    {
        CreateRandomPalm(map, index);
    }
}

void ObjectGenerator::CreateRandomStone(Map& map, int index)
{
    map.objectType[index] = config.Rand(libsiedler2::OT_Stone_Begin, libsiedler2::OT_Stone_End + 1);
    map.objectInfo[index] = config.Rand(2) == 0 ? libsiedler2::OI_Stone1 : libsiedler2::OI_Stone2;
}
