// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "AIResourceMap.h"
#include "RttrForeachPt.h"
#include "ai/AIInterface.h"
#include "ai/aijh/AIMap.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobUsual.h"
#include "gameData/TerrainDesc.h"

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

AIResourceMap::AIResourceMap(const AIResource res, bool isInfinite, const AIInterface& aii, const AIMap& aiMap)
    : res(res), isInfinite(isInfinite), isDiminishableResource(isDiminishable(res)), resRadius(RES_RADIUS[res]),
      aii(aii), aiMap(aiMap)
{}

AIResourceMap::~AIResourceMap() = default;

void AIResourceMap::init()
{
    const MapExtent mapSize = aiMap.GetSize();

    map.Resize(mapSize);
    // Calculate value for each point.
    // This is quite expensive so do just for the diminishable resources to sort out ones where there will never be
    // anything, which allows an optimization when calculating the value which must always be done on demand
    if(isDiminishableResource)
    {
        RTTR_FOREACH_PT(MapPoint, mapSize)
        {
            bool isValid = true;
            if(res == AIResource::Fish)
            {
                isValid =
                  aii.gwb.IsOfTerrain(pt, [](const TerrainDesc& desc) { return desc.kind == TerrainKind::Water; });
            } else if(res == AIResource::Stones)
            {
                isValid = aii.gwb.IsOfTerrain(pt, [](const TerrainDesc& desc) { return desc.Is(ETerrain::Buildable); });
            } else //= granite,gold,iron,coal
            {
                isValid = aii.gwb.IsOfTerrain(pt, [](const TerrainDesc& desc) { return desc.Is(ETerrain::Mineable); });
            }
            map[pt] = isValid ? aii.CalcResourceValue(pt, res) : 0;
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

MapPoint AIResourceMap::findBestPosition(const MapPoint& pt, BuildingQuality size, unsigned radius, int minimum) const
{
    MapPoint best = MapPoint::Invalid();
    int best_value = (minimum == std::numeric_limits<int>::min()) ? minimum : minimum - 1;

    std::vector<MapPoint> pts = aii.gwb.GetPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& curPt : pts)
    {
        const unsigned idx = map.GetIdx(curPt);
        if(map[idx] > best_value)
        {
            if(!aiMap[idx].reachable || !aiMap[idx].owned || aiMap[idx].farmed)
                continue;
            RTTR_Assert(aii.GetBuildingQuality(curPt)
                        == aiMap[curPt].bq); // Temporary, to check if aiMap is correctly update, see below
            if(!canUseBq(aii.GetBuildingQuality(curPt), size)) // map[idx].bq; TODO: Update nodes BQ and use that
                continue;
            // special case fish -> check for other fishery buildings
            if(res == AIResource::Fish && aii.isBuildingNearby(BuildingType::Fishery, curPt, 5))
                continue;
            if(res == AIResource::Borderland && aii.gwb.IsOnRoad(aii.gwb.GetNeighbour(curPt, Direction::SouthEast)))
                continue;
            // dont build next to empty harborspots
            if(aii.isHarborPosClose(curPt, 2, true))
                continue;
            best = curPt;
            best_value = map[idx];
            // TODO: calculate "perfect" rating and instantly return if we got that already
        }
    }

    return best;
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

    for(MapCoord tx = aii.gwb.GetXA(pt, Direction::West), r = 1; r <= radius;
        tx = aii.gwb.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
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
                    resMapVal = aii.CalcResourceValue(curPt, res);
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
                    resMapVal = aii.CalcResourceValue(curPt, res, Direction::West, circleStartValue);
                    circleStartValue = resMapVal;
                } else if(lastValueCalculated)
                {
                    if(step > 0) // we moved direction i%6
                        resMapVal = aii.CalcResourceValue(curPt, res, curDir, resMapVal);
                    else // last step was the previous direction
                        resMapVal = aii.CalcResourceValue(curPt, res, curDir - 1u, resMapVal);
                } else
                {
                    resMapVal = aii.CalcResourceValue(curPt, res);
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
    for(MapCoord tx = aii.gwb.GetXA(pt, Direction::West), r = 1; r <= radius;
        tx = aii.gwb.GetXA(MapPoint(tx, pt.y), Direction::West), ++r)
    {
        MapPoint curPt(tx, pt.y);
        for(const auto curDir : helpers::enumRange(Direction::NorthEast))
        {
            for(MapCoord step = 0; step < r; ++step, curPt = aii.gwb.GetNeighbour(curPt, curDir))
            {
                if(r == 1 && step == 0 && curDir == Direction::NorthEast)
                {
                    // only do a complete calculation for the first point!
                    resValue = aii.CalcResourceValue(curPt, res);
                    circleStartValue = resValue;
                } else if(step == 0 && curDir == Direction::NorthEast)
                {
                    // circle not yet started? -> last direction was outward
                    resValue = aii.CalcResourceValue(curPt, res, Direction::West, circleStartValue);
                    circleStartValue = resValue;
                } else if(step > 0) // we moved direction i%6
                    resValue = aii.CalcResourceValue(curPt, res, curDir, resValue);
                else // last step was the previous direction
                    resValue = aii.CalcResourceValue(curPt, res, curDir - 1u, resValue);
                map[curPt] = resValue;
            }
        }
    }
}

} // namespace AIJH
