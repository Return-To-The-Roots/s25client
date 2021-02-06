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
    BOOST_FORCEINLINE bool IsNodeOk(const MapPoint& pt) const
    {
        // Node blocked -> Can't go there
        const auto* no = world.GetNode(pt).obj;
        const BlockingManner bm = no ? no->GetBM() : BlockingManner::None;
        if(bm != BlockingManner::None && bm != BlockingManner::Tree && bm != BlockingManner::Flag)
            return false;
        return PathConditionReachable::IsNodeOk(pt);
    }

    // Called for every node
    BOOST_FORCEINLINE bool IsEdgeOk(const MapPoint& fromPt, const Direction dir) const
    {
        // If there is a road (but no boat road) we can pass
        const PointRoad road = world.GetPointRoad(fromPt, dir);
        if(road != PointRoad::None && road != PointRoad::Boat)
            return true;

        // Check terrain for node transition
        const auto terrains = world.GetTerrain(fromPt, dir);
        const TerrainDesc& tLeft = world.GetDescription().get(terrains.left);
        const TerrainDesc& tRight = world.GetDescription().get(terrains.right);
        // Don't go next to danger terrain
        if(tLeft.Is(ETerrain::Unreachable) || tRight.Is(ETerrain::Unreachable))
            return false;
        // If either terrain is walkable, then we can use this transition
        return tLeft.Is(ETerrain::Walkable) || tRight.Is(ETerrain::Walkable);
    }
};
