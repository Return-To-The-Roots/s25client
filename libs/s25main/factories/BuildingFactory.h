// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

    static noBuilding* CreateBuilding(GameWorldBase& world, BuildingType type, MapPoint pt, unsigned char player,
                                      Nation nation);
};
