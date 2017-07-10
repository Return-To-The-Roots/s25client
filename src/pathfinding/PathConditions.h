// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef PathConditions_h__
#define PathConditions_h__

#include "world/GameWorldBase.h"
#include "GamePlayer.h"
#include "nodeObjs/noBase.h"
#include "gameData/TerrainData.h"

struct PathConditionHuman
{
    const GameWorldBase& gwb;

    PathConditionHuman(const GameWorldBase& gwb): gwb(gwb){}

    // Called for every node but the start & goal and should return true, if this point is usable
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt) const
    {
        // Node blocked -> Can't go there
        const BlockingManner bm = gwb.GetNO(pt)->GetBM();
        if(bm != BlockingManner::None && bm != BlockingManner::Tree && bm != BlockingManner::Flag)
            return false;
        // If no terrain around this is usable, we can't go here
        for(unsigned dir = 0; dir < 6; ++dir)
        {
            if(TerrainData::IsUseable(gwb.GetRightTerrain(pt, Direction::fromInt(dir))))
                return true;
        }
        return false;
    }

    // Called for every node
    FORCE_INLINE bool IsEdgeOk(const MapPoint& fromPt, const Direction dir) const
    {
        // Check terrain for node transition
        return gwb.IsNodeToNodeForFigure(fromPt, dir);
    }
};

struct PathConditionTrade: public PathConditionHuman
{
    const GamePlayer& player;

    PathConditionTrade(const GameWorldBase& gwb, const unsigned char player): PathConditionHuman(gwb), player(gwb.GetPlayer(player)){}

    // Called for every node but the start & goal and should return true, if this point is usable
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt) const
    {
        if(!PathConditionHuman::IsNodeOk(pt))
            return false;

        unsigned char owner = gwb.GetNode(pt).owner;
        // Ally or no player? Then ok
        return (owner == 0 || player.IsAlly(owner - 1));
    }
};

template<class T_WorldOrViewer>
struct PathConditionRoad
{
    const T_WorldOrViewer& worldOrViewer;
    const bool isBoatRoad;

    PathConditionRoad(const T_WorldOrViewer& worldOrViewer, const bool isBoatRoad): worldOrViewer(worldOrViewer), isBoatRoad(isBoatRoad){}

    // Called for every node but the start & goal and should return true, if this point is usable
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt) const
    {
        return worldOrViewer.IsPlayerTerritory(pt) && worldOrViewer.IsRoadAvailable(isBoatRoad, pt);
    }

    // Called for every edge (node to other node)
    FORCE_INLINE bool IsEdgeOk(const MapPoint&  /*fromPt*/, const Direction /*dir*/) const
    {
        return true;
    }
};

struct PathConditionShip
{
    const World& world;

    PathConditionShip(const World& world): world(world){}

    // Called for every node but the start & goal and should return true, if this point is usable
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt) const
    {
        return world.IsSeaPoint(pt);
    }

    // Called for every node
    FORCE_INLINE bool IsEdgeOk(const MapPoint& fromPt, const Direction dir) const
    {
        // We must have shippable water on both sides
        return TerrainData::IsUsableByShip(world.GetLeftTerrain(fromPt, dir)) &&
               TerrainData::IsUsableByShip(world.GetRightTerrain(fromPt, dir));
    }
};
#endif // PathConditions_h__
