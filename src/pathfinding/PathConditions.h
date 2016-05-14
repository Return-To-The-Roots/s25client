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
        // Feld passierbar?
        noBase::BlockingManner bm = gwb.GetNO(pt)->GetBM();
        return bm == noBase::BM_NOTBLOCKING || bm == noBase::BM_TREE || bm == noBase::BM_FLAG;
    }

    // Called for every node
    FORCE_INLINE bool IsEdgeOk(const MapPoint& fromPt, const unsigned char dir) const
    {
        // Feld passierbar?
        // Nicht �ber Wasser, Lava, S�mpfe gehen
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

struct PathConditionShip
{
    const GameWorldBase& gwb;

    PathConditionShip(const GameWorldBase& gwb): gwb(gwb){}

    // Called for every node but the start & goal and should return true, if this point is usable
    FORCE_INLINE bool IsNodeOk(const MapPoint& pt) const
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
    FORCE_INLINE bool IsEdgeOk(const MapPoint& fromPt, const unsigned char dir) const
    {
        // Der �bergang muss immer aus Wasser sein zu beiden Seiten
        return TerrainData::IsUsableByShip(gwb.GetWalkingTerrain1(fromPt, dir)) &&
               TerrainData::IsUsableByShip(gwb.GetWalkingTerrain2(fromPt, dir));
    }
};
#endif // PathConditions_h__
