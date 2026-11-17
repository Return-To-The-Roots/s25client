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
        : PathConditionHuman(gwb), player(gwb.getPlayer(player))
    {}

    // Called for every node but the start & goal and should return true, if this point is usable
    BOOST_FORCEINLINE bool isNodeOk(const MapPoint& pt) const
    {
        if(!PathConditionHuman::isNodeOk(pt))
            return false;

        unsigned char owner = world.getNode(pt).owner;
        // Ally or no player? Then ok
        return (owner == 0 || player.isAlly(owner - 1));
    }
};
