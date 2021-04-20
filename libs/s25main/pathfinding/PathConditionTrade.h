// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
