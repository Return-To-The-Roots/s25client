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
#include <stdexcept>

void MapGenerator::Create(const std::string& filePath, const MapSettings& settings)
{
    RandomConfig::MapType mapType;

    // create a random map generator based on the map style
    switch (settings.style)
    {
        case MS_Greenland:
            mapType = RandomConfig::Greenland;
            break;
        case MS_Riverland:
            mapType = RandomConfig::Riverland;
            break;
        case MS_Islands:
            mapType = RandomConfig::Islands;
            break;
        case MS_Continent:
            mapType = RandomConfig::Continent;
            break;
        case MS_Migration:
            mapType = RandomConfig::Migration;
            break;
        case MS_Ringland:
            mapType = RandomConfig::Ringland;
            break;
        case MS_Random:
            mapType = RandomConfig::Random;
            break;
        default:
            throw std::logic_error("Invalid enum value");
    }
    RandomConfig config(mapType);
    RandomMapGenerator generator(config);
    Map* randomMap = generator.Create(settings);

    // generate the random map
    libsiedler2::ArchivInfo* archiv = randomMap->CreateArchiv();
    libsiedler2::Write(filePath, *archiv);
    
    // cleanup map and archiv
    delete randomMap;
    delete archiv;
}
