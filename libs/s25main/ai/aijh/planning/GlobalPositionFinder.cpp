//
// Created by Codex on 19.02.26.
//

#include "GlobalPositionFinder.h"

#include "ai/aijh/config/AIConfig.h"
#include "ai/aijh/runtime/AIPlanningContext.h"
#include "AIConstruction.h"
#include "BuildingPlanner.h"
#include "RttrForeachPt.h"
#include "ai/AIInterface.h"
#include "ai/AIQueryService.h"
#include "ai/AIResource.h"
#include "gameData/BuildingConsts.h"
#include "ai/aijh/config/WeightParams.h"

namespace AIJH {
namespace {
bool IsBorderBlocked(const AIWorldView& aijh, const AIQueryService& queries, const BuildingType type,
                     const MapPoint& pt)
{
    const auto& locationParams = aijh.GetConfig().locationParams[type];
    if(locationParams.buildOnBorder)
        return false;
    return queries.CalcResourceValue(pt, AIResource::Borderland) > 1;
}

int ComputeRatingBonus(const AIWorldView& aijh, AIConstruction& construction, const BuildingType buildingType,
                       const MapPoint& candidate)
{
    int totalBonus = 0;
    const auto& locationParams = aijh.GetConfig().locationParams[buildingType];
    for(const auto targetType : helpers::enumRange<BuildingType>())
    {
        const auto& ratingParams = locationParams.rating[targetType];
        if(!ratingParams.enabled)
            continue;

        const unsigned radius = ratingParams.radius > 0 ? ratingParams.radius : locationParams.resourceRating.defaultRadius;
        const int multiplier =
          ratingParams.multiplier != 0 ? ratingParams.multiplier : locationParams.resourceRating.defaultMultiplier;

        if(radius == 0 || multiplier == 0)
            continue;

        const int neighbors = construction.CountUsualBuildingInRadius(candidate, radius, targetType);
        totalBonus += neighbors * multiplier;
    }
    return totalBonus;
}

bool MeetsMinimalResourceRequirement(const AIWorldView& aijh, const BuildingType type, const AIResource res,
                                     int rating)
{
    const unsigned minRequirement = aijh.GetConfig().locationParams[type].minResources[res];
    return rating >= static_cast<int>(minRequirement);
}

bool MeetsPointResourceRequirements(const AIWorldView& aijh, const AIQueryService& queries, const BuildingType type,
                                    const MapPoint& pt)
{
    const auto& minRequirements = aijh.GetConfig().locationParams[type].minResources;
    for(const auto resource : helpers::enumRange<AIResource>())
    {
        if(minRequirements[resource] == 0)
            continue;

        if(!MeetsMinimalResourceRequirement(aijh, type, resource, queries.CalcResourceValue(pt, resource)))
            return false;
    }
    return true;
}

/// Map a BuildingQuality to its building-size level (Castle=4, House=3, Hut=2, Flag/Mine=1, Nothing=0).
int BQLevel(BuildingQuality bq)
{
    switch(bq)
    {
        case BuildingQuality::Harbor:
        case BuildingQuality::Castle: return 4;
        case BuildingQuality::House: return 3;
        case BuildingQuality::Hut: return 2;
        case BuildingQuality::Mine:
        case BuildingQuality::Flag: return 1;
        case BuildingQuality::Nothing:
        default: return 0;
    }
}

constexpr int maxBQLevel = 4; // Castle/Harbor

int ComputeResourcePenalty(const AIWorldView& aijh, const AIQueryService& queries, const BuildingType type,
                           const MapPoint& pt)
{
    int totalPenalty = 0;
    const auto& penaltyParams = aijh.GetConfig().locationParams[type].resourcePenalty;
    for(const auto resource : helpers::enumRange<AIResource>())
    {
        const BuildParams params = penaltyParams[resource];
        if(!params.enabled)
            continue;

        const unsigned resourceValue = queries.CalcResourceValue(pt, resource);
        if(resourceValue < params.min)
            continue;

        const double value = CALC::calcCount(resourceValue, params);
        totalPenalty += static_cast<int>(std::min<double>(value, params.max));
    }

    // Penalize for BQ degradation at adjacent points
    const double bqPenaltyPerLevel = aijh.GetConfig().bqPenaltyPerLevel;
    if(bqPenaltyPerLevel > 0.0)
    {
        double bqPenalty = 0.0;
        const GameWorldBase& world = aijh.GetWorld();
        for(const MapPoint nb : world.GetNeighbours(pt))
        {
            const Node& nbNode = aijh.GetAINode(nb);
            const int decrease = maxBQLevel - BQLevel(nbNode.bq);
            if(decrease > 0)
                bqPenalty += decrease * bqPenaltyPerLevel;
        }
        totalPenalty += static_cast<int>(bqPenalty);
    }

    return totalPenalty;
}

int ComputeResourceRating(const AIWorldView& aijh, const AIQueryService& queries, AIConstruction& construction,
                          const BuildingType type, const MapPoint& pt)
{
    const auto& resourceRating = aijh.GetConfig().locationParams[type].resourceRating;
    int rating = 1;
    if(resourceRating.enabled)
        rating = queries.CalcResourceValue(pt, resourceRating.resource);

    rating += ComputeRatingBonus(aijh, construction, type, pt);
    return rating;
}
} // namespace

GlobalPositionFinder::GlobalPositionFinder(AIPlanningContext& aijh) : aijh(aijh) {}

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

    const AIQueryService& queries = aijh.GetInterface().Queries();
    AIConstruction& construction = aijh.GetConstruction();

    switch(type)
    {
        case BuildingType::Quarry:
            if(!ValidStoneinRange(pt))
                return std::nullopt;
            break;
        default: break;
    }

    const int baseRating = ComputeResourceRating(aijh, queries, construction, type, pt);
    const int resourcePenalty = ComputeResourcePenalty(aijh, queries, type, pt);
    return baseRating - resourcePenalty;
}

MapPoint GlobalPositionFinder::FindBestPosition(const BuildingType bt)
{
    int bestValue = 0;
    MapPoint bestPt = MapPoint::Invalid();
    const AIQueryService& queries = aijh.GetInterface().Queries();
    const MapExtent mapSize = aijh.GetWorld().GetSize();
    const BuildingQuality requiredSize = BUILDING_SIZE[bt];

    RTTR_FOREACH_PT(MapPoint, mapSize)
    {
        const Node& node = aijh.GetAINode(pt);
        if(!node.reachable || !node.owned || node.farmed)
            continue;
        if(!canUseBq(node.bq, requiredSize))
            continue;
        if(queries.isHarborPosClose(pt, 2, true) && requiredSize != BuildingQuality::Harbor)
            continue;
        if(IsBorderBlocked(aijh, queries, bt, pt))
            continue;
        if(!MeetsPointResourceRequirements(aijh, queries, bt, pt))
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
