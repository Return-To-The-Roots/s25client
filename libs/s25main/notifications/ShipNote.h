// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"
#include "gameTypes/MapCoordinates.h"

struct ShipNote
{
    ENABLE_NOTIFICATION(ShipNote);

    enum Type
    {
        Constructed,
        Destroyed
    };

    ShipNote(Type type, unsigned player, const MapPoint& pos) : type(type), player(player), pos(pos) {}

    const Type type;
    const unsigned player;
    const MapPoint pos;
};
