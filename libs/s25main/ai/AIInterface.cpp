// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIInterface.h"
#include "buildings/noBuilding.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "helpers/IdRange.h"
#include "helpers/containerUtils.h"
#include "network/GameMessage_Chat.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/PathConditionRoad.h"
#include "pathfinding/RoadPathFinder.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "gameTypes/MineResourceBehavior.h"
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
    return makePathConditionRoad(gwb, prp->boat_road).isNodeOk(pt);
}

/// Condition for comfort road construction with a possible flag every 2 steps
bool IsPointOK_RoadPathEvenStep(const GameWorldBase& gwb, const MapPoint pt, const Direction dir, const void* param)
{
    if(!IsPointOK_RoadPath(gwb, pt, dir, param))
        return false;
    const auto* prp = static_cast<const Param_RoadPath*>(param);
    return prp->boat_road || gwb.GetBQ(pt, gwb.getNode(pt).owner - 1) != BuildingQuality::Nothing;
}

int GetS4LikeMineResourceRating(const Resource resource, const unsigned defaultRating)
{
    if(resource.getAmount() == 0u)
        return 0;

    return std::max(1u, std::min(static_cast<unsigned>(resource.getAmount()), defaultRating));
}
} // namespace

AIInterface::AIInterface(const GameWorldBase& gwb, std::vector<gc::GameCommandPtr>& gcs, unsigned char playerID)
    : gwb(gwb), player_(gwb.getPlayer(playerID)), gcs(gcs), playerID_(playerID)
{
    for(const auto curHarborId : helpers::idRange<HarborId>(gwb.getNumHarborPoints()))
    {
        bool hasOtherHarbor = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const SeaId seaId = gwb.getSeaId(curHarborId, dir);
            if(!seaId)
                continue;

            for(const auto otherHarborId : helpers::idRangeAfter(curHarborId, gwb.getNumHarborPoints()))
            {
                if(gwb.isHarborAtSea(otherHarborId, seaId))
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

AISubSurfaceResource AIInterface::getSubsurfaceResource(const MapPoint pt) const
{
    const Resource subres = gwb.getNode(pt).resources;
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

AISurfaceResource AIInterface::getSurfaceResource(const MapPoint pt) const
{
    const auto& node = gwb.getNode(pt);
    NodalObjectType no = node.obj ? node.obj->getType() : NodalObjectType::Nothing;
    DescIdx<TerrainDesc> t1 = node.t1;
    // valid terrain?
    if(gwb.getDescription().get(t1).Is(ETerrain::Walkable))
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

int AIInterface::getResourceRating(const MapPoint pt, AIResource res) const
{
    switch(res)
    {
        case AIResource::Wood:
            if(getSurfaceResource(pt) == AISurfaceResource::Wood)
                return RES_RADIUS[res];
            else if(isBuildingOnNode(pt, BuildingType::Woodcutter))
                return -40;
            else if(isBuildingOnNode(pt, BuildingType::Forester))
                return 20;
            break;
        case AIResource::Stones:
            if(getSurfaceResource(pt) == AISurfaceResource::Stones)
                return RES_RADIUS[res];
            break;
        case AIResource::Plantspace:
            if(getSurfaceResource(pt) == AISurfaceResource::Nothing
               && gwb.getDescription().get(gwb.getNode(pt).t1).isVital())
                return RES_RADIUS[res];
            else if(isBuildingOnNode(pt, BuildingType::Forester))
                return -40;
            else if(isBuildingOnNode(pt, BuildingType::Farm))
                return -20;
            break;
        case AIResource::Borderland:
            if(isOwnTerritory(pt) && !isBorder(pt))
                return 0;
            else
            {
                const auto& desc = gwb.getDescription();
                const auto& node = gwb.getNode(pt);
                if(desc.get(node.t1).Is(ETerrain::Walkable) || desc.get(node.t2).Is(ETerrain::Walkable))
                    return RES_RADIUS[res];
            }
            break;
        case AIResource::Gold:
        case AIResource::Ironore:
        case AIResource::Coal:
        case AIResource::Granite:
        {
            const Resource subres = gwb.getNode(pt).resources;
            if(convertToNodeResource(getSubsurfaceResource(pt)) == res)
            {
                const auto mineBuildingType = GetMineBuildingType(subres.getType());
                if(mineBuildingType
                   && getMineResourceBehavior(gwb.getGGS(), *mineBuildingType)
                        == MineResourceBehavior::S4LikeExhaustion)
                    return GetS4LikeMineResourceRating(subres, RES_RADIUS[res]);

                return RES_RADIUS[res];
            }
            if(isMineResourceWorkEverywhere(res)
               && gwb.isOfTerrain(pt, [](const TerrainDesc& desc) { return desc.Is(ETerrain::Mineable); }))
                return RES_RADIUS[res];
            break;
        }
        case AIResource::Fish:
            if(convertToNodeResource(getSubsurfaceResource(pt)) == res)
                return RES_RADIUS[res];
            break;
    }
    return 0;
}

bool AIInterface::isMineResourceWorkEverywhere(const AIResource res) const
{
    const auto resourceType = convertToResourceType(res);
    if(!resourceType)
        return false;

    const auto mineBuildingType = GetMineBuildingType(*resourceType);
    return mineBuildingType
           && getMineResourceBehavior(gwb.getGGS(), *mineBuildingType) == MineResourceBehavior::WorkEverywhere;
}

int AIInterface::CalcResourceValue(const MapPoint pt, AIResource res, helpers::OptionalEnum<Direction> direction,
                                   int lastval) const
{
    const unsigned resRadius = RES_RADIUS[res];
    if(!direction) // calculate complete value from scratch (3n^2+3n+1)
    {
        std::vector<MapPoint> pts = gwb.GetPointsInRadiusWithCenter(pt, resRadius);
        return std::accumulate(pts.begin(), pts.end(), 0, [this, res](int lhs, const auto& curPt) {
            return lhs + this->getResourceRating(curPt, res);
        });
    } else // calculate different nodes only (4n+2 ?anyways much faster)
    {
        const auto iDirection = rttr::enum_cast(*direction);
        int returnVal = lastval;
        // add new points
        // first: go radius steps towards direction-1
        MapPoint tmpPt(pt);
        for(unsigned i = 0; i < resRadius; i++)
            tmpPt = gwb.getNeighbour(tmpPt, *direction - 1u);
        // then clockwise around at radius distance to get all new points
        for(unsigned i = iDirection + 1u; i < iDirection + 3u; ++i)
        {
            int numSteps = resRadius;
            // add 1 extra step on the second side we check to complete the side
            if(i == iDirection + 2u)
                ++numSteps;
            for(MapCoord r2 = 0; r2 < numSteps; ++r2)
            {
                returnVal += getResourceRating(tmpPt, res);
                tmpPt = gwb.getNeighbour(tmpPt, convertToDirection(i));
            }
        }
        // now substract old points not in range of new point
        // go to old center point:
        tmpPt = pt;
        tmpPt = gwb.getNeighbour(tmpPt, *direction + 3u);
        // next: go to the first old point we have to substract
        for(unsigned i = 0; i < RES_RADIUS[res]; i++)
            tmpPt = gwb.getNeighbour(tmpPt, *direction + 2u);
        // now clockwise around at radius distance to remove all old points
        for(int i = iDirection + 4; i < iDirection + 6; ++i)
        {
            int numSteps = resRadius;
            if(i == iDirection + 5)
                ++numSteps;
            for(MapCoord r2 = 0; r2 < numSteps; ++r2)
            {
                returnVal -= getResourceRating(tmpPt, res);
                tmpPt = gwb.getNeighbour(tmpPt, convertToDirection(i));
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
    return gwb.getFreePathFinder().FindPathAlternatingConditions(start, target, false, 100, route, length, nullptr,
                                                                 IsPointOK_RoadPath, IsPointOK_RoadPathEvenStep,
                                                                 nullptr, (void*)&boat);
}

bool AIInterface::CalcBQSumDifference(const MapPoint pt1, const MapPoint pt2) const
{
    return getBuildingQuality(pt2) < getBuildingQuality(pt1);
}

BuildingQuality AIInterface::getBuildingQuality(const MapPoint pt) const
{
    return gwb.GetBQ(pt, playerID_);
}

BuildingQuality AIInterface::getBuildingQualityAnyOwner(const MapPoint pt) const
{
    return gwb.getNode(pt).bq;
}

bool AIInterface::FindPathOnRoads(const noRoadNode& start, const noRoadNode& target, unsigned* length) const
{
    if(length)
        return gwb.getRoadPathFinder().FindPath(start, target, false, std::numeric_limits<unsigned>::max(), nullptr,
                                                length);
    else
        return gwb.getRoadPathFinder().PathExists(start, target, false);
}

const nobHQ* AIInterface::getHeadquarter() const
{
    return gwb.getSpecObj<nobHQ>(player_.getHQPos());
}

bool AIInterface::isBuildingNearby(BuildingType bldType, const MapPoint pt, unsigned maxDistance) const
{
    for(const nobUsual* bld : getBuildings(bldType))
    {
        if(gwb.CalcDistance(pt, bld->getPos()) <= maxDistance)
            return true;
    }
    for(const noBuildingSite* bldSite : getBuildingSites())
    {
        if(bldSite->getBuildingType() == bldType)
        {
            if(gwb.CalcDistance(pt, bldSite->getPos()) <= maxDistance)
                return true;
        }
    }
    return false;
}

bool AIInterface::isHarborPosClose(const MapPoint pt, unsigned maxDistance, bool onlyempty) const
{
    for(const auto id : helpers::idRange<HarborId>(gwb.getNumHarborPoints()))
    {
        const MapPoint harborPoint = gwb.getHarborPoint(id);
        if(gwb.CalcDistance(pt, harborPoint) <= maxDistance && helpers::contains(usableHarbors_, id))
        {
            if(!onlyempty || !isBuildingOnNode(harborPoint, BuildingType::HarborBuilding))
                return true;
        }
    }
    return false;
}

bool AIInterface::isExplorationDirectionPossible(const MapPoint pt, const nobHarborBuilding* originHarbor,
                                                 ShipDirection direction) const
{
    return isExplorationDirectionPossible(pt, originHarbor->getHarborPosID(), direction);
}

bool AIInterface::isExplorationDirectionPossible(const MapPoint pt, HarborId originHarborID,
                                                 ShipDirection direction) const
{
    return gwb.getNextFreeHarborPoint(pt, originHarborID, direction, playerID_).isValid();
}

bool AIInterface::SetCoinsAllowed(const nobMilitary* building, const bool enabled)
{
    return SetCoinsAllowed(building->getPos(), enabled);
}
bool AIInterface::StartStopExpedition(const nobHarborBuilding* hb, bool start)
{
    return StartStopExpedition(hb->getPos(), start);
}
bool AIInterface::SetShipYardMode(const nobShipYard* shipyard, bool buildShips)
{
    return SetShipYardMode(shipyard->getPos(), buildShips);
}
bool AIInterface::DestroyBuilding(const noBuilding* building)
{
    return DestroyBuilding(building->getPos());
}
bool AIInterface::DestroyFlag(const noFlag* flag)
{
    return DestroyFlag(flag->getPos());
}
bool AIInterface::CallSpecialist(const noFlag* flag, Job job)
{
    return CallSpecialist(flag->getPos(), job);
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
