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
    BOOST_FORCEINLINE bool IsNodeOk(const MapPoint& pt) const { return world.IsSeaPoint(pt); }

    // Called for every node
    BOOST_FORCEINLINE bool IsEdgeOk(const MapPoint& fromPt, const Direction dir) const
    {
        // We must have shippable water on both sides
        const auto terrains = world.GetTerrain(fromPt, dir);
        const TerrainDesc& tLeft = world.GetDescription().get(terrains.left);
        const TerrainDesc& tRight = world.GetDescription().get(terrains.right);
        return tLeft.Is(ETerrain::Shippable) && tRight.Is(ETerrain::Shippable);
    }
};
