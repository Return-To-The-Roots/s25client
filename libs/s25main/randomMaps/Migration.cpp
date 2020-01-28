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
#include "randomMaps/Migration.h"

#include "randomMaps/waters/Island.h"
#include "randomMaps/waters/RandomRiver.h"
#include "randomMaps/waters/WaterMap.h"

#include "randomMaps/objects/Harbors.h"
#include "randomMaps/objects/HeadQuarters.h"

#include "randomMaps/terrain/TextureFitter.h"
#include "randomMaps/terrain/TextureMap.h"

#include "randomMaps/resources/Resources.h"

#include "randomMaps/elevation/Reshaper.h"
#include "randomMaps/elevation/Smoother.h"
#include "randomMaps/elevation/Scaler.h"
#include "randomMaps/elevation/HeightMap.h"
#include "randomMaps/elevation/Level.h"

#include <iostream>

Map* Migration::Create(const MapSettings& settings)
{
    const MapExtent size = settings.size;
    const int players = settings.numPlayers;
    
    HeightSettings heightSettings(0, 32);
    RandomHeightMap heightMap(rnd_, heightSettings);

    auto z = heightMap.Create(size);
    
    Smoother smoother(size);
    Scaler scaler(heightSettings);
    Reshaper shaper(rnd_, heightSettings);

    shaper.Elevate(z, Center, size, 2);
    smoother.Smooth(z);
    scaler.Scale(z);
    
    Level level;
    auto seaLevel = level.Water(z, 0.7);
    auto waterMap = WaterMap::Create(z, size, seaLevel);
    
    TextureTranslator textureTranslator(heightSettings);
    TextureMap textureMap(textureTranslator);

    auto mountainLevel = level.Mountain(z, 0.1);
    
    auto textures = textureMap.Create(z, waterMap, size, seaLevel, mountainLevel);
    
    Map* map = new Map(size, "Random Migration", "Auto");
    map->numPlayers = players;
    map->textureLsd = std::vector<TextureType>(textures);
    map->textureRsu = std::vector<TextureType>(textures);
    map->z = z;

    std::vector<std::vector<Position> > islands(players);
    
    HeightSettings islandSettings(heightSettings);
    
    switch (size.x + size.y)
    {
        case 128:  islandSettings = HeightSettings(0, 23); break;
        case 256:  islandSettings = HeightSettings(0, 26); break;
    }
    
    IslandPlacer islandPlacer(islandSettings);
    Island island(rnd_, islandPlacer);
    
    float percentageOfWaterForPlayerIslands = 0.6;
    
    switch (size.x + size.y)
    {
        case 128:  percentageOfWaterForPlayerIslands = 0.6; break;
        case 256:  percentageOfWaterForPlayerIslands = 0.5; break;
        case 512:  percentageOfWaterForPlayerIslands = 0.3; break;
        case 1024: percentageOfWaterForPlayerIslands = 0.2; break;
        case 2048: percentageOfWaterForPlayerIslands = 0.2; break;
    }
    
    const int maximumNumberOfPlayers = 7;
    
    auto waterTiles = level.CountBelowOrEqual(z, seaLevel);
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

    for (int i = 0; i < players; i++)
    {
        islands[i] =
            island
                .OfSize(waterTilesPerPlayer)
                .OfLength(islandLength)
                .OfDistance(islandDistance)
                .Place(map, seaLevel);
    }

    TextureFitter::SmoothTextures(map->textureRsu, map->textureLsd, size);
    TextureFitter::SmoothMountains(map->textureRsu, map->textureLsd, size);
    
    RiverBrush brush;
    Tile center(Position(size.x / 2, size.y / 2));
    int length = (size.x + size.y) / 4;
    
    auto directions = rnd_.IRand(8);
    for (int i = 0; i < players; i++)
    {
        if (rnd_.Rand(1, 100) < 20)
        {
            RandomRiver(rnd_, brush).Create(map, center, directions[i], length, seaLevel);
        }
    }
    
    const int minimumIslandSize = (size.x + size.y) / 8;
    
    Harbors(minimumIslandSize, 20, players).Place(*map);
    HeadQuarters hqs(rnd_);
    
    for (int i = 0; i < players; i++)
    {
        if (!hqs.Place(*map, i, islands[i]))
        {
            std::cout << "Failed to place HQ for player " << (i+1) << std::endl;
        }
    }
    
    Resources(rnd_).Generate(*map, settings);
    
    return map;
}
