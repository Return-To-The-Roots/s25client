// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RoadSegment.h"
#include "pathfinding/PathConditionReachable.h"
#include "world/World.h"
#include "nodeObjs/noBase.h"
#include "gameData/TerrainDesc.h"
#include <boost/config.hpp>

struct PathConditionHuman : PathConditionReachable
{
    PathConditionHuman(const World& world) : PathConditionReachable(world) {}

    // Called for every node but the start & goal and should return true, if this point is usable
    BOOST_FORCEINLINE bool isNodeOk(const MapPoint& pt) const
    {
        // Node blocked -> Can't go there
        const auto* no = world.getNode(pt).obj;
        const BlockingManner bm = no ? no->getBM() : BlockingManner::None;
        if(bm != BlockingManner::None && bm != BlockingManner::Tree && bm != BlockingManner::Flag)
            return false;
        return PathConditionReachable::isNodeOk(pt);
    }

    // Called for every node
    BOOST_FORCEINLINE bool isEdgeOk(const MapPoint& fromPt, const Direction dir) const
    {
        // If there is a road (but no boat road) we can pass
        const PointRoad road = world.getPointRoad(fromPt, dir);
        if(road != PointRoad::None && road != PointRoad::Boat)
            return true;

        // Check terrain for node transition
        const auto terrains = world.getTerrain(fromPt, dir);
        const TerrainDesc& tLeft = world.getDescription().get(terrains.left);
        const TerrainDesc& tRight = world.getDescription().get(terrains.right);
        // Don't go next to danger terrain
        if(tLeft.is(ETerrain::Unreachable) || tRight.is(ETerrain::Unreachable))
            return false;
        // If either terrain is walkable, then we can use this transition
        return tLeft.is(ETerrain::Walkable) || tRight.is(ETerrain::Walkable);
    }
};
