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

#include "rttrDefines.h" // IWYU pragma: keep
#include "mapGenerator/RandomConfig.h"
#include "RttrConfig.h"
#include "files.h"
#include "lua/GameDataLoader.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include <boost/bind.hpp>
#include <boost/random/uniform_real_distribution.hpp>
// Fix stupid conversion warning in boost
#pragma warning(push)
#pragma warning(disable : 4244)
#include <boost/random/uniform_smallint.hpp>
#pragma warning(pop)
#include <ctime>
#include <stdexcept>

bool RandomConfig::Init(MapStyle mapStyle, Landscape landscape)
{
    uint64_t seed = static_cast<uint64_t>(time(NULL));
    return Init(mapStyle, landscape, seed);
}

bool RandomConfig::Init(MapStyle mapStyle, Landscape landscape, uint64_t seed)
{
    WorldDescription desc;
    GameDataLoader gdLoader(desc, RTTRCONFIG.ExpandPath(FILE_PATHS[1]) + "/world");
    if(!gdLoader.Load())
        return false;
    for(DescIdx<TerrainDesc> t(0); t.value < desc.terrain.size(); t.value++)
    {
        const TerrainDesc& ter = desc.get(t);
        if(ter.landscape == landscape)
            terrainDesc.add(ter);
    }
    rng_.seed(static_cast<UsedRNG::result_type>(seed));
    switch(boost::native_value(mapStyle))
    {
        case MapStyle::Greenland: CreateGreenland(); break;
        case MapStyle::Riverland: CreateRiverland(); break;
        case MapStyle::Ringland: CreateRingland(); break;
        case MapStyle::Migration: CreateMigration(); break;
        case MapStyle::Islands: CreateIslands(); break;
        case MapStyle::Continent: CreateContinent(); break;
        case MapStyle::Random: CreateRandom(); break;
        default: throw std::logic_error("Invalid enum value");
    }
    return true;
}

void RandomConfig::CreateGreenland()
{
    CreateDefaultTextures();

    const Point<double> center(0.5, 0.5);

    // greenland all over the map
    areas.push_back(AreaDesc(center, 0.0, 2.0, 8.0, 14, 7, 5, 10, 15));

    // small mountains and hills all over the map
    areas.push_back(AreaDesc(center, 0.0, 2.0, 0.2, 0, 0, 0, 19, 15));

    // very few large mountains all over the map
    areas.push_back(AreaDesc(center, 0.0, 2.0, 0.05, 0, 0, 0, 23, 15));

    // empty space around the players
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7, 7, 0, 4));
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10, 4, 15));
}

void RandomConfig::CreateDefaultTextures(bool snowOrLava)
{
    // Water
    std::vector<DescIdx<TerrainDesc> > terrains =
      FindAllTerrains(boost::bind(&TerrainDesc::kind, _1) == TerrainKind::WATER && boost::bind(&TerrainDesc::Is, _1, ETerrain::Shippable));
    if(!terrains.empty())
    {
        for(int i = 0; i < 4; i++)
            textures.push_back(terrains[Rand(terrains.size())]);
    }
    // Walkable land
    terrains =
      FindAllTerrains(boost::bind(&TerrainDesc::kind, _1) == TerrainKind::LAND && boost::bind(&TerrainDesc::Is, _1, ETerrain::Walkable));
    // Desert or similar
    terrains = FilterTerrains(terrains, boost::bind(&TerrainDesc::GetBQ, _1) == TerrainBQ::FLAG);
    if(!terrains.empty())
    {
        for(unsigned i = 1; i < terrains.size(); i++)
        {
            if(terrainDesc.get(terrains[0]).humidity > terrainDesc.get(terrains[i]).humidity)
                std::swap(terrains[0], terrains[i]);
        }
        textures.push_back(terrains.front());
    }
    // Buildable land ordered by humidity
    terrains =
      FindAllTerrains(boost::bind(&TerrainDesc::kind, _1) == TerrainKind::LAND && boost::bind(&TerrainDesc::Is, _1, ETerrain::Buildable));
    if(terrains.empty())
        throw std::runtime_error("No buildable land found");
    bool swapped;
    do
    {
        swapped = false;
        for(unsigned i = 1; i < terrains.size(); i++)
        {
            if(terrainDesc.get(terrains[i - 1]).humidity > terrainDesc.get(terrains[i]).humidity)
            {
                std::swap(terrains[i - 1], terrains[i]);
                swapped = true;
            }
        }
    } while(swapped);

    for(int i = 0; i < std::min<int>(6, terrains.size()); i++)
        textures.push_back(terrains[i]);

    // Buildable, humid mountain
    terrains = FindAllTerrains(boost::bind(&TerrainDesc::kind, _1) == TerrainKind::MOUNTAIN
                               && boost::bind(&TerrainDesc::Is, _1, ETerrain::Buildable));
    if(!terrains.empty())
    {
        for(unsigned i = 1; i < terrains.size(); i++)
        {
            if(terrainDesc.get(terrains[0]).humidity < terrainDesc.get(terrains[i]).humidity)
                std::swap(terrains[0], terrains[i]);
        }
        textures.push_back(terrains.front());
    }
    // Mountain
    DescIdx<TerrainDesc> t =
      FindTerrain(boost::bind(&TerrainDesc::kind, _1) == TerrainKind::MOUNTAIN && boost::bind(&TerrainDesc::Is, _1, ETerrain::Mineable));
    if(!t)
        throw std::runtime_error("No mineable mountain found");
    for(int i = 0; i < 3; i++)
        textures.push_back(t);

    if(snowOrLava)
        t = FindTerrain(boost::bind(&TerrainDesc::kind, _1) == TerrainKind::SNOW);
    else
        t = DescIdx<TerrainDesc>();
    if(!t)
        t = FindTerrain(boost::bind(&TerrainDesc::kind, _1) == TerrainKind::LAVA);
    if(!t)
        t = FindTerrain(boost::bind(&TerrainDesc::kind, _1) == TerrainKind::SNOW);
    if(!!t)
    {
        for(int i = 0; i < 10; i++)
            textures.push_back(t);
    }
}

void RandomConfig::CreateRiverland()
{
    CreateDefaultTextures();

    const Point<double> center(0.5, 0.5);

    // mix between water and greenland all over the map
    areas.push_back(AreaDesc(center, 0.0, 2.0, 2.0, 14, 7, 0, 10, 15));

    // a couple of mountains (and hills) all over the map
    areas.push_back(AreaDesc(center, 0.0, 2.0, 0.2, 0, 0, 0, 19, 15));

    // very few, very large mountains all over the map
    areas.push_back(AreaDesc(center, 0.0, 2.0, 0.05, 0, 0, 0, 23, 15));

    // empty space around the players
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7, 7, 0, 4));
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10, 4, 15));
}

void RandomConfig::CreateRingland()
{
    CreateDefaultTextures();

    const double rMin = DRand(0.2, 0.5);
    const double rMax = DRand(rMin + 0.1, 0.9);
    const double rMiddle = rMin + (rMax - rMin) / 2;
    const Point<double> center(0.5, 0.5);

    // ring formed land (coastal and greenland)
    areas.push_back(AreaDesc(center, rMin, rMax, 8.0, 14, 7, 5, 10, 15));

    // ring formed mountain land in the middle of the greenland ring
    areas.push_back(AreaDesc(center, rMiddle - 0.05, rMiddle + 0.05, 1.0, 0, 0, 0, 20, 15));

    // empty space around the players
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7, 7, 0, 4));
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10, 4, 15));
}

void RandomConfig::CreateMigration()
{
    CreateDefaultTextures();

    const Point<double> center(0.5, 0.5);

    // inner island with large mountains
    areas.push_back(AreaDesc(center, 0.0, 0.2, 2.0, 14, 7, 0, 20, 20));

    // greenland around the large mountains
    areas.push_back(AreaDesc(center, 0.2, 0.4, 10.0, 14, 7, 5, 10, 20));

    // empty space around the players
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7, 7, 0, 4));
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10, 4, 15));
}

void RandomConfig::CreateIslands()
{
    CreateDefaultTextures(false);

    const Point<double> center(0.5, 0.5);

    // little islands all over the map
    areas.push_back(AreaDesc(center, 0.0, 2.0, 0.06, 14, 7, 0, 18, 15));

    // mountain islands around each player
    areas.push_back(AreaDesc(center, 0.0, 2.0, 5.0, 14, 7, 10, 20, 21, 22));

    // empty space around the players
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7, 7, 0, 4));
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10, 4, 15));
}

void RandomConfig::CreateContinent()
{
    CreateDefaultTextures();

    const Point<double> center(0.5, 0.5);

    // greenland all over the map, apart from the water at the edges
    areas.push_back(AreaDesc(center, 0.0, 0.9, 8.0, 14, 7, 5, 10, 15));

    // small mountains and hills all over the greenland area
    areas.push_back(AreaDesc(center, 0.0, 0.9, 0.2, 0, 0, 0, 18, 15));

    // few very high mountains and hills all over the greenland area
    areas.push_back(AreaDesc(center, 0.0, 0.9, 0.05, 0, 0, 0, 23, 15));

    // empty space around the players
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7, 7, 0, 4));
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10, 4, 15));
}

void RandomConfig::CreateRandom()
{
    CreateDefaultTextures();

    const double p1 = DRand(0.0, 0.4);
    const double p2 = DRand(p1, p1 + 1.4);
    const double p3 = DRand(p2, p2 + 1.0);
    const double pHill = DRand(1.5, 5.0);
    const int minHill = Rand(5);
    const Point<double> center(0.5, 0.5);

    // random inner area with large mountains
    areas.push_back(AreaDesc(center, 0.0, p1, 1.0, 4, 7, 0, 23, 15));

    // random middle area with greenland
    areas.push_back(AreaDesc(center, p1, p2, pHill, 18, 5, minHill, 10, 15));

    // random mountains in the greenland area
    areas.push_back(AreaDesc(center, p1, p2, 0.5, 0, 0, 0, 17, 18));

    // random islands in the remaining water
    areas.push_back(AreaDesc(center, p2, p3, 0.1, 15, 5, 0, 7, 15));

    // empty space around the players
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7, 7, 0, 4));
    areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10, 4, 15));
}

int RandomConfig::Rand(const int max)
{
    return Rand(0, max);
}

int RandomConfig::Rand(const int min, const int max)
{
    RTTR_Assert(max > min);
    boost::random::uniform_smallint<> distr(min, max - 1);
    return distr(rng_);
}

double RandomConfig::DRand(const double min, const double max)
{
    boost::random::uniform_real_distribution<double> distr(min, max);
    return distr(rng_);
}

const TerrainDesc& RandomConfig::GetTerrainByS2Id(uint8_t s2Id) const
{
    return terrainDesc.get(FindTerrain(boost::bind(&TerrainDesc::s2Id, _1) == s2Id));
}
