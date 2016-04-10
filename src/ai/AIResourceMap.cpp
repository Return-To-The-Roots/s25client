// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "AIResourceMap.h"
#include "AIJHHelper.h"
#include "gameData/TerrainData.h"
#include "buildings/nobUsual.h"
#include "buildings/noBuildingSite.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

AIResourceMap::AIResourceMap(const AIJH::Resource res, const AIInterface& aii, const std::vector<AIJH::Node> &nodes)
    : res(res), aii(&aii), nodes(&nodes), resRadius(AIJH::RES_RADIUS[res])
{
}

AIResourceMap::~AIResourceMap()
{
}

void AIResourceMap::Init()
{
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();

    map.clear();
    map.resize(width * height);
    for (MapPoint pt(0, 0); pt.y < height; ++pt.y)
    {
        for (pt.x = 0; pt.x < width; ++pt.x)
        {
            unsigned i = aii->GetIdx(pt);
            if ((*nodes)[i].res == res && res != AIJH::BORDERLAND && TerrainData::IsUseable(aii->GetTerrain(pt)))
            {
                Change(pt, 1);
            }
            else if (res == AIJH::BORDERLAND && aii->IsBorder(pt))
            {
                //only count border area that is actually passable terrain
                if(TerrainData::IsUseable(aii->GetTerrain(pt)))
                    Change(pt, 1);
            }
            if ((*nodes)[i].res == AIJH::MULTIPLE && TerrainData::IsUseable(aii->GetTerrain(pt)) )
            {
                if(aii->GetSubsurfaceResource(pt) == res || aii->GetSurfaceResource(pt) == res)
                    Change(pt, 1);
            }
        }
    }
}

void AIResourceMap::Recalc()
{
    Init();
    if(res == AIJH::WOOD) //existing woodcutters reduce rating      
        AdjustRatingForBlds(BLD_WOODCUTTER, 7, -10);
    else if(res == AIJH::PLANTSPACE)
    {
        AdjustRatingForBlds(BLD_FARM, 3, -25);
        AdjustRatingForBlds(BLD_FORESTER, 6, -25);
    }
}

void AIResourceMap::AdjustRatingForBlds(BuildingType bld, unsigned radius, int value)
{
    const unsigned playerCt = aii->GetPlayerCount();
    for(unsigned i=0; i<playerCt; i++)
    {
        const std::list<nobUsual*>& blds = aii->GetPlayerBuildings(bld, i);
        for(std::list<nobUsual*>::const_iterator it = blds.begin(); it != blds.end(); ++it)
            Change((*it)->GetPos(), radius, value);
        const std::list<noBuildingSite*>& bldSites = aii->GetPlayerBuildingSites(i);
        for(std::list<noBuildingSite*>::const_iterator it = bldSites.begin(); it != bldSites.end(); ++it)
        {
            if((*it)->GetBuildingType() == bld)
                Change((*it)->GetPos(), radius, value);
        }
    }
}

namespace{
    struct MapPoint2IdxWithRadius
    {
        typedef std::pair<unsigned, unsigned> result_type;
        const AIInterface& aii_;

        MapPoint2IdxWithRadius(const AIInterface& aii): aii_(aii){}
        result_type operator()(const MapPoint pt, unsigned r)
        {
            return std::make_pair(aii_.GetIdx(pt), r);
        }
    };
}

void AIResourceMap::Change(const MapPoint pt, unsigned radius, int value)
{
    map[aii->GetIdx(pt)] += value * radius;
    std::vector<MapPoint2IdxWithRadius::result_type> pts = aii->GetPointsInRadius(pt, radius, MapPoint2IdxWithRadius(*aii));
    for(std::vector<MapPoint2IdxWithRadius::result_type>::iterator it = pts.begin(); it != pts.end(); ++it)
        map[it->first] += value * (radius - it->second);
}

bool AIResourceMap::FindGoodPosition(MapPoint& pt, int threshold, BuildingQuality size, int radius, bool inTerritory)
{
    RTTR_Assert(pt.x < aii->GetMapWidth() && pt.y < aii->GetMapHeight());

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    std::vector<MapPoint> pts = aii->GetPointsInRadius(pt, radius);
    for(std::vector<MapPoint>::iterator it = pts.begin(); it != pts.end(); ++it)
    {
        const unsigned idx = aii->GetIdx(*it);
        if (map[idx] >= threshold)
        {
            if ((inTerritory && !(*nodes)[idx].owned) || (*nodes)[idx].farmed)
                continue;
            const BuildingQuality bq = aii->GetBuildingQuality(*it); //(*nodes)[idx].bq; TODO: Update nodes BQ and use that
            if ( (bq >= size && bq < BQ_MINE) // normales Gebäude
                || (bq == size))   // auch Bergwerke
            {
                pt = *it;
                return true;
            }
        }
    }
    return false;
}


bool AIResourceMap::FindBestPosition(MapPoint& pt, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
    RTTR_Assert(pt.x < aii->GetMapWidth() && pt.y < aii->GetMapHeight());

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    MapPoint best(0, 0);
    int best_value = -1;

    std::vector<MapPoint> pts = aii->GetPointsInRadius(pt, radius);
    for(std::vector<MapPoint>::iterator it = pts.begin(); it != pts.end(); ++it)
    {
        const unsigned idx = aii->GetIdx(*it);
        if (map[idx] > best_value)
        {
            if (!(*nodes)[idx].reachable || (inTerritory && !(*nodes)[idx].owned) || (*nodes)[idx].farmed)
                continue;
            const BuildingQuality bq = aii->GetBuildingQuality(*it); //(*nodes)[idx].bq; TODO: Update nodes BQ and use that
            if ( (bq >= size && bq < BQ_MINE) // normales Gebäude
                || (bq == size))   // auch Bergwerke
            {
                best = *it;
                best_value = map[idx];
            }
        }
    }

    if (best_value >= minimum)
    {
        pt = best;
        return true;
    }
    return false;
}
