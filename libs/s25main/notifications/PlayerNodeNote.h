// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"
#include "gameTypes/MapCoordinates.h"

struct PlayerNodeNote
{
    ENABLE_NOTIFICATION(PlayerNodeNote);

    enum Type
    {
        Visibility // Nodes visibility has changed
    };

    PlayerNodeNote(Type type, const MapPoint& pt, unsigned player) : type(type), pt(pt), player(player) {}

    const Type type;
    const MapPoint pt;
    const unsigned player; // Player for which this node has changed
};
