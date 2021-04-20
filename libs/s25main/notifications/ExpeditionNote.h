// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"
#include "gameTypes/MapCoordinates.h"

struct ExpeditionNote
{
    ENABLE_NOTIFICATION(ExpeditionNote);

    enum Type
    {
        Waiting,
        ColonyFounded
    };

    ExpeditionNote(Type type, unsigned player, const MapPoint& pos) : type(type), player(player), pos(pos) {}

    const Type type;
    const unsigned player;
    const MapPoint pos;
};
