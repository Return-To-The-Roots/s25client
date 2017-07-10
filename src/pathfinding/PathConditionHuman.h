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

#ifndef PathConditionHuman_h__
#define PathConditionHuman_h__

#include "world/World.h"
#include "nodeObjs/noBase.h"
#include "gameData/TerrainData.h"

struct PathConditionHuman
{
    const World& world;

    PathConditionHuman(const World& world): world(world){}

    // Called for every node but the start & goal and should return true, if this point is usable
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt) const
    {
        // Node blocked -> Can't go there
        const BlockingManner bm = world.GetNO(pt)->GetBM();
        if(bm != BlockingManner::None && bm != BlockingManner::Tree && bm != BlockingManner::Flag)
            return false;
        // Check that there is no deadly terrain and at least one good (buildable) terrain
        bool goodTerrainFound = false;
        for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
        {
            // Note: Old version used TerrainData::IsUsable which disallowed buildable water. Refine that?
            // However we still need to check if ANY terrain is dangerous
            const TerrainBQ bq = TerrainData::GetBuildingQuality(world.GetRightTerrain(pt, Direction::fromInt(dir)));
            if(bq == TerrainBQ::DANGER)
                return false;
            else if(bq != TerrainBQ::NOTHING)
                goodTerrainFound = true;
        }
        return goodTerrainFound;
    }

    // Called for every node
    FORCE_INLINE bool IsEdgeOk(const MapPoint& fromPt, const Direction dir) const
    {
        // Wenn ein Weg da drüber geht, dürfen wir das sowieso, aber kein Wasserweg!
        unsigned char road = world.GetPointRoad(fromPt, dir);
        if(road && road != RoadSegment::RT_BOAT + 1)
            return true;

        // Check terrain for node transition
        TerrainBQ bq1 = TerrainData::GetBuildingQuality(world.GetRightTerrain(fromPt, dir));
        TerrainBQ bq2 = TerrainData::GetBuildingQuality(world.GetLeftTerrain(fromPt, dir));
        // Don't go next to danger terrain
        return (bq1 != TerrainBQ::DANGER && bq2 != TerrainBQ::DANGER);
    }
};

#endif // PathConditionHuman_h__
