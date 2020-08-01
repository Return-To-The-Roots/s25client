// Copyright (c) 2018 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef PathConditionReachable_h__
#define PathConditionReachable_h__

#include "world/World.h"

struct PathConditionReachable
{
    const World& world;

    PathConditionReachable(const World& world) : world(world) {}

    // Called for every node but the start & goal and should return true, if this point is usable
    BOOST_FORCEINLINE bool IsNodeOk(const MapPoint& pt) const
    {
        bool goodTerrainFound = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const TerrainDesc& t = world.GetDescription().get(world.GetRightTerrain(pt, dir));
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
        const TerrainDesc& tLeft = world.GetDescription().get(world.GetLeftTerrain(fromPt, dir));
        const TerrainDesc& tRight = world.GetDescription().get(world.GetRightTerrain(fromPt, dir));
        // Don't go next to danger terrain
        return !tLeft.Is(ETerrain::Unreachable) && !tRight.Is(ETerrain::Unreachable);
    }
};

#endif // PathConditionReachable_h__
