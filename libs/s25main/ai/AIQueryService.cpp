// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIQueryService.h"

#include "EventManager.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "helpers/containerUtils.h"
#include "nodeObjs/noRoadNode.h"
#include "nodeObjs/noTree.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/PathConditionRoad.h"
#include "pathfinding/RoadPathFinder.h"
#include "gameData/TerrainDesc.h"
#include "world/BQCalculator.h"
#include <limits>
#include <numeric>
#include <set>

namespace {

constexpr unsigned kDefaultTTL      = 1'000;
constexpr unsigned kFishTTL         = 10'000;
constexpr unsigned kBorderlandTTL   = 30'000;
constexpr unsigned kPermanentTTL    = 1'000'000;
constexpr unsigned kEvictEveryNMisses         = 4096;
constexpr size_t   kResourceValueCacheHardCap = 200'000;

unsigned GetCacheTTL(AIResource res, int value)
{
    switch(res)
    {
        case AIResource::Fish:
            return kFishTTL;
        case AIResource::Gold:
        case AIResource::Ironore:
        case AIResource::Coal:
        case AIResource::Granite:
        case AIResource::Stones:
            return (value == 0) ? kPermanentTTL : kDefaultTTL;
        case AIResource::Borderland:
            return kBorderlandTTL;
        default: // Wood, Plantspace
            return kDefaultTTL;
    }
}

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

unsigned GetBuildingQualityPenaltyValue(BuildingQuality bq)
{
    switch(bq)
    {
        case BuildingQuality::Nothing: return 0;
        case BuildingQuality::Flag: return 1;
        case BuildingQuality::Hut: return 2;
        case BuildingQuality::House: return 3;
        case BuildingQuality::Mine: return 3;
        case BuildingQuality::Castle: return 4;
        case BuildingQuality::Harbor: return 4;
    }
    return 0;
}

unsigned GetBuildingQualityPenalty(BuildingQuality before, BuildingQuality after)
{
    const unsigned beforeValue = GetBuildingQualityPenaltyValue(before);
    const unsigned afterValue = GetBuildingQualityPenaltyValue(after);
    return (beforeValue > afterValue) ? (beforeValue - afterValue) : 0;
}
} // namespace

AIQueryService::AIQueryService(const GameWorldBase& gwb, unsigned char playerID)
    : gwb(gwb), player_(gwb.GetPlayer(playerID)), playerID_(playerID)
{
    InitUsableHarbors();
}

AIQueryService::~AIQueryService() = default;

void AIQueryService::InitUsableHarbors()
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

AISubSurfaceResource AIQueryService::GetSubsurfaceResource(const MapPoint pt) const
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

AISurfaceResource AIQueryService::GetSurfaceResource(const MapPoint pt) const
{
    const auto& node = gwb.GetNode(pt);
    NodalObjectType no = node.obj ? node.obj->GetType() : NodalObjectType::Nothing;
    DescIdx<TerrainDesc> t1 = node.t1;
    if(gwb.GetDescription().get(t1).Is(ETerrain::Walkable))
    {
        if(no == NodalObjectType::Tree)
        {
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

int AIQueryService::GetResourceRating(const MapPoint pt, AIResource res) const
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

int AIQueryService::ComputeResourceValueUncached(const MapPoint pt, AIResource res,
                                                  helpers::OptionalEnum<Direction> direction, int lastval) const
{
    const unsigned resRadius = RES_RADIUS[res];
    if(!direction)
    {
        std::vector<MapPoint> pts = gwb.GetPointsInRadiusWithCenter(pt, resRadius);
        return std::accumulate(pts.begin(), pts.end(), 0, [this, res](int lhs, const auto& curPt) {
            return lhs + this->GetResourceRating(curPt, res);
        });
    } else
    {
        const auto iDirection = rttr::enum_cast(*direction);
        int returnVal = lastval;
        MapPoint tmpPt(pt);
        for(unsigned i = 0; i < resRadius; i++)
            tmpPt = gwb.GetNeighbour(tmpPt, *direction - 1u);
        for(unsigned i = iDirection + 1u; i < iDirection + 3u; ++i)
        {
            int numSteps = resRadius;
            if(i == iDirection + 2u)
                ++numSteps;
            for(MapCoord r2 = 0; r2 < numSteps; ++r2)
            {
                returnVal += GetResourceRating(tmpPt, res);
                tmpPt = gwb.GetNeighbour(tmpPt, convertToDirection(i));
            }
        }
        tmpPt = pt;
        tmpPt = gwb.GetNeighbour(tmpPt, *direction + 3u);
        for(unsigned i = 0; i < RES_RADIUS[res]; i++)
            tmpPt = gwb.GetNeighbour(tmpPt, *direction + 2u);
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
}

int AIQueryService::CalcResourceValue(const MapPoint pt, AIResource res, helpers::OptionalEnum<Direction> direction,
                                      int lastval) const
{
    if(direction)
        return ComputeResourceValueUncached(pt, res, direction, lastval);

    const unsigned currentGF = gwb.GetEvMgr().GetCurrentGF();
    const CacheKey key{pt, res};

    auto it = resourceValueCache_.find(key);
    if(it != resourceValueCache_.end() && currentGF < it->second.expiresAtGF)
    {
        ++resourceValueCacheHits_;
        return it->second.value;
    }

    MaybeEvictExpired(currentGF);

    const int value = ComputeResourceValueUncached(pt, res, boost::none, 0xffff);
    resourceValueCache_[key] = {value, currentGF + GetCacheTTL(res, value)};
    ++resourceValueCacheMisses_;
    return value;
}

void AIQueryService::MaybeEvictExpired(unsigned currentGF) const
{
    if(++resourceValueCacheMissesSinceLastEvict_ < kEvictEveryNMisses
       && resourceValueCache_.size() <= kResourceValueCacheHardCap)
        return;
    resourceValueCacheMissesSinceLastEvict_ = 0;

    for(auto it = resourceValueCache_.begin(); it != resourceValueCache_.end();)
    {
        if(currentGF >= it->second.expiresAtGF)
            it = resourceValueCache_.erase(it);
        else
            ++it;
    }

    if(resourceValueCache_.size() > kResourceValueCacheHardCap)
        resourceValueCache_.clear();
}

void AIQueryService::InvalidateResourceValueInRadius(const MapPoint center, const AIResource res,
                                                     const unsigned radius)
{
    if(resourceValueCache_.empty())
        return;
    for(const MapPoint& pt : gwb.GetPointsInRadiusWithCenter(center, radius))
        resourceValueCache_.erase(CacheKey{pt, res});
}

bool AIQueryService::IsBorder(const MapPoint pt) const
{
    return gwb.GetNode(pt).boundary_stones[BorderStonePos::OnPoint] == (playerID_ + 1);
}

bool AIQueryService::IsOwnTerritory(const MapPoint pt) const
{
    return gwb.GetNode(pt).owner == (playerID_ + 1);
}

bool AIQueryService::IsRoad(const MapPoint pt, Direction dir) const
{
    return gwb.GetPointRoad(pt, dir) != PointRoad::None;
}

bool AIQueryService::IsObjectTypeOnNode(const MapPoint pt, NodalObjectType objectType) const
{
    return gwb.GetNO(pt)->GetType() == objectType;
}

bool AIQueryService::IsBuildingOnNode(const MapPoint pt, BuildingType bld) const
{
    const noBase* no = gwb.GetNO(pt);
    const NodalObjectType noType = no->GetType();
    return (noType == NodalObjectType::Building || noType == NodalObjectType::Buildingsite)
           && (static_cast<const noBaseBuilding*>(no)->GetBuildingType() == bld);
}

bool AIQueryService::IsVisible(const MapPoint pt) const
{
    return gwb.CalcVisiblityWithAllies(pt, playerID_) == Visibility::Visible;
}

bool AIQueryService::CalcBQSumDifference(const MapPoint pt1, const MapPoint pt2) const
{
    return GetBuildingQuality(pt2) < GetBuildingQuality(pt1);
}

BuildingQuality AIQueryService::GetBuildingQuality(const MapPoint pt) const
{
    return gwb.GetBQ(pt, playerID_);
}

BuildingQuality AIQueryService::GetBuildingQualityAnyOwner(const MapPoint pt) const
{
    return gwb.GetNode(pt).bq;
}

bool AIQueryService::IsReservedMilitaryBorderSlot(const MapPoint pt, const BuildingQuality currentBQ) const
{
    if(!reserveMilitaryBorderSlots_ || !IsOwnTerritory(pt))
        return false;

    const int borderlandLevel = CalcResourceValue(pt, AIResource::Borderland);
    if(borderlandLevel <= static_cast<int>(reserveMilitaryBorderlandThreshold_))
        return false;

    for(const BuildingType militaryType : BuildingProperties::militaryBldTypes)
    {
        if(!CanBuildBuildingtype(militaryType))
            continue;
        if(canUseBq(currentBQ, BUILDING_SIZE[militaryType]))
            return true;
    }

    return false;
}

unsigned AIQueryService::EstimateBuildLocationBQPenalty(const MapPoint buildingPos) const
{
    const BQCalculator bqCalculator(gwb);
    const auto isOnRoad = [&](MapPoint pt) { return gwb.IsOnRoad(pt); };
    const auto getHypotheticalBlockingManner = [&](MapPoint pt) {
        return (pt == buildingPos) ? BlockingManner::Building : gwb.GetNO(pt)->GetBM();
    };

    unsigned penalty = 0;
    for(const MapPoint nb : gwb.GetNeighbours(buildingPos))
    {
        const BuildingQuality beforeBQ = gwb.GetBQ(nb, playerID_);
        const BuildingQuality afterNodeBQ = bqCalculator(nb, isOnRoad, getHypotheticalBlockingManner, false);
        const BuildingQuality afterBQ = gwb.AdjustBQ(nb, playerID_, afterNodeBQ);
        penalty += GetBuildingQualityPenalty(beforeBQ, afterBQ);
    }

    return penalty;
}

unsigned AIQueryService::EstimateRoadRouteBQPenalty(MapPoint start, const std::vector<Direction>& route) const
{
    if(route.empty())
        return 0;

    std::set<MapPoint, MapPointLess> routePoints;
    std::set<MapPoint, MapPointLess> affectedPoints;

    auto addAffectedPoint = [&](MapPoint pt) {
        routePoints.insert(pt);
        affectedPoints.insert(pt);
        affectedPoints.insert(gwb.GetNeighbour(pt, Direction::East));
        affectedPoints.insert(gwb.GetNeighbour(pt, Direction::SouthEast));
        affectedPoints.insert(gwb.GetNeighbour(pt, Direction::SouthWest));
    };

    addAffectedPoint(start);
    for(const Direction dir : route)
    {
        start = gwb.GetNeighbour(start, dir);
        addAffectedPoint(start);
    }

    const BQCalculator bqCalculator(gwb);
    const auto isOnHypotheticalRoad = [&](MapPoint pt) { return gwb.IsOnRoad(pt) || routePoints.count(pt) > 0; };

    unsigned penalty = 0;
    for(const MapPoint pt : affectedPoints)
    {
        const BuildingQuality beforeBQ = gwb.GetBQ(pt, playerID_);
        const BuildingQuality afterNodeBQ = bqCalculator(pt, isOnHypotheticalRoad);
        const BuildingQuality afterBQ = gwb.AdjustBQ(pt, playerID_, afterNodeBQ);
        penalty += GetBuildingQualityPenalty(beforeBQ, afterBQ);
    }

    return penalty;
}

bool AIQueryService::FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<Direction>* route,
                                            unsigned* length) const
{
    bool boat = false;
    return gwb.GetFreePathFinder().FindPathAlternatingConditions(start, target, false, 100, route, length, nullptr,
                                                                 IsPointOK_RoadPath, IsPointOK_RoadPathEvenStep,
                                                                 nullptr, (void*)&boat);
}

bool AIQueryService::FindPathOnRoads(const noRoadNode& start, const noRoadNode& target, unsigned* length) const
{
    if(length)
        return gwb.GetRoadPathFinder().FindPath(start, target, false, std::numeric_limits<unsigned>::max(), nullptr,
                                                length);
    else
        return gwb.GetRoadPathFinder().PathExists(start, target, false);
}

bool AIQueryService::CanBuildCatapult() const
{
    return player_.CanBuildCatapult();
}

bool AIQueryService::CanBuildBuildingtype(BuildingType bt) const
{
    return player_.IsBuildingEnabled(bt);
}

bool AIQueryService::IsPlayerAttackable(unsigned char playerID) const
{
    return player_.IsAttackable(playerID);
}

const nobHQ* AIQueryService::GetHeadquarter() const
{
    const MapPoint hqPos = player_.GetHQPos();
    if(!hqPos.isValid())
        return nullptr;
    return gwb.GetSpecObj<nobHQ>(hqPos);
}

const std::list<noBuildingSite*>& AIQueryService::GetBuildingSites() const
{
    return player_.GetBuildingRegister().GetBuildingSites();
}

const std::list<noBuildingSite*>& AIQueryService::GetPlayerBuildingSites(unsigned playerId) const
{
    return gwb.GetPlayer(playerId).GetBuildingRegister().GetBuildingSites();
}

const std::list<nobUsual*>& AIQueryService::GetBuildings(const BuildingType type) const
{
    return player_.GetBuildingRegister().GetBuildings(type);
}

const std::list<nobUsual*>& AIQueryService::GetPlayerBuildings(const BuildingType type, unsigned playerId) const
{
    return gwb.GetPlayer(playerId).GetBuildingRegister().GetBuildings(type);
}

const std::list<nobMilitary*>& AIQueryService::GetMilitaryBuildings() const
{
    return player_.GetBuildingRegister().GetMilitaryBuildings();
}

const std::list<nobHarborBuilding*>& AIQueryService::GetHarbors() const
{
    return player_.GetBuildingRegister().GetHarbors();
}

const std::list<nobBaseWarehouse*>& AIQueryService::GetStorehouses() const
{
    return player_.GetBuildingRegister().GetStorehouses();
}

bool AIQueryService::isBuildingNearby(BuildingType bldType, const MapPoint pt, unsigned maxDistance) const
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

bool AIQueryService::isHarborPosClose(const MapPoint pt, unsigned maxDistance, bool onlyEmpty) const
{
    for(unsigned i = 1; i <= gwb.GetNumHarborPoints(); i++)
    {
        const MapPoint harborPoint = gwb.GetHarborPoint(i);
        if(gwb.CalcDistance(pt, harborPoint) <= maxDistance && helpers::contains(usableHarbors_, i))
        {
            if(!onlyEmpty || !IsBuildingOnNode(harborPoint, BuildingType::HarborBuilding))
                return true;
        }
    }
    return false;
}

const Inventory& AIQueryService::GetInventory() const
{
    return player_.GetInventory();
}

unsigned AIQueryService::GetNumShips() const
{
    return player_.GetNumShips();
}

const std::vector<noShip*>& AIQueryService::GetShips() const
{
    return player_.GetShips();
}

unsigned AIQueryService::GetShipID(const noShip* ship) const
{
    return player_.GetShipID(ship);
}

bool AIQueryService::IsExplorationDirectionPossible(const MapPoint pt, const nobHarborBuilding* originHarbor,
                                                    ShipDirection direction) const
{
    return IsExplorationDirectionPossible(pt, originHarbor->GetHarborPosID(), direction);
}

bool AIQueryService::IsExplorationDirectionPossible(const MapPoint pt, unsigned originHarborID,
                                                    ShipDirection direction) const
{
    return gwb.GetNextFreeHarborPoint(pt, originHarborID, direction, playerID_) > 0;
}

Nation AIQueryService::GetNation() const
{
    return player_.nation;
}
