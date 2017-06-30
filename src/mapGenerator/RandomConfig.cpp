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

#include "defines.h" // IWYU pragma: keep
#include "mapGenerator/RandomConfig.h"

RandomConfig RandomConfig::CreateGreenland()
{
    RandomConfig config;
    
    for (int i = 0; i < 4; i++)
        config.textures.push_back(TT_WATER);
    
    config.textures.push_back(TT_DESERT);
    config.textures.push_back(TT_STEPPE);
    config.textures.push_back(TT_SAVANNAH);
    config.textures.push_back(TT_MEADOW1);
    config.textures.push_back(TT_MEADOW_FLOWERS);
    config.textures.push_back(TT_MEADOW2);
    config.textures.push_back(TT_MEADOW2);
    
    config.textures.push_back(TT_MOUNTAINMEADOW);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    
    for (int i = 0; i < 10; i++)
        config.textures.push_back(TT_SNOW);

    const Point<double> center(0.5, 0.5);
    
    // greenland all over the map
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 8.0,  14, 7, 5, 10, 15));

    // small mountains and hills all over the map
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 0.2,   0, 0, 0, 19, 15));

    // very few large mountains all over the map
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 0.05,  0, 0, 0, 23, 15));

    // empty space around the players
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7,  7,  0, 4));
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10,  4, 15));

    return config;
}

RandomConfig RandomConfig::CreateRiverland()
{
    RandomConfig config;
    
    for (int i = 0; i < 4; i++)
        config.textures.push_back(TT_WATER);
    
    config.textures.push_back(TT_DESERT);
    config.textures.push_back(TT_STEPPE);
    config.textures.push_back(TT_SAVANNAH);
    config.textures.push_back(TT_MEADOW1);
    config.textures.push_back(TT_MEADOW_FLOWERS);
    config.textures.push_back(TT_MEADOW2);
    config.textures.push_back(TT_MEADOW2);
    
    config.textures.push_back(TT_MOUNTAINMEADOW);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    
    for (int i = 0; i < 10; i++)
        config.textures.push_back(TT_SNOW);
    
    const Point<double> center(0.5, 0.5);

    // mix between water and greenland all over the map
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 2.0,  14, 7, 0, 10, 15));

    // a couple of mountains (and hills) all over the map
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 0.2,   0, 0, 0, 19, 15));

    // very few, very large mountains all over the map
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 0.05,  0, 0, 0, 23, 15));

    // empty space around the players
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7,  7,  0, 4));
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10,  4, 15));

    return config;
}

RandomConfig RandomConfig::CreateRingland()
{
    RandomConfig config;
    
    for (int i = 0; i < 4; i++)
        config.textures.push_back(TT_WATER);
    
    config.textures.push_back(TT_DESERT);
    config.textures.push_back(TT_STEPPE);
    config.textures.push_back(TT_SAVANNAH);
    config.textures.push_back(TT_MEADOW1);
    config.textures.push_back(TT_MEADOW_FLOWERS);
    config.textures.push_back(TT_MEADOW2);
    config.textures.push_back(TT_MEADOW2);
    
    config.textures.push_back(TT_MOUNTAINMEADOW);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    
    for (int i = 0; i < 10; i++)
        config.textures.push_back(TT_SNOW);
    
    const double rMin = config.DRand(0.2, 0.5);
    const double rMax = config.DRand(rMin + 0.1, 0.9);
    const double rMiddle = rMin + (rMax - rMin) / 2;
    const Point<double> center(0.5, 0.5);

    // ring formed land (coastal and greenland)
    config.areas.push_back(AreaDesc(center, rMin, rMax, 8.0,  14, 7, 5, 10, 15));

    // ring formed mountain land in the middle of the greenland ring
    config.areas.push_back(AreaDesc(center, rMiddle-0.05, rMiddle+0.05, 1.0,   0, 0, 0, 20, 15));

    // empty space around the players
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7,  7,  0, 4));
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10,  4, 15));

    return config;
}

RandomConfig RandomConfig::CreateMigration()
{
    RandomConfig config;
    
    for (int i = 0; i < 4; i++)
        config.textures.push_back(TT_WATER);
    
    config.textures.push_back(TT_DESERT);
    config.textures.push_back(TT_STEPPE);
    config.textures.push_back(TT_SAVANNAH);
    config.textures.push_back(TT_MEADOW1);
    config.textures.push_back(TT_MEADOW_FLOWERS);
    config.textures.push_back(TT_MEADOW2);
    config.textures.push_back(TT_MEADOW2);
    
    config.textures.push_back(TT_MOUNTAINMEADOW);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    
    for (int i = 0; i < 10; i++)
        config.textures.push_back(TT_SNOW);
    
    const Point<double> center(0.5, 0.5);

    // inner island with large mountains
    config.areas.push_back(AreaDesc(center, 0.0, 0.2, 2.0,   14, 7, 0, 20, 20));

    // greenland around the large mountains
    config.areas.push_back(AreaDesc(center, 0.2, 0.4, 10.0,  14, 7, 5, 10, 20));

    // empty space around the players
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0,  0, 0, 7,  7,  0, 4));
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0,  8, 0, 5, 10,  4, 15));

    return config;
}

RandomConfig RandomConfig::CreateIslands()
{
    RandomConfig config;
    
    for (int i = 0; i < 4; i++)
        config.textures.push_back(TT_WATER);

    config.textures.push_back(TT_DESERT);
    config.textures.push_back(TT_STEPPE);
    config.textures.push_back(TT_SAVANNAH);
    config.textures.push_back(TT_MEADOW1);
    config.textures.push_back(TT_MEADOW_FLOWERS);
    config.textures.push_back(TT_MEADOW2);
    config.textures.push_back(TT_MEADOW2);

    config.textures.push_back(TT_MOUNTAINMEADOW);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);

    for (int i = 0; i < 10; i++)
        config.textures.push_back(TT_LAVA);
    
    const Point<double> center(0.5, 0.5);

    // little islands all over the map
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 0.06, 14, 7, 0, 18, 15));
    
    // mountain islands around each player
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 5.0,  14, 7, 10, 20, 21, 22));

    // empty space around the players
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0,  0, 0, 7,  7,  0, 4));
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0,  8, 0, 5, 10,  4, 15));
    
    return config;
}

RandomConfig RandomConfig::CreateContinent()
{
    RandomConfig config;
    
    for (int i = 0; i < 4; i++)
        config.textures.push_back(TT_WATER);
    
    config.textures.push_back(TT_DESERT);
    config.textures.push_back(TT_STEPPE);
    config.textures.push_back(TT_SAVANNAH);
    config.textures.push_back(TT_MEADOW1);
    config.textures.push_back(TT_MEADOW_FLOWERS);
    config.textures.push_back(TT_MEADOW2);
    config.textures.push_back(TT_MEADOW2);
    
    config.textures.push_back(TT_MOUNTAINMEADOW);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    
    for (int i = 0; i < 10; i++)
        config.textures.push_back(TT_SNOW);

    const Point<double> center(0.5, 0.5);

    // greenland all over the map, apart from the water at the edges
    config.areas.push_back(AreaDesc(center, 0.0, 0.9, 8.0,  14, 7, 5, 10, 15));

    // small mountains and hills all over the greenland area
    config.areas.push_back(AreaDesc(center, 0.0, 0.9, 0.2,   0, 0, 0, 18, 15));

    // few very high mountains and hills all over the greenland area
    config.areas.push_back(AreaDesc(center, 0.0, 0.9, 0.05,  0, 0, 0, 23, 15));

    // empty space around the players
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 0, 0, 7,  7,  0, 4));
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0, 8, 0, 5, 10,  4, 15));

    return config;
}

RandomConfig RandomConfig::CreateRandom()
{
    RandomConfig config;
    
    for (int i = 0; i < 4; i++)
        config.textures.push_back(TT_WATER);
    
    config.textures.push_back(TT_DESERT);
    config.textures.push_back(TT_STEPPE);
    config.textures.push_back(TT_SAVANNAH);
    config.textures.push_back(TT_MEADOW1);
    config.textures.push_back(TT_MEADOW_FLOWERS);
    config.textures.push_back(TT_MEADOW2);
    config.textures.push_back(TT_MEADOW2);
    
    config.textures.push_back(TT_MOUNTAINMEADOW);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    config.textures.push_back(TT_MOUNTAIN1);
    
    for (int i = 0; i < 10; i++)
        config.textures.push_back(TT_SNOW);
    
    const double p1 = config.DRand(0.0, 0.4);
    const double p2 = config.DRand(p1, p1 + 1.4);
    const double p3 = config.DRand(p2, p2 + 1.0);
    const double pHill = config.DRand(1.5, 5.0);
    const int minHill = config.Rand(5);
    const Point<double> center(0.5, 0.5);

    // random inner area with large mountains
    config.areas.push_back(AreaDesc(center, 0.0, p1,    1.0,  4, 7, 0, 23, 15));

    // random middle area with greenland
    config.areas.push_back(AreaDesc(center, p1,  p2,    pHill, 18, 5, minHill, 10, 15));

    // random mountains in the greenland area
    config.areas.push_back(AreaDesc(center, p1,  p2,    0.5,  0, 0, 0, 17, 18));

    // random islands in the remaining water
    config.areas.push_back(AreaDesc(center, p2,  p3,    0.1, 15, 5, 0,  7, 15));
    
    // empty space around the players
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0,  0, 0, 7,  7,  0, 4));
    config.areas.push_back(AreaDesc(center, 0.0, 2.0, 100.0,  8, 0, 5, 10,  4, 15));
    
    return config;
}

int RandomConfig::Rand(const int max)
{
    return Rand(0, max);
}

int RandomConfig::Rand(const int min, const int max)
{
    // NOTE: the portable RANDOM class generates the same sequence of values after
    // a while when creating large number of new values. Therefore, the platform
    // dependent rand() function is used here.

    return min + rand() % (max - min);
}

double RandomConfig::DRand(const double min, const double max)
{
    return min + static_cast<double>(Rand(0, RAND_MAX)) /
        (static_cast<double>(RAND_MAX / (max - min)));
}

