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
#include "mapGenerator/MapGenerator.h"
#include "mapGenerator/RandomConfig.h"
#include "mapGenerator/RandomMapGenerator.h"
#include "libsiedler2/src/libsiedler2.h"

void MapGenerator::Create(const std::string& filePath, const MapSettings& settings)
{
    RandomMapGenerator generator;
    Map* randomMap = NULL;

    // create a random map generator based on the map style
    switch (settings.style)
    {
        case MS_Greenland:
            randomMap = generator.Create(settings, RandomConfig::CreateGreenland());
            break;
        case MS_Riverland:
            randomMap = generator.Create(settings, RandomConfig::CreateRiverland());
            break;
        case MS_Islands:
            randomMap = generator.Create(settings, RandomConfig::CreateIslands());
            break;
        case MS_Continent:
            randomMap = generator.Create(settings, RandomConfig::CreateContinent());
            break;
        case MS_Migration:
            randomMap = generator.Create(settings, RandomConfig::CreateMigration());
            break;
        case MS_Ringland:
            randomMap = generator.Create(settings, RandomConfig::CreateRingland());
            break;
        case MS_Random:
            randomMap = generator.Create(settings, RandomConfig::CreateRandom());
            break;
    }
    
    // generate the random map
    libsiedler2::ArchivInfo* archiv = randomMap->CreateArchiv();
    libsiedler2::Write(filePath, *archiv);
    
    // cleanup map and archiv
    delete randomMap;
    delete archiv;
}
