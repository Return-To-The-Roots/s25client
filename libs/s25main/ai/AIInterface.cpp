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
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "helpers/containerUtils.h"
#include "network/GameMessage_Chat.h"
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
    return prp->boat_road || gwb.GetBQ(pt, gwb.GetNode(pt).owner - 1) != BuildingQuality::Nothing;
}
} // namespace

AIInterface::AIInterface(const GameWorldBase& gwb, std::vector<gc::GameCommandPtr>& gcs, unsigned char playerID)
    : gwb(gwb), player_(gwb.GetPlayer(playerID)), gcs(gcs), playerID_(playerID)
{
    for(unsigned curHarborId = 1; curHarborId <= gwb.GetNumHarborPoints(); curHarborId++)
    {
        bool hasOtherHarbor = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const unsigned short seaId = gwb.GetSeaId(curHarborId, dir);
            if(!seaId)
                continue;

            for(unsigned otherHarborId = curHarborId + 1; otherHarborId <= gwb.GetNumHarborPoints(); otherHarborId++)
            {
                if(gwb.IsHarborAtSea(otherHarborId, seaId))
                {
                    hasOtherHarbor = true;
                    break;
                }
            }
            if(hasOtherHarbor)
                usableHarbors_.push_back(curHarborId);
        }
    }
}

AIInterface::~AIInterface() = default;

AISubSurfaceResource AIInterface::GetSubsurfaceResource(const MapPoint pt) const
{
    const Resource subres = gwb.GetNode(pt).resources;
    if(subres.getAmount() == 0u)
        return AISubSurfaceResource::Nothing;
    switch(subres.getType())
    {
        case ResourceType::Iron: return AISubSurfaceResource::Ironore;
        case ResourceType::Gold: return AISubSurfaceResource::Gold;
        case ResourceType::Coal: return AISubSurfaceResource::Coal;
        case ResourceType::Granite: return AISubSurfaceResource::Granite;
        case ResourceType::Fish: return AISubSurfaceResource::Fish;
        case ResourceType::Nothing:
        case ResourceType::Water: break;
    }
    return AISubSurfaceResource::Nothing;
}

AISurfaceResource AIInterface::GetSurfaceResource(const MapPoint pt) const
{
    const auto& node = gwb.GetNode(pt);
    NodalObjectType no = node.obj ? node.obj->GetType() : NodalObjectType::Nothing;
    DescIdx<TerrainDesc> t1 = node.t1;
    // valid terrain?
    if(gwb.GetDescription().get(t1).Is(ETerrain::Walkable))
    {
        if(no == NodalObjectType::Tree)
        {
            // exclude pineapple because it's not a real tree
            if(static_cast<const noTree*>(node.obj)->ProducesWood())
                return AISurfaceResource::Wood;
            else
                return AISurfaceResource::Blocked;
        } else if(no == NodalObjectType::Granite)
            return AISurfaceResource::Stones;
        else if(no == NodalObjectType::Nothing || no == NodalObjectType::Environment)
            return AISurfaceResource::Nothing;
        else
            return AISurfaceResource::Blocked;
    } else
        return AISurfaceResource::Blocked;
}

int AIInterface::GetResourceRating(const MapPoint pt, AIResource res) const
{
    switch(res)
    {
        case AIResource::Wood:
            if(GetSurfaceResource(pt) == AISurfaceResource::Wood)
                return RES_RADIUS[res];
            else if(IsBuildingOnNode(pt, BuildingType::Woodcutter))
                return -40;
            else if(IsBuildingOnNode(pt, BuildingType::Forester))
                return 20;
            break;
        case AIResource::Stones:
            if(GetSurfaceResource(pt) == AISurfaceResource::Stones)
                return RES_RADIUS[res];
            break;
        case AIResource::Plantspace:
            if(GetSurfaceResource(pt) == AISurfaceResource::Nothing
               && gwb.GetDescription().get(gwb.GetNode(pt).t1).IsVital())
                return RES_RADIUS[res];
            else if(IsBuildingOnNode(pt, BuildingType::Forester))
                return -40;
            else if(IsBuildingOnNode(pt, BuildingType::Farm))
                return -20;
            break;
        case AIResource::Borderland:
            if(IsOwnTerritory(pt) && !IsBorder(pt))
                return 0;
            else
            {
                const auto& desc = gwb.GetDescription();
                const auto& node = gwb.GetNode(pt);
                if(desc.get(node.t1).Is(ETerrain::Walkable) || desc.get(node.t2).Is(ETerrain::Walkable))
                    return RES_RADIUS[res];
            }
            break;
        case AIResource::Gold:
        case AIResource::Ironore:
        case AIResource::Coal:
        case AIResource::Granite:
        case AIResource::Fish:
            if(convertToNodeResource(GetSubsurfaceResource(pt)) == res)
                return RES_RADIUS[res];
            break;
    }
    return 0;
}

int AIInterface::CalcResourceValue(const MapPoint pt, AIResource res, helpers::OptionalEnum<Direction> direction,
                                   int lastval) const
{
    const unsigned resRadius = RES_RADIUS[res];
    if(!direction) // calculate complete value from scratch (3n^2+3n+1)
    {
        std::vector<MapPoint> pts = gwb.GetPointsInRadiusWithCenter(pt, resRadius);
        return std::accumulate(pts.begin(), pts.end(), 0, [this, res](int lhs, const auto& curPt) {
            return lhs + this->GetResourceRating(curPt, res);
        });
    } else // calculate different nodes only (4n+2 ?anyways much faster)
    {
        const auto iDirection = rttr::enum_cast(*direction);
        int returnVal = lastval;
        // add new points
        // first: go radius steps towards direction-1
        MapPoint tmpPt(pt);
        for(unsigned i = 0; i < resRadius; i++)
            tmpPt = gwb.GetNeighbour(tmpPt, *direction - 1u);
        // then clockwise around at radius distance to get all new points
        for(unsigned i = iDirection + 1u; i < iDirection + 3u; ++i)
        {
            int numSteps = resRadius;
            // add 1 extra step on the second side we check to complete the side
            if(i == iDirection + 2u)
                ++numSteps;
            for(MapCoord r2 = 0; r2 < numSteps; ++r2)
            {
                returnVal += GetResourceRating(tmpPt, res);
                tmpPt = gwb.GetNeighbour(tmpPt, convertToDirection(i));
            }
        }
        // now substract old points not in range of new point
        // go to old center point:
        tmpPt = pt;
        tmpPt = gwb.GetNeighbour(tmpPt, *direction + 3u);
        // next: go to the first old point we have to substract
        for(unsigned i = 0; i < RES_RADIUS[res]; i++)
            tmpPt = gwb.GetNeighbour(tmpPt, *direction + 2u);
        // now clockwise around at radius distance to remove all old points
        for(int i = iDirection + 4; i < iDirection + 6; ++i)
        {
            int numSteps = resRadius;
            if(i == iDirection + 5)
                ++numSteps;
            for(MapCoord r2 = 0; r2 < numSteps; ++r2)
            {
                returnVal -= GetResourceRating(tmpPt, res);
                tmpPt = gwb.GetNeighbour(tmpPt, convertToDirection(i));
            }
        }
        return returnVal;
    }
    // if(returnval<0&&lastval>=0&&res==AIResource::Borderland)
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

bool AIInterface::isBuildingNearby(BuildingType bldType, const MapPoint pt, unsigned maxDistance) const
{
    for(const nobUsual* bld : GetBuildings(bldType))
    {
        if(gwb.CalcDistance(pt, bld->GetPos()) <= maxDistance)
            return true;
    }
    for(const noBuildingSite* bldSite : GetBuildingSites())
    {
        if(bldSite->GetBuildingType() == bldType)
        {
            if(gwb.CalcDistance(pt, bldSite->GetPos()) <= maxDistance)
                return true;
        }
    }
    return false;
}

bool AIInterface::isHarborPosClose(const MapPoint pt, unsigned maxDistance, bool onlyempty) const
{
    // skip harbordummy
    for(unsigned i = 1; i <= gwb.GetNumHarborPoints(); i++)
    {
        const MapPoint harborPoint = gwb.GetHarborPoint(i);
        if(gwb.CalcDistance(pt, harborPoint) <= maxDistance && helpers::contains(usableHarbors_, i))
        {
            if(!onlyempty || !IsBuildingOnNode(harborPoint, BuildingType::HarborBuilding))
                return true;
        }
    }
    return false;
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

void AIInterface::Chat(const std::string& message, ChatDestination destination)
{
    pendingChatMsgs_.push_back(std::make_unique<GameMessage_Chat>(playerID_, destination, message));
}

std::vector<std::unique_ptr<GameMessage_Chat>> AIInterface::FetchChatMessages()
{
    std::vector<std::unique_ptr<GameMessage_Chat>> tmp;
    std::swap(tmp, pendingChatMsgs_);
    return tmp;
}
