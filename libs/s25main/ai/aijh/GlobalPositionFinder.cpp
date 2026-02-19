//
// Created by Codex on 19.02.26.
//

#include "GlobalPositionFinder.h"

#include "AIPlayerJH.h"
#include "AIConfig.h"
#include "AIConstruction.h"
#include "BuildingPlanner.h"
#include "RttrForeachPt.h"
#include "ai/AIInterface.h"
#include "ai/AIResource.h"
#include "gameData/BuildingConsts.h"
#include "WeightParams.h"

namespace AIJH {
namespace {
bool IsBorderBlocked(const AIPlayerJH& aijh, const AIInterface& aii, const BuildingType type, const MapPoint& pt)
{
    const auto& locationParams = aijh.GetConfig().locationParams[type];
    if(locationParams.buildOnBorder)
        return false;
    return aii.CalcResourceValue(pt, AIResource::Borderland) > 1;
}

int ComputeRatingBonus(const AIPlayerJH& aijh, AIConstruction& construction, const BuildingType buildingType,
                       const BuildingType targetType, const unsigned defaultRadius, const int defaultMultiplier,
                       const MapPoint& candidate)
{
    unsigned radius = defaultRadius;
    int multiplier = defaultMultiplier;
    const auto& ratingParams = aijh.GetConfig().locationParams[buildingType].rating[targetType];
    if(ratingParams.enabled)
    {
        radius = ratingParams.radius;
        multiplier = ratingParams.multiplier;
    }

    if(radius == 0 || multiplier == 0)
        return 0;

    const int neighbors = construction.CountUsualBuildingInRadius(candidate, radius, targetType);
    return neighbors * multiplier;
}
} // namespace

GlobalPositionFinder::GlobalPositionFinder(AIPlayerJH& aijh) : aijh(aijh) {}

bool GlobalPositionFinder::CheckProximity(const BuildingType type, const MapPoint& pt) const
{
    if(!pt.isValid())
        return false;

    AIConstruction& construction = aijh.GetConstruction();
    const auto locationParam = aijh.GetConfig().locationParams[type];
    const unsigned buildingCount = aijh.GetBldPlanner().GetNumBuildings(type);

    for(const auto otherType : helpers::enumRange<BuildingType>())
    {
        const ProximityParams proximity = locationParam.proximity[otherType];
        if(proximity.enabled)
        {
            const unsigned minRadius = static_cast<unsigned>(CALC::calcCount(buildingCount, proximity.minimal));
            if(otherType == BuildingType::Storehouse)
                return !construction.OtherStoreInRadius(pt, minRadius);
            return !construction.OtherUsualBuildingInRadius(pt, minRadius, otherType);
        }
    }
    return true;
}

bool GlobalPositionFinder::ValidFishInRange(const MapPoint pt) const
{
    constexpr unsigned maxRadius = 5;
    const GameWorldBase& world = aijh.GetWorld();
    return world.CheckPointsInRadius(
      pt, maxRadius,
      [&world, pt](const MapPoint curPt, unsigned) {
          if(world.GetNode(curPt).resources.has(ResourceType::Fish))
          {
              for(const MapPoint nb : world.GetNeighbours(curPt))
              {
                  if(world.FindHumanPath(pt, nb, 10))
                      return true;
              }
          }
          return false;
      },
      false);
}

bool GlobalPositionFinder::ValidStoneinRange(const MapPoint pt) const
{
    constexpr unsigned maxRadius = 8;
    const GameWorldBase& world = aijh.GetWorld();
    for(MapCoord tx = world.GetXA(pt, Direction::West), r = 1; r <= maxRadius;
        tx = world.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = world.GetNeighbour(t2, convertToDirection(i)), ++r2)
            {
                if(world.GetNO(t2)->GetType() == NodalObjectType::Granite)
                {
                    if(world.FindHumanPath(pt, t2, 20))
                        return true;
                }
            }
        }
    }
    return false;
}

std::optional<int> GlobalPositionFinder::GetPointRating(const BuildingType type, const MapPoint& pt) const
{
    if(!CheckProximity(type, pt))
        return std::nullopt;

    AIInterface& aii = aijh.GetInterface();
    AIConstruction& construction = aijh.GetConstruction();
    switch(type)
    {
        case BuildingType::Woodcutter:
        {
            const int woodRating = aii.CalcResourceValue(pt, AIResource::Wood);
            if(woodRating < 50 || construction.OtherUsualBuildingInRadius(pt, 3, BuildingType::Woodcutter))
                return std::nullopt;
            const int ratingBonus = ComputeRatingBonus(aijh, construction, type, BuildingType::Forester, 7, 300, pt);
            return woodRating + ratingBonus;
        }
        case BuildingType::Forester:
        {
            const int plantspaceRating = aii.CalcResourceValue(pt, AIResource::Plantspace);
            if(plantspaceRating < 50)
                return std::nullopt;
            const int ratingBonus = ComputeRatingBonus(aijh, construction, type, BuildingType::Woodcutter, 6, 50, pt);
            return plantspaceRating + ratingBonus;
        }
        case BuildingType::Farm:
        {
            const int plantspaceRating = aii.CalcResourceValue(pt, AIResource::Plantspace);
            if(plantspaceRating < 85 || construction.OtherUsualBuildingInRadius(pt, 8, BuildingType::Forester))
                return std::nullopt;
            return plantspaceRating;
        }
        case BuildingType::Quarry:
        {
            const int stoneRating = aii.CalcResourceValue(pt, AIResource::Stones);
            if(stoneRating < 40 || !ValidStoneinRange(pt))
                return std::nullopt;
            return stoneRating;
        }
        case BuildingType::Fishery:
        {
            const int fishRating = aii.CalcResourceValue(pt, AIResource::Fish);
            if(fishRating < 20 || aii.isBuildingNearby(BuildingType::Fishery, pt, 5)
               || !ValidFishInRange(pt))
                return std::nullopt;
            return fishRating;
        }
        default: return 1;
    }
}

MapPoint GlobalPositionFinder::FindBestPosition(const BuildingType bt)
{
    int bestValue = 0;
    MapPoint bestPt = MapPoint::Invalid();
    AIInterface& aii = aijh.GetInterface();
    const MapExtent mapSize = aijh.GetWorld().GetSize();
    const BuildingQuality requiredSize = BUILDING_SIZE[bt];

    RTTR_FOREACH_PT(MapPoint, mapSize)
    {
        const Node& node = aijh.GetAINode(pt);
        if(!node.reachable || !node.owned || node.farmed)
            continue;
        if(!canUseBq(node.bq, requiredSize))
            continue;
        if(aii.isHarborPosClose(pt, 2, true) && requiredSize != BuildingQuality::Harbor)
            continue;
        if(IsBorderBlocked(aijh, aii, bt, pt))
            continue;

        const std::optional<int> pointRating = GetPointRating(bt, pt);
        if(!pointRating)
            continue;
        if(*pointRating > bestValue)
        {
            bestValue = *pointRating;
            bestPt = pt;
        }
    }

    return bestPt;
}
} // namespace AIJH
