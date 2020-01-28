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
#include "randomMaps/Continent.h"

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

Map* Continent::Create(const MapSettings& settings)
{
    const MapExtent size = settings.size;
    const int players = settings.numPlayers;

    HeightSettings heightSettings(0,32);
    RandomHeightMap heihgtMap(rnd_, heightSettings);

    auto z = heihgtMap.Create(size);
    
    Smoother smoother(size);
    Scaler scaler(heightSettings);
    Reshaper shaper(rnd_, heightSettings);
    
    shaper.Elevate(z, Contrast, size, 4.0);
    shaper.Elevate(z, Center, size, 1.5);
    smoother.Smooth(z);
    scaler.Scale(z);
    
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
    
    Level level;
    auto mountainLevel = level.Mountain(z, percentageMountain);
    auto seaLevel      = level.Water(z, percentageWater);
    auto waterMap      = WaterMap::Create(z, size, seaLevel);
    
    TextureTranslator textureTranslator(heightSettings);
    TextureMap textureMap(textureTranslator);
    
    auto textures = textureMap.Create(z, waterMap, size, seaLevel, mountainLevel);
    auto textureRsu = textures;
    auto textureLsd(textures);
    
    TextureFitter::SmoothTextures(textureRsu, textureLsd, size);
    TextureFitter::SmoothMountains(textureRsu, textureLsd, size);
    
    Map* map = new Map(size, "Random Continent", "Auto");
    map->numPlayers = players;
    map->textureLsd = textureLsd;
    map->textureRsu = textureRsu;
    map->z = z;
    
    RiverBrush brush;
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
        if (rnd_.ByChance(33))
        {
            RandomRiver(rnd_, brush).Create(map, center, i, length, seaLevel);
        }
    }

    const int minimumIslandSize = (size.x + size.y) / 8;

    Harbors(minimumIslandSize, 20, players).Place(*map);
    HeadQuarters hqs(rnd_);
    hqs.Place(*map, settings.numPlayers);
    
    Resources(rnd_).Generate(*map, settings);
    
    return map;
}
