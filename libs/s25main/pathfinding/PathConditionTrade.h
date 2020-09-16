// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation,  either version 2 of the License,  or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not,  see <http://www.gnu.org/licenses/>.

#pragma once

#include "GamePlayer.h"
#include "pathfinding/PathConditionHuman.h"
#include "world/GameWorldBase.h"
#include <boost/config.hpp>

struct PathConditionTrade : public PathConditionHuman
{
    const GamePlayer& player;

    PathConditionTrade(const GameWorldBase& gwb, unsigned char player)
        : PathConditionHuman(gwb), player(gwb.GetPlayer(player))
    {}

    // Called for every node but the start & goal and should return true, if this point is usable
    BOOST_FORCEINLINE bool IsNodeOk(const MapPoint& pt) const
    {
        if(!PathConditionHuman::IsNodeOk(pt))
            return false;

        unsigned char owner = world.GetNode(pt).owner;
        // Ally or no player? Then ok
        return (owner == 0 || player.IsAlly(owner - 1));
    }
};
