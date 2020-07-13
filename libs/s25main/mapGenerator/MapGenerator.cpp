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

#include "mapGenerator/MapGenerator.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/RandomConfig.h"
#include "mapGenerator/RandomMapGenerator.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/libsiedler2.h"
#include <stdexcept>

void MapGenerator::Create(const std::string& filePath, const MapSettings& settings)
{
    // create a random map generator based on the map style
    RandomConfig config;
    if(!config.Init(settings.style, settings.type))
        throw std::runtime_error("Error initializing random map config");
    RandomMapGenerator generator(config);
    Map randomMap = generator.Create(settings);

    // generate the random map
    libsiedler2::Archiv archiv = randomMap.CreateArchiv();
    libsiedler2::Write(filePath, archiv);
}
