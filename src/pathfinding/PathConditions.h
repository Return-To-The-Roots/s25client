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

#include "gameData/TerrainData.h"

struct PathConditionRoad
{
    const GameWorldBase& gwb;
    const bool isBoatRoad;

    PathConditionRoad(const GameWorldBase& gwb, const bool isBoatRoad): gwb(gwb), isBoatRoad(isBoatRoad){}

    // Called first for every node but the goal
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt, const unsigned char dir) const
    {
        // Auch auf unserem Territorium?
        if(!gwb.IsPlayerTerritory(pt))
            return false;

        // Feld bebaubar?
        if(!gwb.RoadAvailable(isBoatRoad, pt, dir))
            return false;

        return true;
    }

    // Called for every node
    FORCE_INLINE bool IsNodeToDestOk(const MapPoint& pt, const unsigned char dir) const
    {
        return true;
    }
};

struct PathConditionHuman
{
    const GameWorldBase& gwb;

    PathConditionHuman(const GameWorldBase& gwb): gwb(gwb){}

    // Called first for every node but the goal
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt, const unsigned char dir) const
    {
        // Feld passierbar?
        noBase::BlockingManner bm = gwb.GetNO(pt)->GetBM();
        if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
            return false;

        return true;
    }

    // Called for every node
    FORCE_INLINE bool IsNodeToDestOk(const MapPoint& pt, const unsigned char dir) const
    {
        // Feld passierbar?
        // Nicht über Wasser, Lava, Sümpfe gehen
        if(!gwb.IsNodeToNodeForFigure(pt, (dir + 3) % 6))
            return false;

        return true;
    }
};

struct PathConditionTrade: public PathConditionHuman
{
    const GameClientPlayer& player;

    PathConditionTrade(const GameWorldBase& gwb, const unsigned char player): PathConditionHuman(gwb), player(gwb.GetPlayer(player)){}

    // Called for every node
    FORCE_INLINE bool IsNodeToDestOk(const MapPoint& pt, const unsigned char dir) const
    {
        if(!PathConditionHuman::IsNodeToDestOk(pt, dir))
            return false;

        unsigned char owner = gwb.GetNode(pt).owner;
        // Ally or no player? Then ok
        return (owner == 0 || player.IsAlly(owner - 1));
    }
};

struct PathConditionShip
{
    const GameWorldBase& gwb;

    PathConditionShip(const GameWorldBase& gwb): gwb(gwb){}

    // Called first for every node but the goal
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt, const unsigned char dir) const
    {
        // Ein Meeresfeld?
        for(unsigned i = 0; i < 6; ++i)
        {
            if(!TerrainData::IsUsableByShip(gwb.GetTerrainAround(pt, i)))
                return false;
        }

        return true;
    }

    // Called for every node
    FORCE_INLINE bool IsNodeToDestOk(const MapPoint& pt, const unsigned char dir) const
    {
        // Der Übergang muss immer aus Wasser sein zu beiden Seiten
        const unsigned char reverseDir = (dir + 3) % 6;
        return TerrainData::IsUsableByShip(gwb.GetWalkingTerrain1(pt, reverseDir)) &&
               TerrainData::IsUsableByShip(gwb.GetWalkingTerrain2(pt, reverseDir));
    }
};
#endif // PathConditions_h__