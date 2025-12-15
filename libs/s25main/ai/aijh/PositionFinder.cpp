
#include "PositionFinder.h"

#include "AIPlayerJH.h"
#include "AIConfig.h"
#include "AIConstruction.h"
#include "BuildingPlanner.h"
#include "WeightParams.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHQ.h"
#include "buildings/nobMilitary.h"
#include "gameData/BuildingConsts.h"

namespace AIJH {
PositionFinder::PositionFinder(AIPlayerJH& aijh) : aijh(aijh) {}

MapPoint PositionFinder::FindBestPosition(BuildingType bt)
{
    AIInterface& aii = aijh.getAIInterface();
    int bestValue = 0;
    MapPoint bestPt = MapPoint::Invalid();
    std::vector<MapPoint> arounds;
    for(const nobMilitary* mil : aii.GetMilitaryBuildings())
    {
        arounds.push_back(mil->GetPos());
    }
    if(const auto* headquarters = aii.GetHeadquarter())
        arounds.push_back(headquarters->GetPos());

    for(const MapPoint around : arounds)
    {
        auto point = FindPositionAround(bt, around, 11);
        if(point.pt.isValid())
        {
            if(point.rating > bestValue)
            {
                bestValue = point.rating;
                bestPt = point.pt;
            }
        }
    }
    return bestPt;
}

RatedPoint PositionFinder::FindPositionAround(BuildingType type, const MapPoint& around, int searchRadius)
{
    AIConstruction& construction = aijh.GetConstruction();

    // First check the general minRadius condition for all building types
    // auto locationParam = AI_CONFIG.locationParams[type];
    // unsigned buildingCount = aijh.GetBldPlanner().GetNumBuildings(type);
    // auto proximity = locationParam.proximity[type];
    /*if(proximity.enabled)
    {
        MapPoint point = aijh.SimpleFindPosition(around, BUILDING_SIZE[type], 3);
        unsigned minRadius = (unsigned)CALC::calcCount(buildingCount, proximity.minimal);
        if(construction.OtherUsualBuildingInRadius(point, minRadius, type))
        {
            return {MapPoint::Invalid(), 0};
        }
    }*/

    // Then apply specific logic for certain building types
    switch(type)
    {
        case BuildingType::Woodcutter:
        {
            auto results = FindBestPositions(around, AIResource::Wood, BUILDING_SIZE[type], searchRadius, 50);

            for(auto& point : results)
            {
                if(point.pt.isValid()
                   && !construction.OtherUsualBuildingInRadius(point.pt, 3, BuildingType::Woodcutter))
                {
                    int foresters = construction.CountUsualBuildingInRadius(point.pt, 7, BuildingType::Forester);
                    return {point.pt, point.rating + 300 * foresters};
                }
            }
            break;
        }
        case BuildingType::Forester:
        {
            auto results = FindBestPositions(around, AIResource::Plantspace, BUILDING_SIZE[type], searchRadius, 50);

            for(auto& point : results)
            {
                if(!CheckProximity(type, point.pt)) continue;
                int woodcutters = construction.CountUsualBuildingInRadius(point.pt, 6, BuildingType::Woodcutter);
                return {point.pt, point.rating + woodcutters * 50};
            }
            break;
        }
        case BuildingType::Farm:
        {
            auto results = FindBestPositions(around, AIResource::Plantspace, BUILDING_SIZE[type], searchRadius, 85);
            for(const auto& point : results)
            {
                if(!construction.OtherUsualBuildingInRadius(point.pt, 8, BuildingType::Forester))
                {
                    return point;
                }
            }
            break;
        }
        case BuildingType::Quarry:
        {
            auto results = FindBestPositions(around, AIResource::Stones, BUILDING_SIZE[type], searchRadius, 40);
            for(const auto& point : results)
            {
                if(!point.pt.isValid() || !ValidStoneinRange(point.pt))
                {
                    break;
                }
                return point;
            }
            break;
        }
        case BuildingType::Fishery:
        {
            auto results = FindBestPositions(around, AIResource::Fish, BUILDING_SIZE[type], searchRadius, 20);
            for(const auto& point : results)
            {
                if(construction.OtherUsualBuildingInRadius(point.pt, 3, BuildingType::Fishery))
                {
                    break;
                }
                if(!point.pt.isValid() || !ValidResourceInRange(point.pt))
                {
                    break;
                }
                return point;
            }
            break;
        }
        default:
        {
            // For all other building types, just return a simple position if the minRadius check passed
            MapPoint point = aijh.SimpleFindPosition(around, BUILDING_SIZE[type], 6);
            if(CheckProximity(type, point))
                return {point, 1};
            break;
        }
    }
    return {MapPoint::Invalid(), 0};
}

bool PositionFinder::CheckProximity(BuildingType type, const MapPoint& pt){
    if(!pt.isValid())
    {
        return false;
    }
    AIConstruction& construction = aijh.GetConstruction();
    const auto locationParam = aijh.GetConfig().locationParams[type];
    unsigned buildingCount = aijh.GetBldPlanner().GetNumBuildings(type);
    for(const auto otherType : helpers::enumRange<BuildingType>())
    {
        ProximityParams proximity = locationParam.proximity[otherType];
        if(proximity.enabled)
        {
            unsigned minRadius = (unsigned)CALC::calcCount(buildingCount, proximity.minimal);
            if(otherType == BuildingType::Storehouse)
            {
                return !construction.OtherStoreInRadius(pt, minRadius);
            }
            return !construction.OtherUsualBuildingInRadius(pt, minRadius, otherType);
        }
    }
    return true;
}

RatedPointSet PositionFinder::FindBestPositions(const MapPoint& pt, const AIResource res, BuildingQuality size,
                                                unsigned radius, int minimum)
{
    AIJH::AIResourceMap resMap = aijh.GetResMap(res);
    resMap.updateAround(pt, radius);
    return resMap.findBestPositions(pt, size, radius, minimum, 5);
}

bool PositionFinder::ValidFishInRange(const MapPoint pt)
{
    unsigned max_radius = 5;
    return aijh.gwb.CheckPointsInRadius(
      pt, max_radius,
      [this, pt](const MapPoint curPt, unsigned) {
          if(aijh.gwb.GetNode(curPt).resources.has(ResourceType::Fish)) // fish on current spot?
          {
              // try to find a path to a neighboring node on the coast
              for(const MapPoint nb : aijh.gwb.GetNeighbours(curPt))
              {
                  if(aijh.gwb.FindHumanPath(pt, nb, 10))
                      return true;
              }
          }
          return false;
      },
      false);
}

bool PositionFinder::ValidStoneinRange(const MapPoint pt)
{
    unsigned max_radius = 8;
    for(MapCoord tx = aijh.gwb.GetXA(pt, Direction::West), r = 1; r <= max_radius;
        tx = aijh.gwb.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = aijh.gwb.GetNeighbour(t2, convertToDirection(i)), ++r2)
            {
                // point has tree & path is available?
                if(aijh.gwb.GetNO(t2)->GetType() == NodalObjectType::Granite)
                {
                    if(aijh.gwb.FindHumanPath(pt, t2, 20))
                        return true;
                }
            }
        }
    }
    return false;
}

} // namespace AIJH
