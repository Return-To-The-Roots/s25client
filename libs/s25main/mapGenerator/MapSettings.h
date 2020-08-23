// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef MapSettings_h__
#define MapSettings_h__

#include "gameTypes/MapCoordinates.h"
#include "gameData/DescIdx.h"
#include "gameData/LandscapeDesc.h"
#include <string>

namespace rttr { namespace mapGenerator {

    enum class MapStyle
    {
        Water,
        Land,
        Mixed
    };

    struct MapSettings
    {
        MapSettings()
            : numPlayers(2), size(MapExtent::all(128)), ratioGold(9), ratioIron(36), ratioCoal(40), ratioGranite(15), type(0),
              style(MapStyle::Mixed)
        {}

        void Validate();

        std::string name, author;
        unsigned numPlayers;
        MapExtent size;
        unsigned short ratioGold;
        unsigned short ratioIron;
        unsigned short ratioCoal;
        unsigned short ratioGranite;
        DescIdx<LandscapeDesc> type;
        MapStyle style;
    };

}} // namespace rttr::mapGenerator

#endif // MapSettings_h__
