// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIResourceMap.h"
#include "RttrForeachPt.h"
#include "ai/AIQueryService.h"
#include "ai/aijh/runtime/AIMap.h"
#include "gameData/TerrainDesc.h"

#include <boost/concept/detail/has_constraints.hpp>

namespace AIJH {

static constexpr bool isDiminishable(AIResource res)
{
    switch(res)
    {
        case AIResource::Gold:
        case AIResource::Ironore:
        case AIResource::Coal:
        case AIResource::Granite:
        case AIResource::Fish:
        case AIResource::Stones: return true;
        case AIResource::Wood:
        case AIResource::Plantspace:
        case AIResource::Borderland: return false;
    }
    return false;
}

AIResourceMap::AIResourceMap(const AIResource res, bool isInfinite, const AIQueryService& queries,
                             const GameWorldBase& world, const AIMap& aiMap)
    : res(res), isInfinite(isInfinite), isDiminishableResource(isDiminishable(res)), resRadius(RES_RADIUS[res]),
      queries_(queries), world_(world), aiMap(aiMap)
{}

AIResourceMap::~AIResourceMap() = default;

void AIResourceMap::init()
{
    const MapExtent mapSize = aiMap.GetSize();

    map.Resize(mapSize);
    // Calculate value for each point.
    // This is quite expensive so do just for the diminishable resources to sort out ones where there will never be
    // anything, which allows to skip calculating the value when updating the resource map
    if(isDiminishableResource)
    {
        ETerrain requiredTerrain;
        if(res == AIResource::Fish || res == AIResource::Stones)
            requiredTerrain = ETerrain::Buildable;
        else
        {
            RTTR_Assert(helpers::contains(
              std::vector<AIResource>{AIResource::Gold, AIResource::Ironore, AIResource::Coal, AIResource::Granite},
              res));
            requiredTerrain = ETerrain::Mineable;
        }
        RTTR_FOREACH_PT(MapPoint, mapSize)
        {
            // Calculate only if we can build there
            const bool isValid =
              world_.IsOfTerrain(pt, [requiredTerrain](const TerrainDesc& desc) { return desc.Is(requiredTerrain); });
            map[pt] = isValid ? queries_.CalcResourceValue(pt, res) : 0;
        }
    }
}

void AIResourceMap::updateAround(const MapPoint& pt, int radius)
{
    if(isDiminishableResource)
        updateAroundDiminishable(pt, radius);
    else
        updateAroundReplinishable(pt, radius);
}

unsigned AIResourceMap::calcResources() const
{
    unsigned sum = 0;
    for(unsigned i = 0; i < aiMap.Size(); i++)
    {
        auto node = aiMap[i];
        if(node.reachable && node.owned && !node.farmed)
        {
            sum += map[i];
        }
    }
    return sum;
}
unsigned AIResourceMap::calcResources(const MapPoint& pt, unsigned radius) const
{
    unsigned sum = 0;
    std::vector<MapPoint> pts = world_.GetPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& curPt : pts)
    {
        if(curPt.x >= map.GetSize().x || curPt.y >= map.GetSize().y)
            continue;

        const unsigned idx = map.GetIdx(curPt);
        if(!aiMap[idx].reachable || !aiMap[idx].owned || aiMap[idx].farmed)
            continue;
        sum += map[idx];
    }

    return sum;
}
int AIResourceMap::getResourcesAt(const MapPoint& pt) const
{
    const unsigned idx = map.GetIdx(pt);
    return map[idx];
}

RatedPointSet AIResourceMap::findBestPositions(const MapPoint& pt, BuildingQuality size, unsigned radius,
                                               int minimum, int maxCount) const
{
    int min_value = (minimum == std::numeric_limits<int>::min()) ? minimum : minimum - 1;
    RatedPointSet topSet(maxCount);
    std::vector<MapPoint> pts = world_.GetPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& curPt : pts)
    {
        const unsigned idx = map.GetIdx(curPt);

        if(map[idx] > min_value)
        {
            if(!aiMap[idx].reachable || !aiMap[idx].owned || aiMap[idx].farmed)
                continue;
            RTTR_Assert(queries_.GetBuildingQuality(curPt)
                        == aiMap[curPt].bq); // Temporary, to check if aiMap is correctly update, see below
            const BuildingQuality currentBQ = queries_.GetBuildingQuality(curPt);
            if(!canUseBq(currentBQ, size)) // map[idx].bq; TODO: Update nodes BQ and use that
                continue;
            if(res != AIResource::Borderland && queries_.IsReservedMilitaryBorderSlot(curPt, currentBQ))
                continue;
            // special case fish -> check for other fishery buildings
            if(res == AIResource::Fish && queries_.isBuildingNearby(BuildingType::Fishery, curPt, 5))
                continue;
            if(res == AIResource::Borderland
               && (world_.IsOnRoad(world_.GetNeighbour(curPt, Direction::SouthEast))
                   || world_.IsInsideComputerBarrier(curPt)))
                continue;
            // dont build next to empty harborspots
            if(queries_.isHarborPosClose(curPt, 2, true))
                continue;
            auto worst = topSet.insert({curPt, map[idx]});
            if(worst)
            {
                min_value = worst.value().rating;
            }
        }
    }
    return topSet;
}

std::pair<MapPoint, int> AIResourceMap::findBestPosition(const MapPoint& pt, BuildingQuality size, unsigned radius,
                                                         int minimum) const
{
    MapPoint best = MapPoint::Invalid();
    int best_value = (minimum == std::numeric_limits<int>::min()) ? minimum : minimum - 1;
    std::vector<MapPoint> pts = world_.GetPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& curPt : pts)
    {
        const unsigned idx = map.GetIdx(curPt);

        if(map[idx] > best_value)
        {
            if(!aiMap[idx].reachable || !aiMap[idx].owned || aiMap[idx].farmed)
                continue;
            RTTR_Assert(queries_.GetBuildingQuality(curPt)
                        == aiMap[curPt].bq); // Temporary, to check if aiMap is correctly update, see below
            const BuildingQuality currentBQ = queries_.GetBuildingQuality(curPt);
            if(!canUseBq(currentBQ, size)) // map[idx].bq; TODO: Update nodes BQ and use that
                continue;
            if(res != AIResource::Borderland && queries_.IsReservedMilitaryBorderSlot(curPt, currentBQ))
                continue;
            // special case fish -> check for other fishery buildings
            if(res == AIResource::Fish && queries_.isBuildingNearby(BuildingType::Fishery, curPt, 5))
                continue;
            if(res == AIResource::Borderland
               && (world_.IsOnRoad(world_.GetNeighbour(curPt, Direction::SouthEast))
                   || world_.IsInsideComputerBarrier(curPt)))
                continue;
            // dont build next to empty harborspots
            if(queries_.isHarborPosClose(curPt, 2, true))
                continue;
            best = curPt;
            best_value = map[idx];
            // TODO: calculate "perfect" rating and instantly return if we got that already
        }
    }
    return {best, best_value};
}

void AIResourceMap::avoidPosition(const MapPoint& pt)
{
    map[pt] = 0;
}

void AIResourceMap::updateAroundDiminishable(const MapPoint& pt, const int radius)
{
    if(isInfinite)
        return;

    bool lastCircleValueCalculated = false;
    bool lastValueCalculated = false;
    // to avoid having to calculate a value twice and still move left on the same level without any problems we use this
    // variable to remember the first calculation we did in the circle.
    int circleStartValue = 0;

    for(MapCoord tx = world_.GetXA(pt, Direction::West), r = 1; r <= radius;
        tx = world_.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint curPt(tx, pt.y);
        for(const auto curDir : helpers::enumRange(Direction::NorthEast))
        {
            for(MapCoord step = 0; step < r; ++step, curPt = aiMap.GetNeighbour(curPt, curDir))
            {
                int& resMapVal = map[curPt];
                // only do a complete calculation for the first point or when moving outward and the last value is
                // unknown
                if((r < 2 || !lastCircleValueCalculated) && step < 1 && curDir == Direction::NorthEast && resMapVal)
                {
                    resMapVal = queries_.CalcResourceValue(curPt, res);
                    circleStartValue = resMapVal;
                    lastCircleValueCalculated = true;
                    lastValueCalculated = true;
                } else if(!resMapVal) // was there ever anything? if not skip it!
                {
                    if(step < 1 && curDir == Direction::NorthEast)
                        lastCircleValueCalculated = false;
                    lastValueCalculated = false;
                } else if(step < 1 && curDir == Direction::NorthEast) // circle not yet started? -> last direction was
                                                                      // outward (left=0)
                {
                    resMapVal = queries_.CalcResourceValue(curPt, res, Direction::West, circleStartValue);
                    circleStartValue = resMapVal;
                } else if(lastValueCalculated)
                {
                    if(step > 0) // we moved direction i%6
                        resMapVal = queries_.CalcResourceValue(curPt, res, curDir, resMapVal);
                    else // last step was the previous direction
                        resMapVal = queries_.CalcResourceValue(curPt, res, curDir - 1u, resMapVal);
                } else
                {
                    resMapVal = queries_.CalcResourceValue(curPt, res);
                    lastValueCalculated = true;
                }
            }
        }
    }
}

void AIResourceMap::updateAroundReplinishable(const MapPoint& pt, const int radius)
{
    // to avoid having to calculate a value twice and still move left on the same level without any problems we use this
    // variable to remember the first calculation we did in the circle.
    int circleStartValue = 0;

    int resValue = 0;
    for(MapCoord tx = world_.GetXA(pt, Direction::West), r = 1; r <= radius;
        tx = world_.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint curPt(tx, pt.y);
        for(const auto curDir : helpers::enumRange(Direction::NorthEast))
        {
            for(MapCoord step = 0; step < r; ++step, curPt = world_.GetNeighbour(curPt, curDir))
            {
                if(r == 1 && step == 0 && curDir == Direction::NorthEast)
                {
                    // only do a complete calculation for the first point!
                    resValue = queries_.CalcResourceValue(curPt, res);
                    circleStartValue = resValue;
                } else if(step == 0 && curDir == Direction::NorthEast)
                {
                    // circle not yet started? -> last direction was outward
                    resValue = queries_.CalcResourceValue(curPt, res, Direction::West, circleStartValue);
                    circleStartValue = resValue;
                } else if(step > 0) // we moved direction i%6
                    resValue = queries_.CalcResourceValue(curPt, res, curDir, resValue);
                else // last step was the previous direction
                    resValue = queries_.CalcResourceValue(curPt, res, curDir - 1u, resValue);
                map[curPt] = resValue;
            }
        }
    }
}
} // namespace AIJH
