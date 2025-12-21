// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "notifications/notifications.h"

/// Notification emitted when a building produces a ware
struct ProductionNote
{
    ENABLE_NOTIFICATION(ProductionNote);

    unsigned player;
    MapPoint pos;
    BuildingType building;
    GoodType good;

    ProductionNote(unsigned player, MapPoint pos, BuildingType building, GoodType good)
        : player(player), pos(pos), building(building), good(good)
    {}
};
