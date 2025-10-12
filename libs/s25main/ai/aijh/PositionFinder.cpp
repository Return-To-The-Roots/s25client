
#include "PositionFinder.h"

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
    arounds.push_back(aii.GetHeadquarter()->GetPos());

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
    auto locationParam = AI_CONFIG.locationParams[type];
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
} // namespace AIJH
