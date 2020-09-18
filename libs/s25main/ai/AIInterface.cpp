// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "AIInterface.h"
#include "buildings/noBuilding.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/PathConditionRoad.h"
#include "pathfinding/RoadPathFinder.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "gameData/TerrainDesc.h"
#include <limits>
#include <numeric>

class noRoadNode;

namespace {
/// Param for road-build pathfinding
struct Param_RoadPath
{
    /// Boat or normal road
    bool boat_road;
};

bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapPoint pt, const Direction, const void* param)
{
    const auto* prp = static_cast<const Param_RoadPath*>(param);
    return makePathConditionRoad(gwb, prp->boat_road).IsNodeOk(pt);
}

/// Condition for comfort road construction with a possible flag every 2 steps
bool IsPointOK_RoadPathEvenStep(const GameWorldBase& gwb, const MapPoint pt, const Direction dir, const void* param)
{
    if(!IsPointOK_RoadPath(gwb, pt, dir, param))
        return false;
    const auto* prp = static_cast<const Param_RoadPath*>(param);
    return prp->boat_road || gwb.GetBQ(pt, gwb.GetNode(pt).owner - 1) != BQ_NOTHING;
}
} // namespace

AIResource AIInterface::GetSubsurfaceResource(const MapPoint pt) const
{
    Resource subres = gwb.GetNode(pt).resources;
    if(subres.getAmount() == 0u)
        return AIResource::NOTHING;
    switch(subres.getType())
    {
        case Resource::Iron: return AIResource::IRONORE;
        case Resource::Gold: return AIResource::GOLD;
        case Resource::Coal: return AIResource::COAL;
        case Resource::Granite: return AIResource::GRANITE;
        case Resource::Fish: return AIResource::FISH;
        default: break;
    }
    return AIResource::NOTHING;
}

AIResource AIInterface::GetSurfaceResource(const MapPoint pt) const
{
    NodalObjectType no = gwb.GetNO(pt)->GetType();
    DescIdx<TerrainDesc> t1 = gwb.GetNode(pt).t1;
    // valid terrain?
    if(gwb.GetDescription().get(t1).Is(ETerrain::Walkable))
    {
        if(no == NOP_TREE)
        {
            // exclude pineapple because it's not a real tree
            if(gwb.GetSpecObj<noTree>(pt)->ProducesWood())
                return AIResource::WOOD;
            else
                return AIResource::BLOCKED;
        } else if(no == NOP_GRANITE)
            return AIResource::STONES;
        else if(no == NOP_NOTHING || no == NOP_ENVIRONMENT)
            return AIResource::NOTHING;
        else
            return AIResource::BLOCKED;
    } else
        return AIResource::BLOCKED;
}

int AIInterface::GetResourceRating(const MapPoint pt, AIResource res) const
{
    // surface resource?
    if(res == AIResource::PLANTSPACE || res == AIResource::BORDERLAND || res == AIResource::WOOD
       || res == AIResource::STONES)
    {
        AIResource surfaceRes = GetSurfaceResource(pt);
        DescIdx<TerrainDesc> t1 = gwb.GetNode(pt).t1, t2 = gwb.GetNode(pt).t2;
        if(surfaceRes == res
           || (res == AIResource::PLANTSPACE && surfaceRes == AIResource::NOTHING
               && gwb.GetDescription().get(t1).IsVital())
           || (res == AIResource::BORDERLAND && (IsBorder(pt) || !IsOwnTerritory(pt))
               && (gwb.GetDescription().get(t1).Is(ETerrain::Walkable)
                   || gwb.GetDescription().get(t2).Is(ETerrain::Walkable))))
        {
            return RES_RADIUS[static_cast<unsigned>(res)];
        }
        // Adjust based on building on node (if any)
        if(res == AIResource::WOOD)
        {
            if(IsBuildingOnNode(pt, BLD_WOODCUTTER))
                return -40;
            if(IsBuildingOnNode(pt, BLD_FORESTER))
                return 20;
        } else if(res == AIResource::PLANTSPACE)
        {
            if(IsBuildingOnNode(pt, BLD_FORESTER))
                return -40;
            if(IsBuildingOnNode(pt, BLD_FARM))
                return -20;
        }
    }
    // so it's a subsurface resource or something we dont calculate (multiple,blocked,nothing)
    else
    {
        if(GetSubsurfaceResource(pt) == res)
            return RES_RADIUS[static_cast<unsigned>(res)];
    }
    return 0;
}

int AIInterface::CalcResourceValue(const MapPoint pt, AIResource res, int8_t direction, int lastval) const
{
    if(direction == -1) // calculate complete value from scratch (3n^2+3n+1)
    {
        std::vector<MapPoint> pts = gwb.GetPointsInRadiusWithCenter(pt, RES_RADIUS[static_cast<unsigned>(res)]);
        return std::accumulate(pts.begin(), pts.end(), 0, [this, res](int lhs, const auto& curPt) {
            return lhs + this->GetResourceRating(curPt, res);
        });
    } else // calculate different nodes only (4n+2 ?anyways much faster)
    {
        int returnVal = lastval;
        // add new points
        // first: go radius steps towards direction-1
        MapPoint tmpPt(pt);
        for(unsigned i = 0; i < RES_RADIUS[static_cast<unsigned>(res)]; i++)
            tmpPt = gwb.GetNeighbour(tmpPt, Direction(direction + 5));
        // then clockwise around at radius distance to get all new points
        for(int i = direction + 1; i < (direction + 3); ++i)
        {
            int resRadius = RES_RADIUS[static_cast<unsigned>(res)];
            // add 1 extra step on the second side we check to complete the side
            if(i == direction + 2)
                ++resRadius;
            for(MapCoord r2 = 0; r2 < resRadius; ++r2)
            {
                returnVal += GetResourceRating(tmpPt, res);
                tmpPt = gwb.GetNeighbour(tmpPt, Direction(i));
            }
        }
        // now substract old points not in range of new point
        // go to old center point:
        tmpPt = pt;
        tmpPt = gwb.GetNeighbour(tmpPt, Direction(direction + 3));
        // next: go to the first old point we have to substract
        for(unsigned i = 0; i < RES_RADIUS[static_cast<unsigned>(res)]; i++)
            tmpPt = gwb.GetNeighbour(tmpPt, Direction(direction + 2));
        // now clockwise around at radius distance to remove all old points
        for(int i = direction + 4; i < (direction + 6); ++i)
        {
            int resRadius = RES_RADIUS[static_cast<unsigned>(res)];
            if(i == direction + 5)
                ++resRadius;
            for(MapCoord r2 = 0; r2 < resRadius; ++r2)
            {
                returnVal -= GetResourceRating(tmpPt, res);
                tmpPt = gwb.GetNeighbour(tmpPt, Direction(i));
            }
        }
        return returnVal;
    }
    // if(returnval<0&&lastval>=0&&res==AIResource::BORDERLAND)
    // LOG.write(("AIInterface::CalcResourceValue - warning: negative returnvalue direction %i oldval %i\n", direction,
    // lastval);
}

bool AIInterface::FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<Direction>* route /*= nullptr*/,
                                         unsigned* length /*= nullptr*/) const
{
    bool boat = false;
    return gwb.GetFreePathFinder().FindPathAlternatingConditions(start, target, false, 100, route, length, nullptr,
                                                                 IsPointOK_RoadPath, IsPointOK_RoadPathEvenStep,
                                                                 nullptr, (void*)&boat);
}

bool AIInterface::CalcBQSumDifference(const MapPoint pt1, const MapPoint pt2) const
{
    return GetBuildingQuality(pt2) < GetBuildingQuality(pt1);
}

BuildingQuality AIInterface::GetBuildingQuality(const MapPoint pt) const
{
    return gwb.GetBQ(pt, playerID_);
}

BuildingQuality AIInterface::GetBuildingQualityAnyOwner(const MapPoint pt) const
{
    return gwb.GetNode(pt).bq;
}

bool AIInterface::FindPathOnRoads(const noRoadNode& start, const noRoadNode& target, unsigned* length) const
{
    if(length)
        return gwb.GetRoadPathFinder().FindPath(start, target, false, std::numeric_limits<unsigned>::max(), nullptr,
                                                length);
    else
        return gwb.GetRoadPathFinder().PathExists(start, target, false);
}

const nobHQ* AIInterface::GetHeadquarter() const
{
    return gwb.GetSpecObj<nobHQ>(player_.GetHQPos());
}

bool AIInterface::IsExplorationDirectionPossible(const MapPoint pt, const nobHarborBuilding* originHarbor,
                                                 ShipDirection direction) const
{
    return IsExplorationDirectionPossible(pt, originHarbor->GetHarborPosID(), direction);
}

bool AIInterface::IsExplorationDirectionPossible(const MapPoint pt, unsigned originHarborID,
                                                 ShipDirection direction) const
{
    return gwb.GetNextFreeHarborPoint(pt, originHarborID, direction, playerID_) > 0;
}

bool AIInterface::SetCoinsAllowed(const nobMilitary* building, const bool enabled)
{
    return SetCoinsAllowed(building->GetPos(), enabled);
}
bool AIInterface::StartStopExpedition(const nobHarborBuilding* hb, bool start)
{
    return StartStopExpedition(hb->GetPos(), start);
}
bool AIInterface::SetShipYardMode(const nobShipYard* shipyard, bool buildShips)
{
    return SetShipYardMode(shipyard->GetPos(), buildShips);
}
bool AIInterface::DestroyBuilding(const noBuilding* building)
{
    return DestroyBuilding(building->GetPos());
}
bool AIInterface::DestroyFlag(const noFlag* flag)
{
    return DestroyFlag(flag->GetPos());
}
bool AIInterface::CallSpecialist(const noFlag* flag, Job job)
{
    return CallSpecialist(flag->GetPos(), job);
}
