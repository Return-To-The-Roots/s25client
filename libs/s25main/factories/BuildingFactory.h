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

#pragma once

#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Nation.h"

class noBuilding;
class GameWorldBase;

/// Static Factory class used to create buildings
/// Use ONLY this class to add new buildings to the map.
/// Only exception is during deserialization (switch on GOT instead of building type), this case is handled in
/// SerializedGameData
class BuildingFactory
{
public:
    BuildingFactory() = delete;

    static noBuilding* CreateBuilding(GameWorldBase& gwg, BuildingType type, MapPoint pt, unsigned char player,
                                      Nation nation);
};
