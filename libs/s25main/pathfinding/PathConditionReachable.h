// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "world/World.h"

struct PathConditionReachable
{
    const World& world;

    PathConditionReachable(const World& world) : world(world) {}

    // Called for every node but the start & goal and should return true, if this point is usable
    BOOST_FORCEINLINE bool IsNodeOk(const MapPoint& pt) const
    {
        bool goodTerrainFound = false;
        for(const DescIdx<TerrainDesc> tIdx : world.GetTerrainsAround(pt))
        {
            const TerrainDesc& t = world.GetDescription().get(tIdx);
            if(t.Is(ETerrain::Unreachable))
                return false;
            else if(t.Is(ETerrain::Walkable))
                goodTerrainFound = true;
        }
        return goodTerrainFound;
    }
    // Called for every node
    BOOST_FORCEINLINE bool IsEdgeOk(const MapPoint& fromPt, const Direction dir) const
    {
        // Check terrain for node transition
        const auto terrains = world.GetTerrain(fromPt, dir);
        const TerrainDesc& tLeft = world.GetDescription().get(terrains.left);
        const TerrainDesc& tRight = world.GetDescription().get(terrains.right);
        // Don't go next to danger terrain
        return !tLeft.Is(ETerrain::Unreachable) && !tRight.Is(ETerrain::Unreachable);
    }
};
