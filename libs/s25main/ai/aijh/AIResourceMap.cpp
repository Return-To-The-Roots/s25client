// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

AIResourceMap::AIResourceMap(const AIResource res, const AIInterface& aii, const AIMap& aiMap)
    : res(res), resRadius(RES_RADIUS[static_cast<unsigned>(res)]), aii(aii), aiMap(aiMap)
{}

AIResourceMap::~AIResourceMap() = default;

void AIResourceMap::Init()
{
    const MapExtent mapSize = aiMap.GetSize();

    map.Resize(mapSize);
    RTTR_FOREACH_PT(MapPoint, mapSize)
    {
        const Node& node = aiMap[pt];
        if(res == AIResource::FISH && node.res == res)
            Change(pt, 1);
        else if(aii.gwb.GetDescription().get(aii.gwb.GetNode(pt).t1).Is(ETerrain::Walkable))
        {
            if((res != AIResource::BORDERLAND && node.res == res) || (res == AIResource::BORDERLAND && aii.IsBorder(pt))
               || (node.res == AIResource::MULTIPLE && (aii.GetSubsurfaceResource(pt) == res || aii.GetSurfaceResource(pt) == res)))
                Change(pt, 1);
        }
    }
}

void AIResourceMap::Recalc()
{
    Init();
    if(res == AIResource::WOOD) // existing woodcutters reduce rating
        AdjustRatingForBlds(BLD_WOODCUTTER, 7, -10);
    else if(res == AIResource::PLANTSPACE)
    {
        AdjustRatingForBlds(BLD_FARM, 3, -25);
        AdjustRatingForBlds(BLD_FORESTER, 6, -25);
    }
}

void AIResourceMap::AdjustRatingForBlds(BuildingType bld, unsigned radius, int value)
{
    const unsigned playerCt = aii.GetNumPlayers();
    for(unsigned i = 0; i < playerCt; i++)
    {
        const std::list<nobUsual*>& blds = aii.GetPlayerBuildings(bld, i);
        for(auto bld : blds)
            Change(bld->GetPos(), radius, value);
        const std::list<noBuildingSite*>& bldSites = aii.GetPlayerBuildingSites(i);
        for(auto bldSite : bldSites)
        {
            if(bldSite->GetBuildingType() == bld)
                Change(bldSite->GetPos(), radius, value);
        }
    }
}

namespace {
    struct ValueAdjuster
    {
        NodeMapBase<int>& map;
        unsigned radius;
        int value;

        ValueAdjuster(NodeMapBase<int>& map, unsigned radius, int value) : map(map), radius(radius), value(value) {}
        bool operator()(const MapPoint pt, unsigned r) const
        {
            map[pt] += value * (radius - r);
            return false; // Don't exit
        }
    };
} // namespace

void AIResourceMap::Change(const MapPoint pt, unsigned radius, int value)
{
    aii.gwb.CheckPointsInRadius(pt, radius, ValueAdjuster(map, radius, value), true);
}

MapPoint AIResourceMap::FindGoodPosition(const MapPoint& pt, int threshold, BuildingQuality size, int radius, bool inTerritory) const
{
    RTTR_Assert(pt.x < map.GetWidth() && pt.y < map.GetHeight());

    // TODO was besseres wär schön ;)
    if(radius == -1)
        radius = 30;

    std::vector<MapPoint> pts = aii.gwb.GetPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& curPt : pts)
    {
        const unsigned idx = map.GetIdx(curPt);
        if(map[idx] >= threshold)
        {
            if((inTerritory && !aiMap[idx].owned) || aiMap[idx].farmed)
                continue;
            RTTR_Assert(aii.GetBuildingQuality(curPt) == aiMap[curPt].bq);
            if(canUseBq(aii.GetBuildingQuality(curPt), size)) //(*nodes)[idx].bq; TODO: Update nodes BQ and use that
                return curPt;
        }
    }
    return MapPoint::Invalid();
}

MapPoint AIResourceMap::FindBestPosition(const MapPoint& pt, BuildingQuality size, int minimum, int radius, bool inTerritory) const
{
    RTTR_Assert(pt.x < map.GetWidth() && pt.y < map.GetHeight());

    // TODO was besseres wär schön ;)
    if(radius == -1)
        radius = 30;

    MapPoint best = MapPoint::Invalid();
    int best_value = (minimum == std::numeric_limits<int>::min()) ? minimum : minimum - 1;

    std::vector<MapPoint> pts = aii.gwb.GetPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& curPt : pts)
    {
        const unsigned idx = map.GetIdx(curPt);
        if(map[idx] > best_value)
        {
            if(!aiMap[idx].reachable || (inTerritory && !aiMap[idx].owned) || aiMap[idx].farmed)
                continue;
            RTTR_Assert(aii.GetBuildingQuality(curPt) == aiMap[curPt].bq);
            if(canUseBq(aii.GetBuildingQuality(curPt), size)) //(*nodes)[idx].bq; TODO: Update nodes BQ and use that
            {
                best = curPt;
                best_value = map[idx];
            }
        }
    }

    return best;
}

} // namespace AIJH
