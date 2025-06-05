
#include "PositionFinder.h"

#include "AIConstruction.h"
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
    RatedPoint result = {MapPoint::Invalid(), 0};
    AIConstruction& construction = aijh.GetConstruction();
    switch(type)
    {
        case BuildingType::Woodcutter:
        {
            auto results = FindBestPositions(around, AIResource::Wood, BUILDING_SIZE[type], searchRadius, 50);

            for(auto& point : results)
            {
                if(point.pt.isValid() && !construction.OtherUsualBuildingInRadius(point.pt, 3, BuildingType::Woodcutter))
                {
                    return point;
                }
            }
            break;
        }
        case BuildingType::Farm:
        {
            auto results = FindBestPositions(around, AIResource::Plantspace, BUILDING_SIZE[type], searchRadius, 85);
            for(const auto& point : results)
            {
                if(!construction.OtherUsualBuildingInRadius(point.pt, 6, BuildingType::Forester))
                {
                    return point;
                }
            }
            break;
        }
        default: break;
    }
    return result;
}

RatedPointSet PositionFinder::FindBestPositions(const MapPoint& pt, const AIResource res, BuildingQuality size,
                                                unsigned radius, int minimum)
{
    AIJH::AIResourceMap resMap = aijh.GetResMap(res);
    resMap.updateAround(pt, radius);
    return resMap.findBestPositions(pt, size, radius, minimum, 5);
}
} // namespace AIJH