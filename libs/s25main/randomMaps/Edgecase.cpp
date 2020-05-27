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
#include "randomMaps/Edgecase.h"

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

Map* Edgecase::Create(const MapSettings& settings)
{
    const MapExtent size = settings.size;
    const int players = settings.numPlayers;

    HeightSettings heightSettings(0,32);
    RandomHeightMap heightMap(rnd_, heightSettings);

    auto z = heightMap.Create(size);
    
    Smoother smoother(size);
    Scaler scaler(heightSettings);
    Reshaper shaper(rnd_, heightSettings);
    
    shaper.Elevate(z, Corners, size, 2);
    shaper.Elevate(z, Center, size, -4);
    shaper.Elevate(z, Edges, size, 4);
    smoother.Smooth(z);
    scaler.Scale(z);
    
    double pctMountain, pctSea;
    switch ((size.x + size.y) / 2) {
        case  64: pctMountain = 0.40; pctSea = 0.155; break;
        case 128: pctMountain = 0.45; pctSea = 0.160; break;
        case 256: pctMountain = 0.46; pctSea = 0.182; break;
        case 512: pctMountain = 0.45; pctSea = 0.185; break;
        default:  pctMountain = 0.50; pctSea = 0.190; break;
    }
    
    Level level;
    auto mountainLevel = level.Mountain(z, pctMountain);
    auto seaLevel      = level.Water(   z, pctSea);
    auto waterMap      = WaterMap::Create(z, size, seaLevel);

    TextureTranslator textureTranslator(heightSettings);
    TextureMap textureMap(textureTranslator);
    
    auto textures = textureMap.Create(z, waterMap, size, seaLevel, mountainLevel);
    auto textureRsu = textures;
    auto textureLsd(textures);

    TextureFitter::SmoothTextures(textureRsu, textureLsd, size);
    TextureFitter::SmoothMountains(textureRsu, textureLsd, size);

    Map* map = new Map(size, "Random Edgecase", "Auto");
    map->numPlayers = players;
    map->textureLsd = textureLsd;
    map->textureRsu = textureRsu;
    map->z = z;
    
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
    
    RiverBrush brush;
    for (int i = 0; i < 8; i++)
    {
        int fixed = (size.x + size.y) / (8 -  2 * (i % 2));
        int length = int(rnd_.DRand(1.0, 1.5) * fixed);

        RandomRiver(rnd_, brush).Create(map, sources[i], i, length, seaLevel);
    }
    
    TextureFitter::SmoothTextures(textureRsu, textureLsd, size, true);
    
    const int minimumIslandSize = (size.x + size.y) / 8;

    Harbors(minimumIslandSize, 20, players).Place(*map);
    HeadQuarters hqs(rnd_);
    hqs.Place(*map, settings.numPlayers);
    Resources(rnd_).Generate(*map, settings);
    
    return map;
}
