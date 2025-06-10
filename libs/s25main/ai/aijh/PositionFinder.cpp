
#include "PositionFinder.h"

#include "AIConstruction.h"
#include "BuildingPlanner.h"
#include "ai/aijh/AIMap.h"
#include "buildings/nobBaseWarehouse.h"
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
    for(const nobBaseWarehouse* wh : aii.GetStorehouses())
    {
        arounds.push_back(wh->GetPos());
    }
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
    switch(type)
    {
        case BuildingType::Sawmill:
        {
            MapPoint point = aijh.SimpleFindPosition(around, BUILDING_SIZE[type], 3);
            unsigned sawmills = aijh.GetBldPlanner().GetNumBuildings(BuildingType::Sawmill);
            if(construction.OtherUsualBuildingInRadius(point, 2 * sawmills, BuildingType::Sawmill))
            {
                break;
            }
            return {point, 1};;
        }
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
                if(construction.OtherUsualBuildingInRadius(point.pt, 11, BuildingType::Forester))
                {
                    continue;
                }
                if(construction.OtherUsualBuildingInRadius(point.pt, 8, BuildingType::Farm))
                {
                    continue;
                }
                if(construction.OtherStoreInRadius(point.pt, 11))
                {
                    continue;
                }
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
        default: break;
    }
    return {MapPoint::Invalid(), 0};;
}

RatedPointSet PositionFinder::FindBestPositions(const MapPoint& pt, const AIResource res, BuildingQuality size,
                                                unsigned radius, int minimum)
{
    AIJH::AIResourceMap resMap = aijh.GetResMap(res);
    resMap.updateAround(pt, radius);
    return resMap.findBestPositions(pt, size, radius, minimum, 5);
}
} // namespace AIJH