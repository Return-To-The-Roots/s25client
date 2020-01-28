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
#include "randomMaps/Rivers.h"

#include "randomMaps/waters/WaterMap.h"
#include "randomMaps/waters/River.h"
#include "randomMaps/waters/RandomRiver.h"
#include "randomMaps/waters/Island.h"

#include "randomMaps/objects/Harbors.h"
#include "randomMaps/objects/HeadQuarters.h"

#include "randomMaps/terrain/TextureFitter.h"
#include "randomMaps/terrain/TextureMap.h"

#include "randomMaps/resources/Resources.h"

#include "randomMaps/elevation/MountainElevator.h"
#include "randomMaps/elevation/Reshaper.h"
#include "randomMaps/elevation/Smoother.h"
#include "randomMaps/elevation/Scaler.h"
#include "randomMaps/elevation/HeightMap.h"
#include "randomMaps/elevation/Level.h"

Map* Rivers::Create(const MapSettings& settings)
{
    const int players = settings.numPlayers;

    auto size = settings.size;
    auto total = size.x * size.y;
    
    HeightSettings heightSettings(10, 60);
    RandomHeightMap heightMap(rnd_, heightSettings);

    auto z = heightMap.Create(size);
    auto corner = ElevationCorner(rnd_.Rand(0, 8));
    
    Reshaper(rnd_, heightSettings).Elevate(z, corner, 2.0, size);
    Smoother(size).Smooth(z);

    TextureTranslator textureTranslator(heightSettings);
    TextureMap textureMap(textureTranslator);
    
    auto mountainLevel = Level::Mountain(z, 0.09);
    auto waterLevel = Level::Water(z, 0.55);
    auto waterMap = WaterMap::Create(z, size, waterLevel);
    
    auto textures = textureMap.Create(z, waterMap, size, waterLevel, mountainLevel);
    
    Map* map = new Map(size, "Random Rivers", "Auto");
    map->numPlayers = players;
    map->textureLsd = std::vector<TextureType>(textures);
    map->textureRsu = std::vector<TextureType>(textures);
    map->z = z;

    IslandPlacer islandPlacer(heightSettings);
    Island island(rnd_, islandPlacer);
    
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
    
    island
        .OfLength(islandLength)
        .OfSize(islandSize)
        .OfDistance(islandDistance)
        .Place(map, waterLevel);

    TextureFitter::SmoothTextures(map->textureRsu, map->textureLsd, size);
    TextureFitter::SmoothMountains(map->textureRsu, map->textureLsd, size);
    
    const int minimumIslandSize = (size.x + size.y) / 8;
    const int minimumCoastSize = 20;
    const int maximumIslandHarbors = players;
    
    Harbors(minimumIslandSize, minimumCoastSize, maximumIslandHarbors).Place(*map);
    HeadQuarters hqs(rnd_);
    hqs.Place(*map, settings.numPlayers);
    Resources(rnd_).Generate(*map, settings);
    
    return map;
}
