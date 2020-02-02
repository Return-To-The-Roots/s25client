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

#include "mapGenerator/RandomMap.h"
#include "mapGenerator/Islands.h"
#include "mapGenerator/Rivers.h"
#include "mapGenerator/Harbors.h"
#include "mapGenerator/HeadQuarters.h"
#include "mapGenerator/Resources.h"
#include "mapGenerator/Reshaper.h"
#include "mapGenerator/Textures.h"

#include "lua/GameDataLoader.h"
#include "libsiedler2/libsiedler2.h"

#include <stdexcept>
#include <iostream>

namespace rttr {
namespace mapGenerator {

void PostProcessHeightMap(Map_& map)
{
    switch (map.size.x + map.size.y)
    {
        case 128: Smooth(10, 2.0, map.z, map.size); break;
        case 256: Smooth(14, 2.0, map.z, map.size); break;
        case 512: Smooth(12, 3.0, map.z, map.size); break;
        case 1024:Smooth(15, 3.0, map.z, map.size); break;
        case 2048:Smooth(17, 3.0, map.z, map.size); break;
        default:  Smooth(18, 3.0, map.z, map.size); break;
    }
    
    ScaleToFit(map.z, map.height);
}

void PostProcessTextures(Map_& map, TextureMapping_& mapping)
{
    AddCoastTextures(map, mapping);
    AddMountainTransition(map, mapping);
    SmoothTextures(map, mapping);
    IncreaseMountains(map, mapping);
}

void CreateContinentMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping)
{
    map.name = "Random Continent";
    
    const auto size = map.size;

    map.z = CreateRandomHeightMap(size, map.height, rnd);
    
    Reshaper shaper(rnd, map.height);
    
    shaper.Elevate(map.z, Reshaper::Contrast, size, 4.0);
    shaper.Elevate(map.z, Reshaper::Center, size, 1.5);

    PostProcessHeightMap(map);

    double percentageMountain;
    double percentageWater = 0.5;
    
    switch (size.x + size.y)
    {
        case  128: percentageMountain = 0.05; break;
        case  256: percentageMountain = 0.10; break;
        case  512: percentageMountain = 0.11; break;
        case 1024: percentageMountain = 0.15; break;
        case 2048: percentageMountain = 0.18; break;
        default:   percentageMountain = 0.18; break;
    }
    
    map.mountains = MountainLevel(map.z, percentageMountain);
    map.sea = SeaLevel(map.z, percentageWater);
    
    CreateTextures(map, mapping);
    PostProcessTextures(map, mapping);
    
    RiverBrush brush(mapping);
    Tile center(Position(size.x / 2, size.y / 2));
    int length = (size.x + size.y) / 4;
    int maximumRivers;
    
    switch (size.x + size.y)
    {
        case  128: maximumRivers = 0; break;
        case  256: maximumRivers = 1; break;
        case  512: maximumRivers = 2; break;
        case 1024: maximumRivers = 4; break;
        case 2048: maximumRivers = 8; break;
        default:   maximumRivers = 1; break;
    }
    
    for (int i = 0; i < maximumRivers; i++)
    {
        if (rnd.ByChance(33))
        {
            RandomRiver(brush).Create(rnd, map, center, i, length);
        }
    }

    const int minimumIslandSize = int(0.01 * size.x * size.y);
    const int minimumCoastSize = 20;

    PlaceHarbors(minimumIslandSize, minimumCoastSize, map.numPlayers, map, mapping);
    PlaceHeadQuarters(map, mapping, rnd, map.numPlayers);
}

void CreateEdgecaseMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping)
{
    map.name = "Random Edgecase";

    const MapExtent size = map.size;
    
    map.z = CreateRandomHeightMap(size, map.height, rnd);
    
    Reshaper shaper(rnd, map.height);
    
    shaper.Elevate(map.z, Reshaper::Corners, size, 2);
    shaper.Elevate(map.z, Reshaper::Center, size, -4);
    shaper.Elevate(map.z, Reshaper::Edges, size, 4);

    PostProcessHeightMap(map);
    
    double pctMountain, pctSea;
    switch ((size.x + size.y) / 2)
    {
        case  64: pctMountain = 0.40; pctSea = 0.155; break;
        case 128: pctMountain = 0.45; pctSea = 0.160; break;
        case 256: pctMountain = 0.46; pctSea = 0.182; break;
        case 512: pctMountain = 0.45; pctSea = 0.185; break;
        default:  pctMountain = 0.50; pctSea = 0.190; break;
    }
    
    map.mountains = MountainLevel(map.z, pctMountain);
    map.sea = SeaLevel(map.z, pctSea);

    CreateTextures(map, mapping);
    
    int waterTiles = std::count_if(map.z.begin(),
                                   map.z.end(),
                                   [&map](auto h) { return h <= map.sea; });
    
    int allIslandSize = static_cast<int>(waterTiles * 0.3);
    int islandLength = (size.x + size.y) / 2;
    int islands = rnd.Rand(-2, 3);
    
    for (int i = 0; i < islands; ++i)
    {
        CreateIsland(rnd, allIslandSize / islands, islandLength, 5, map, mapping);
    }
    
    PostProcessTextures(map, mapping);
    
    std::vector<Position> sources {
        Position(0, size.y/2), // right
        Position(0, 0),        // down & right
        Position(size.x/2, 0), // down
        Position(0, 0),        // down & left
        Position(0, size.y/2), // left
        Position(0, 0),        // up & left
        Position(size.x/2, 0), // up
        Position(0, 0)         // up & right
    };
    
    RiverBrush brush(mapping);
    for (int i = 0; i < 8; i++)
    {
        int fixed = (size.x + size.y) / (8 -  2 * (i % 2));
        int length = int(rnd.DRand(1.0, 1.5) * fixed);

        RandomRiver(brush).Create(rnd, map, sources[i], i, length);
    }
    
    SmoothTextures(map, mapping, true);
    
    const int minimumIslandSize = int(0.01 * size.x * size.y);
    const int minimumCoastSize = 20;

    PlaceHarbors(minimumIslandSize, minimumCoastSize, -1, map, mapping);
    PlaceHeadQuarters(map, mapping, rnd, map.numPlayers);
}

void CreateMigrationMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping)
{
    map.name = "Random Migration";

    const MapExtent size = map.size;

    map.z = CreateRandomHeightMap(size, map.height, rnd);
    
    Reshaper shaper(rnd, map.height);

    shaper.Elevate(map.z, Reshaper::Center, size, 2);
    
    PostProcessHeightMap(map);
    
    map.sea = SeaLevel(map.z, 0.7);
    map.mountains = MountainLevel(map.z, 0.1);
    
    CreateTextures(map, mapping);

    Islands islands(map.numPlayers);
    
    double percentageOfWaterForPlayerIslands;
    
    switch (size.x + size.y)
    {
        case 128:  percentageOfWaterForPlayerIslands = 0.6; break;
        case 256:  percentageOfWaterForPlayerIslands = 0.5; break;
        case 512:  percentageOfWaterForPlayerIslands = 0.3; break;
        case 1024: percentageOfWaterForPlayerIslands = 0.2; break;
        case 2048: percentageOfWaterForPlayerIslands = 0.2; break;
        default:   percentageOfWaterForPlayerIslands = 0.6; break;
    }
    
    const int maximumNumberOfPlayers = 7;
    
    int waterTiles = std::count_if(map.z.begin(),
                                   map.z.end(),
                                   [&map](auto h) { return h <= map.sea; });
    
    auto waterTilesPerPlayer = int(waterTiles
                                   * percentageOfWaterForPlayerIslands
                                   / maximumNumberOfPlayers);
    
    int islandLength = (size.x + size.y) / 2;
    int islandDistance;

    switch (size.x + size.y)
    {
        case 128:  islandDistance = 3; break;
        case 256:  islandDistance = 5; break;
        case 512:  islandDistance = 10; break;
        case 1024: islandDistance = 12; break;
        case 2048: islandDistance = 14; break;
        default:   islandDistance = (size.x + size.y) / 16; break;
    }
    
    for (int i = 0; i < map.numPlayers; i++)
    {
        islands[i] = CreateIsland(rnd,
                                  waterTilesPerPlayer,
                                  islandLength,
                                  islandDistance,
                                  map,
                                  mapping);
    }
    
    PostProcessTextures(map, mapping);
    
    RiverBrush brush(mapping);
    Tile center(Position(size.x / 2, size.y / 2));
    int length = (size.x + size.y) / 4;
    
    auto directions = rnd.ShuffledRange(8);
    for (int i = 0; i < map.numPlayers; i++)
    {
        if (rnd.Rand(1, 100) < 20)
        {
            RandomRiver(brush).Create(rnd, map, center, directions[i], length);
        }
    }
    
    const int minimumIslandSize = int(0.01 * size.x * size.y);
    const int minimumCoastSize = 20;
    
    PlaceHarbors(minimumIslandSize, minimumCoastSize, map.numPlayers, map, mapping);

    for (int i = 0; i < map.numPlayers; i++)
    {
        if (!PlaceHeadQuarter(map, mapping, i, islands[i]))
        {
            std::cout << "Failed to place HQ for player " << (i+1) << std::endl;
        }
    }
}

void CreateCrazyMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping)
{
    map.name = "Random Rivers";

    const auto size = map.size;

    // HEIGHT MAP ...
    
    map.z = CreateRandomHeightMap(size, map.height, rnd);
    auto corner = Reshaper::Corner(rnd.Rand(0, 8));
    
    Reshaper(rnd, map.height).Elevate(map.z, corner, rnd.DRand(-2.0, 2.0), size);
    PostProcessHeightMap(map);

    // TEXTURES ...

    map.mountains = MountainLevel(map.z, rnd.DRand(0.1, 0.2));
    map.sea = SeaLevel(map.z, rnd.DRand(0.2, 0.6));

    CreateTextures(map, mapping);

    // ISLANDS ...
    
    int islandLength = (size.x + size.y) / 2;
    int islandSize = int(size.x * size.y * rnd.DRand(0.5, 2.0));
    int islandDistance;

    switch (size.x + size.y)
    {
        case 128:  islandDistance = 4; break;
        case 256:  islandDistance = 5; break;
        case 512:  islandDistance = 6; break;
        case 1024: islandDistance = 7; break;
        case 2048: islandDistance = 8; break;
        default:   islandDistance = (size.x + size.y) / 32; break;
    }
    
    if (rnd.ByChance(50))
    {
        CreateIsland(rnd,
                     islandSize,
                     islandLength,
                     islandDistance,
                     map,
                     mapping);
    }
    
    // FIXING TEXTURES ...
    
    PostProcessTextures(map, mapping);
    
    // RIVERS ...
    
    RiverBrush brush(mapping);
    for (int i = 0; i < rnd.Rand(0, 2); i++)
    {
        int direction = rnd.Rand(0, 7);
        int length = int(rnd.Rand((size.x+size.y) / 2, size.x+size.y));

        RandomRiver(brush).Create(rnd, map, rnd.RandomPoint(size), direction, length);
    }
    
    // HARBORS & HQs ...
    
    const int minimumIslandSize = int(0.01 * size.x * size.y);
    const int minimumCoastSize = 40;
    
    PlaceHarbors(minimumIslandSize, minimumCoastSize, -1, map, mapping);
    PlaceHeadQuarters(map, mapping, rnd, map.numPlayers);
}

void CreateRiversMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping)
{
    map.name = "Random Rivers";
    
    auto size = map.size;
    auto total = size.x * size.y;
    
    map.z = CreateRandomHeightMap(size, map.height, rnd);
    auto corner = Reshaper::Corner(rnd.Rand(0, 8));
    
    Reshaper(rnd, map.height).Elevate(map.z, corner, 2.0, size);

    PostProcessHeightMap(map);

    map.mountains = MountainLevel(map.z, 0.09);
    map.sea = SeaLevel(map.z, 0.55);

    CreateTextures(map, mapping);

    int islandLength = (size.x + size.y) / 2;
    int islandSize = total * 2;
    int islandDistance;
    
    switch (size.x + size.y)
    {
        case 128:  islandDistance = 4; break;
        case 256:  islandDistance = 5; break;
        case 512:  islandDistance = 6; break;
        case 1024: islandDistance = 7; break;
        case 2048: islandDistance = 8; break;
        default:   islandDistance = (size.x + size.y) / 32; break;
    }
    
    CreateIsland(rnd, islandSize, islandLength, islandDistance, map, mapping);

    PostProcessTextures(map, mapping);
    
    const int minimumIslandSize = int(0.01 * size.x * size.y);
    const int minimumCoastSize = 40;
    
    PlaceHarbors(minimumIslandSize, minimumCoastSize, -1, map, mapping);
    PlaceHeadQuarters(map, mapping, rnd, map.numPlayers);
}

void CreateRandomMap(const std::string& filePath, const MapSettings& settings)
{
    RandomUtility rnd;
    WorldDescription worldDesc;
    GameDataLoader gdLoader(worldDesc);
    
    if(!gdLoader.Load())
    {
        throw std::runtime_error("failed to load game data");
    }
    
    RttrTextureMapping mapping(worldDesc, settings.type);
    Map_ map(worldDesc, settings.size);

    map.author = "Auto";
    map.landscape = settings.type;
    map.numPlayers = settings.numPlayers;
    
    switch (settings.style)
    {
        case MapStyle::Continent:
            CreateContinentMap(rnd, map, mapping);
            break;
        case MapStyle::Edgecase:
            CreateEdgecaseMap(rnd, map, mapping);
            break;
        case MapStyle::Migration:
            CreateMigrationMap(rnd, map, mapping);
            break;
        case MapStyle::Rivers:
            CreateRiversMap(rnd, map, mapping);
            break;
        case MapStyle::Random:
            CreateCrazyMap(rnd, map, mapping);
            break;
        default:
            throw std::invalid_argument("selected map style not supported");
    }

    PlaceResources(map, rnd, mapping, settings);
    
    libsiedler2::Write(filePath, map.CreateArchiv());
}

}}
