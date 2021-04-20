// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

struct RoadNote
{
    ENABLE_NOTIFICATION(RoadNote);

    enum Type
    {
        Constructed,
        ConstructionFailed
    };

    RoadNote(Type type, unsigned player, const MapPoint& pos, const std::vector<Direction>& route)
        : type(type), player(player), pos(pos), route(route)
    {}

    const Type type;
    const unsigned player;
    const MapPoint pos;
    const std::vector<Direction>& route;
};
