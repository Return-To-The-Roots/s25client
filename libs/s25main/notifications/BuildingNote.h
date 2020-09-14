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

    BuildingNote(Type type, unsigned player, const MapPoint& pos, BuildingType bld) : type(type), player(player), pos(pos), bld(bld) {}

    const Type type;
    const unsigned player;
    const MapPoint pos;
    const BuildingType bld;
};
