// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"

struct BuildingNote
{
    ENABLE_NOTIFICATION(BuildingNote);

    enum Type
    {
        Constructed,
        Destroyed,
        Captured,     /// Military building was captured (player = new owner)
        Lost,         /// Military building was captured or lost
        NoRessources, /// Building can't find any more resources
        LuaOrder,     /// Ordered to build by lua
        LostLand      /// Lost land to another player's military building
    };

    BuildingNote(Type type, unsigned player, const MapPoint& pos, BuildingType bld)
        : type(type), player(player), pos(pos), bld(bld)
    {}

    const Type type;
    const unsigned player;
    const MapPoint pos;
    const BuildingType bld;
};
