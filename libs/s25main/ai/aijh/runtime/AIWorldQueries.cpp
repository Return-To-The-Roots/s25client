// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ai/aijh/runtime/AIWorldQueries.h"

#include "ai/aijh/runtime/AIMapState.h"
#include "ai/aijh/runtime/AIPlayerJH.h"
#include "BuildingRegister.h"
#include "ai/aijh/planning/AIConstruction.h"
#include "ai/aijh/planning/BuildingPlanner.h"
#include "buildings/noBaseBuilding.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noTree.h"

#include <algorithm>
#include <limits>
#include <list>
#include <vector>

namespace AIJH {

MapPoint AIWorldQueries::SimpleFindPosition(const MapPoint& pt, const BuildingType type, const unsigned radius) const
{
    const BuildingQuality size = BUILDING_SIZE[type];
    const std::vector<MapPoint> pts = owner_.gwb.GetPointsInRadius(pt, radius);
    MapPoint bestFallback = MapPoint::Invalid();
    int bestQualityDelta = std::numeric_limits<int>::max();

    for(const MapPoint& curPt : pts)
    {
        if(!owner_.GetAINode(curPt).reachable || owner_.GetAINode(curPt).farmed || !owner_.aii.IsOwnTerritory(curPt))
            continue;
        if(owner_.aii.isHarborPosClose(curPt, 2, true))
        {
            if(size != BuildingQuality::Harbor)
                continue;
        }
        RTTR_Assert(owner_.aii.GetBuildingQuality(curPt) == owner_.GetAINode(curPt).bq);
        const BuildingQuality nodeBq = owner_.aii.GetBuildingQuality(curPt);
        if(!BuildingProperties::IsMilitary(type) && owner_.aii.Queries().IsReservedMilitaryBorderSlot(curPt, nodeBq))
            continue;
        if(nodeBq == size)
            return curPt;
        if(canUseBq(nodeBq, size))
        {
            const int delta = static_cast<int>(nodeBq) - static_cast<int>(size);
            if(delta < bestQualityDelta)
            {
                bestQualityDelta = delta;
                bestFallback = curPt;
            }
        }
    }

    return bestFallback;
}

MapPoint AIWorldQueries::FindPositionForBuildingAround(BuildingType type, const MapPoint& around)
{
    constexpr unsigned searchRadius = 11;
    MapPoint foundPos = MapPoint::Invalid();
    switch(type)
    {
        case BuildingType::Bakery:
        case BuildingType::Brewery:
        case BuildingType::Armory:
        case BuildingType::Metalworks:
        case BuildingType::Ironsmelter:
        case BuildingType::Slaughterhouse:
        case BuildingType::PigFarm:
        case BuildingType::Mill:
        case BuildingType::Well:
        {
            foundPos = SimpleFindPosition(around, type, searchRadius);
            if(owner_.construction->OtherUsualBuildingInRadius(foundPos, 4, BuildingType::Forester))
                foundPos = MapPoint::Invalid();
            break;
        }
        case BuildingType::Hunter:
        {
            if(HuntablesinRange(around, (2 << owner_.GetBldPlanner().GetNumBuildings(BuildingType::Hunter))))
                foundPos = SimpleFindPosition(around, type, searchRadius);
            break;
        }
        case BuildingType::Quarry:
        {
            const unsigned numQuarries = owner_.GetBldPlanner().GetNumBuildings(BuildingType::Quarry);
            foundPos = owner_.FindBestPosition(around, AIResource::Stones, BUILDING_SIZE[type], searchRadius,
                                               std::min(40u, 1 + numQuarries * 10));
            if(foundPos.isValid() && !ValidStoneinRange(foundPos))
            {
                owner_.GetResMap(AIResource::Stones).avoidPosition(foundPos);
                foundPos = MapPoint::Invalid();
            }
            break;
        }
        case BuildingType::Barracks:
        case BuildingType::Guardhouse:
        case BuildingType::Watchtower:
        case BuildingType::Fortress:
            foundPos = owner_.FindBestPosition(around, AIResource::Borderland, BUILDING_SIZE[type], searchRadius);
            break;
        case BuildingType::GoldMine:
            foundPos = owner_.FindBestPosition(around, AIResource::Gold, BuildingQuality::Mine, searchRadius);
            break;
        case BuildingType::CoalMine:
            foundPos = owner_.FindBestPosition(around, AIResource::Coal, BuildingQuality::Mine, searchRadius);
            break;
        case BuildingType::IronMine:
            foundPos = owner_.FindBestPosition(around, AIResource::Ironore, BuildingQuality::Mine, searchRadius);
            break;
        case BuildingType::GraniteMine:
            foundPos = owner_.FindBestPosition(around, AIResource::Granite, BuildingQuality::Mine, searchRadius);
            break;
        case BuildingType::Fishery:
            foundPos = owner_.FindBestPosition(around, AIResource::Fish, BUILDING_SIZE[type], searchRadius);
            if(foundPos.isValid() && !ValidFishInRange(foundPos))
            {
                owner_.GetResMap(AIResource::Fish).avoidPosition(foundPos);
                foundPos = MapPoint::Invalid();
            } else if(owner_.construction->OtherUsualBuildingInRadius(foundPos, 3, BuildingType::Fishery))
            {
                foundPos = MapPoint::Invalid();
            }
            break;
        case BuildingType::Storehouse:
            if(!owner_.construction->OtherStoreInRadius(around, 15))
                foundPos = SimpleFindPosition(around, type, searchRadius);
            break;
        case BuildingType::HarborBuilding:
            foundPos = SimpleFindPosition(around, type, searchRadius);
            if(foundPos.isValid() && !HarborPosRelevant(owner_.GetWorld().GetHarborPointID(foundPos)))
                foundPos = MapPoint::Invalid();
            break;
        case BuildingType::Shipyard:
            foundPos = SimpleFindPosition(around, type, searchRadius);
            if(foundPos.isValid() && IsInvalidShipyardPosition(foundPos))
                foundPos = MapPoint::Invalid();
            break;
        case BuildingType::Farm:
            if(owner_.construction->OtherUsualBuildingInRadius(around, 6, BuildingType::Forester))
                break;
            foundPos = owner_.FindBestPosition(around, AIResource::Plantspace, BUILDING_SIZE[type], searchRadius, 85);
            break;
        case BuildingType::Catapult:
            foundPos = SimpleFindPosition(around, type, searchRadius);
            if(foundPos.isValid() && owner_.aii.isBuildingNearby(BuildingType::Catapult, foundPos, 7))
                foundPos = MapPoint::Invalid();
            break;
        default: foundPos = SimpleFindPosition(around, type, searchRadius); break;
    }
    return foundPos;
}

unsigned AIWorldQueries::GetAvailableResources(AISurfaceResource resource) const
{
    unsigned sum = 0;
    const AIMap& aiMap = owner_.mapState_->GetMap();
    for(unsigned i = 0; i < aiMap.Size(); ++i)
    {
        const auto node = aiMap[i];
        if(node.owned)
        {
            const unsigned short x = i % aiMap.GetSize().x;
            const unsigned short y = i / aiMap.GetSize().x;
            if(owner_.aii.GetSurfaceResource(MapPoint(x, y)) == resource)
                sum++;
        }
    }
    return sum;
}

unsigned AIWorldQueries::GetDensity(MapPoint pt, AIResource res, int radius)
{
    const AIMap& aiMap = owner_.mapState_->GetMap();
    RTTR_Assert(pt.x < aiMap.GetWidth() && pt.y < aiMap.GetHeight());

    const std::vector<MapPoint> pts = owner_.gwb.GetPointsInRadius(pt, radius);
    const unsigned numAllPTs = pts.size();
    RTTR_Assert(numAllPTs > 0);

    const auto hasResource = [this, res](const MapPoint& curPt) {
        return owner_.CalcResource(curPt) == res;
    };
    const unsigned numGoodPts = helpers::count_if(pts, hasResource);
    return (numGoodPts * 100) / numAllPTs;
}

bool AIWorldQueries::HuntablesinRange(const MapPoint pt, unsigned min)
{
    if(owner_.aii.isBuildingNearby(BuildingType::Hunter, pt, 14))
        return false;
    unsigned maxrange = 25;
    unsigned short fx, fy, lx, ly;
    const unsigned short SQUARE_SIZE = 19;
    unsigned huntablecount = 0;
    if(pt.x > SQUARE_SIZE)
        fx = pt.x - SQUARE_SIZE;
    else
        fx = 0;
    if(pt.y > SQUARE_SIZE)
        fy = pt.y - SQUARE_SIZE;
    else
        fy = 0;
    if(pt.x + SQUARE_SIZE < owner_.gwb.GetWidth())
        lx = pt.x + SQUARE_SIZE;
    else
        lx = owner_.gwb.GetWidth() - 1;
    if(pt.y + SQUARE_SIZE < owner_.gwb.GetHeight())
        ly = pt.y + SQUARE_SIZE;
    else
        ly = owner_.gwb.GetHeight() - 1;
    for(MapPoint p2(0, fy); p2.y <= ly; ++p2.y)
    {
        for(p2.x = fx; p2.x <= lx; ++p2.x)
        {
            for(const noBase& fig : owner_.gwb.GetFigures(p2))
            {
                if(fig.GetType() == NodalObjectType::Animal)
                {
                    if(!static_cast<const noAnimal&>(fig).CanHunted())
                        continue;
                    if(owner_.gwb.FindHumanPath(pt, static_cast<const noAnimal&>(fig).GetPos(), maxrange))
                    {
                        if(++huntablecount >= min)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

bool AIWorldQueries::ValidTreeinRange(const MapPoint pt)
{
    const unsigned max_radius = 6;
    for(MapCoord tx = owner_.gwb.GetXA(pt, Direction::West), r = 1; r <= max_radius;
        tx = owner_.gwb.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = owner_.gwb.GetNeighbour(t2, convertToDirection(i)), ++r2)
            {
                if(owner_.gwb.GetNO(t2)->GetType() == NodalObjectType::Tree)
                {
                    if(!owner_.gwb.GetNode(t2).reserved && owner_.gwb.GetSpecObj<noTree>(t2)->ProducesWood())
                    {
                        if(owner_.gwb.FindHumanPath(pt, t2, 20))
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

bool AIWorldQueries::ValidStoneinRange(const MapPoint pt)
{
    const unsigned max_radius = 8;
    for(MapCoord tx = owner_.gwb.GetXA(pt, Direction::West), r = 1; r <= max_radius;
        tx = owner_.gwb.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = owner_.gwb.GetNeighbour(t2, convertToDirection(i)), ++r2)
            {
                if(owner_.gwb.GetNO(t2)->GetType() == NodalObjectType::Granite)
                {
                    if(owner_.gwb.FindHumanPath(pt, t2, 20))
                        return true;
                }
            }
        }
    }
    return false;
}

unsigned AIWorldQueries::BQsurroundcheck(const MapPoint pt, unsigned range, bool includeexisting, unsigned limit)
{
    const unsigned maxvalue = 6 * (2 << (range - 1)) - 5;
    unsigned count = 0;
    RTTR_Assert(owner_.aii.GetBuildingQuality(pt) == owner_.GetAINode(pt).bq);
    if((owner_.aii.GetBuildingQuality(pt) >= BuildingQuality::Hut
        && owner_.aii.GetBuildingQuality(pt) <= BuildingQuality::Castle)
       || owner_.aii.GetBuildingQuality(pt) == BuildingQuality::Harbor)
    {
        count++;
    }
    NodalObjectType nob = owner_.gwb.GetNO(pt)->GetType();
    if(includeexisting)
    {
        if(nob == NodalObjectType::Building || nob == NodalObjectType::Buildingsite || nob == NodalObjectType::Extension
           || nob == NodalObjectType::Fire || nob == NodalObjectType::CharburnerPile)
            count++;
    }
    for(MapCoord tx = owner_.gwb.GetXA(pt, Direction::West), r = 1; r <= range;
        tx = owner_.gwb.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = owner_.gwb.GetNeighbour(t2, convertToDirection(i)), ++r2)
            {
                if(limit && ((count * 100) / maxvalue) > limit)
                    return ((count * 100) / maxvalue);
                if((owner_.aii.GetBuildingQualityAnyOwner(t2) >= BuildingQuality::Hut
                    && owner_.aii.GetBuildingQualityAnyOwner(t2) <= BuildingQuality::Castle)
                   || owner_.aii.GetBuildingQualityAnyOwner(t2) == BuildingQuality::Harbor)
                {
                    count++;
                    continue;
                }
                if(includeexisting)
                {
                    nob = owner_.gwb.GetNO(t2)->GetType();
                    if(nob == NodalObjectType::Building || nob == NodalObjectType::Buildingsite
                       || nob == NodalObjectType::Extension || nob == NodalObjectType::Fire
                       || nob == NodalObjectType::CharburnerPile)
                        count++;
                }
            }
        }
    }
    return ((count * 100) / maxvalue);
}

bool AIWorldQueries::HarborPosRelevant(unsigned harborid, bool onlyempty) const
{
    if(harborid < 1 || harborid > owner_.gwb.GetNumHarborPoints())
    {
        RTTR_Assert(false);
        return false;
    }
    if(!onlyempty)
        return helpers::contains(owner_.aii.getUsableHarbors(), harborid);

    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        const unsigned short seaId = owner_.gwb.GetSeaId(harborid, dir);
        if(!seaId)
            continue;

        for(unsigned curHarborId = 1; curHarborId <= owner_.gwb.GetNumHarborPoints(); curHarborId++)
        {
            if(curHarborId != harborid && owner_.gwb.IsHarborAtSea(curHarborId, seaId))
            {
                if(owner_.gwb.IsHarborPointFree(curHarborId, owner_.playerId))
                    return true;
            }
        }
    }
    return false;
}

bool AIWorldQueries::NoEnemyHarbor()
{
    for(unsigned i = 1; i <= owner_.gwb.GetNumHarborPoints(); i++)
    {
        if(owner_.aii.IsBuildingOnNode(owner_.gwb.GetHarborPoint(i), BuildingType::HarborBuilding)
           && !owner_.aii.IsOwnTerritory(owner_.gwb.GetHarborPoint(i)))
        {
            return false;
        }
    }
    return true;
}

bool AIWorldQueries::IsInvalidShipyardPosition(const MapPoint pt)
{
    return owner_.aii.isBuildingNearby(BuildingType::Shipyard, pt, 19) || !owner_.aii.isHarborPosClose(pt, 7);
}

bool AIWorldQueries::ValidFishInRange(const MapPoint pt)
{
    const unsigned max_radius = 5;
    return owner_.gwb.CheckPointsInRadius(
      pt, max_radius,
      [this, pt](const MapPoint curPt, unsigned) {
          if(owner_.gwb.GetNode(curPt).resources.has(ResourceType::Fish))
          {
              for(const MapPoint nb : owner_.gwb.GetNeighbours(curPt))
              {
                  if(owner_.gwb.FindHumanPath(pt, nb, 10))
                      return true;
              }
          }
          return false;
      },
      false);
}

unsigned AIWorldQueries::GetNumAIRelevantSeaIds() const
{
    std::vector<unsigned short> validseaids;
    std::list<unsigned short> onetimeuseseaids;
    for(unsigned i = 1; i <= owner_.gwb.GetNumHarborPoints(); i++)
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const unsigned short seaId = owner_.gwb.GetSeaId(i, dir);
            if(!seaId)
                continue;
            if(!helpers::contains(validseaids, seaId))
            {
                if(!helpers::contains(onetimeuseseaids, seaId))
                    onetimeuseseaids.push_back(seaId);
                else
                {
                    onetimeuseseaids.remove(seaId);
                    validseaids.push_back(seaId);
                }
            }
        }
    }
    return validseaids.size();
}

unsigned AIWorldQueries::GetProductivity(BuildingType type) const
{
    return owner_.player.GetBuildingRegister().CalcAverageProductivity(type);
}

} // namespace AIJH
