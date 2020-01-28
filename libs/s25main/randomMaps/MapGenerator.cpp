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
#include "randomMaps/MapGenerator.h"
#include "randomMaps/algorithm/RandomUtility.h"
#include "randomMaps/RandomMap.h"
#include "randomMaps/Map.h"
#include "libsiedler2/libsiedler2.h"

void MapGenerator::Create(const std::string& filePath, const MapSettings& settings)
{
    RandomUtility rnd;
    TextureMapping* textureMapping = NULL;
    
    switch (settings.type.value)
    {
        case 0: // greenland
            textureMapping = new GreenlandMapping();
            break;
        case 1: // wasteland
            textureMapping = new WastelandMapping();
            break;
        case 2: // winterland
            textureMapping = new WinterMapping();
            break;
        default:
            break;
    }

    RandomMap rndMap(rnd);
    Map* map = rndMap.Create(settings);
    map->type = settings.type.value;
    
    libsiedler2::Archiv* archiv = map->CreateArchiv(*textureMapping);
    libsiedler2::Write(filePath, *archiv);

    delete textureMapping;
    delete map;
    delete archiv;
}
