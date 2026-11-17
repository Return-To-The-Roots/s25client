// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "world/World.h"
#include "gameData/TerrainDesc.h"
#include <boost/config.hpp>

struct PathConditionShip
{
    const World& world;

    PathConditionShip(const World& world) : world(world) {}

    // Called for every node but the start & goal and should return true, if this point is usable
    BOOST_FORCEINLINE bool isNodeOk(const MapPoint& pt) const { return world.isSeaPoint(pt); }

    // Called for every node
    BOOST_FORCEINLINE bool isEdgeOk(const MapPoint& fromPt, const Direction dir) const
    {
        // We must have shippable water on both sides
        const auto terrains = world.getTerrain(fromPt, dir);
        const TerrainDesc& tLeft = world.getDescription().get(terrains.left);
        const TerrainDesc& tRight = world.getDescription().get(terrains.right);
        return tLeft.is(ETerrain::Shippable) && tRight.is(ETerrain::Shippable);
    }
};
