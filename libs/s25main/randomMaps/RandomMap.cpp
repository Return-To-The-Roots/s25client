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
#include "randomMaps/RandomMap.h"
#include "randomMaps/Continent.h"
#include "randomMaps/Edgecase.h"
#include "randomMaps/Migration.h"
#include "randomMaps/Random.h"
#include "randomMaps/Rivers.h"

Map* RandomMap::Create(const MapSettings& settings)
{
    Continent continent(rnd_);
    Edgecase edgecase(rnd_);
    Migration migration(rnd_);
    Rivers rivers(rnd_);
    Random random(rnd_);

    switch (settings.style) {
        case MapStyle::Continent:
            return continent.Create(settings);
        case MapStyle::Edgecase:
            return edgecase.Create(settings);
        case MapStyle::Migration:
            return migration.Create(settings);
        case MapStyle::Rivers:
            return rivers.Create(settings);
        case MapStyle::Random:
            return random.Create(settings);
    }
    return NULL;
}
