// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "mapGenerator/AreaDesc.h"
#include "mapGenerator/MapStyle.h"
#include "random/XorShift.h"
#include "gameData/DescIdx.h"
#include "gameData/WorldDescription.h"
#include <vector>

struct LandscapeDesc;

/**
 * Random map configuration.
 */
class RandomConfig
{
public:
    bool Init(MapStyle mapStyle, DescIdx<LandscapeDesc> landscape);
    bool Init(MapStyle mapStyle, DescIdx<LandscapeDesc> landscape, uint64_t seed);

    WorldDescription worldDesc;
    std::vector<DescIdx<TerrainDesc>> landscapeTerrains;

    /**
     * Description of different areas to use for random map generation.
     */
    std::vector<AreaDesc> areas;

    /**
     * Mapping of elevation (= height values) to terrain. The number of elements defines the
     * maximum height value used to generate the random map.
     * For example, following statement will assign water textures to landscape with the
     * height of "0": textures[0] = TT_WATER;
     */
    std::vector<DescIdx<TerrainDesc>> textures;

    /**
     * Generates a random number between 0 and max-1.
     * @param max maximum value
     * @return a new random number
     */
    int Rand(int max);

    /**
     * Generates a random number between min and max-1.
     * @param min minimum value
     * @param max maximum value
     * @return a new random number
     */
    int Rand(int min, int max);

    /**
     * Generates a random number between min and max.
     * @param min minimum value
     * @param max maximum value
     * @return a new random number
     */
    double DRand(double min, double max);

    const TerrainDesc& GetTerrainByS2Id(uint8_t s2Id) const;

    template<class T_Predicate>
    DescIdx<TerrainDesc> FindTerrain(T_Predicate predicate) const;
    template<class T_Predicate>
    std::vector<DescIdx<TerrainDesc>> FindAllTerrains(T_Predicate predicate) const;
    template<class T_Predicate>
    std::vector<DescIdx<TerrainDesc>> FilterTerrains(const std::vector<DescIdx<TerrainDesc>>& inTerrains,
                                                     T_Predicate predicate) const;

private:
    void CreateGreenland();

    void CreateDefaultTextures(bool snowOrLava = true);

    void CreateRiverland();
    void CreateRingland();
    void CreateMigration();
    void CreateIslands();
    void CreateContinent();
    void CreateRandom();

    using UsedRNG = XorShift;
    UsedRNG rng_;
};

template<class T_Predicate>
inline DescIdx<TerrainDesc> RandomConfig::FindTerrain(T_Predicate predicate) const
{
    for(DescIdx<TerrainDesc> t : landscapeTerrains)
    {
        if(predicate(worldDesc.get(t)))
            return t;
    }
    return DescIdx<TerrainDesc>();
}

template<class T_Predicate>
inline std::vector<DescIdx<TerrainDesc>> RandomConfig::FindAllTerrains(T_Predicate predicate) const
{
    std::vector<DescIdx<TerrainDesc>> result;
    for(DescIdx<TerrainDesc> t : landscapeTerrains)
        result.push_back(t);
    return FilterTerrains(result, predicate);
}

template<class T_Predicate>
inline std::vector<DescIdx<TerrainDesc>>
RandomConfig::FilterTerrains(const std::vector<DescIdx<TerrainDesc>>& inTerrains, T_Predicate predicate) const
{
    std::vector<DescIdx<TerrainDesc>> result;
    for(DescIdx<TerrainDesc> t : inTerrains)
    {
        if(predicate(worldDesc.get(t)))
            result.push_back(t);
    }
    return result;
}
