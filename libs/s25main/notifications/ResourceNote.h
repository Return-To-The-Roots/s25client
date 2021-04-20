// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Resource.h"

struct ResourceNote
{
    ENABLE_NOTIFICATION(ResourceNote);

    ResourceNote(unsigned player, const MapPoint& pos, Resource res) : player(player), pos(pos), res(res) {}

    const unsigned player;
    const MapPoint pos;
    const Resource res;
};
