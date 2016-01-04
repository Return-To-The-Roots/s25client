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
#include "defines.h"
#include "AIResourceMap.h"

AIResourceMap::AIResourceMap(const GameWorldBase& gwb, const std::vector<AIJH::Node> &nodes)
    : gwb(gwb), nodes(nodes)
{
}

AIResourceMap::~AIResourceMap(void)
{
}

void AIResourceMap::Init()
{
    unsigned short width = gwb.GetWidth();
    unsigned short height = gwb.GetHeight();

    map.clear();
    map.resize(width * height);
    for (MapPoint pt(0, 0); pt.y < height; ++pt.y)
    {
        for (pt.x = 0; pt.x < width; ++pt.x)
        {
            unsigned i = gwb.GetIdx(pt);
            if (nodes[i].res == res && res != AIJH::BORDERLAND)
            {
                ChangeResourceMap(pt, AIJH::RES_RADIUS[res], 1);
            }else if (nodes[i].border && res == AIJH::BORDERLAND)
            {
                ChangeResourceMap(pt, AIJH::RES_RADIUS[AIJH::BORDERLAND], 1);
            }
        }
    }
}

struct MapPoint2IdxWithRadius
{
    typedef std::pair<unsigned, unsigned> result_type;
    const GameWorldBase& gwb_;

    MapPoint2IdxWithRadius(const GameWorldBase& gwb): gwb_(gwb){}
    result_type operator()(const MapPoint pt, unsigned r)
    {
        return std::make_pair(gwb_.GetIdx(pt), r);
    }
};

struct MapPoint2Idx
{
    typedef unsigned result_type;
    const GameWorldBase& gwb_;

    MapPoint2Idx(const GameWorldBase& gwb): gwb_(gwb){}
    result_type operator()(const MapPoint pt, unsigned r)
    {
        return gwb_.GetIdx(pt);
    }
};

void AIResourceMap::ChangeResourceMap(const MapPoint pt, unsigned radius, int value)
{
    map[gwb.GetIdx(pt)] += value * radius;
    std::vector<MapPoint2IdxWithRadius::result_type> pts = gwb.GetPointsInRadius(pt, radius, MapPoint2IdxWithRadius(gwb));

    for(std::vector<MapPoint2IdxWithRadius::result_type>::iterator it = pts.begin(); it != pts.end(); ++it)
        map[it->first] += value * (radius - it->second);
}


bool AIResourceMap::FindGoodPosition(MapPoint& pt, int threshold, BuildingQuality size, int radius, bool inTerritory)
{
    assert(pt.x < gwb.GetWidth() && pt.y < gwb.GetHeight());

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    std::vector<MapPoint> pts = gwb.GetPointsInRadius(pt, radius);
    for(std::vector<MapPoint>::iterator it = pts.begin(); it != pts.end(); ++it)
    {
        const unsigned idx = gwb.GetIdx(*it);
        if (map[idx] >= threshold)
        {
            if ((inTerritory && !nodes[idx].owned) || nodes[idx].farmed)
                continue;
            if ( (nodes[idx].bq >= size && nodes[idx].bq < BQ_MINE) // normales Gebäude
                || (nodes[idx].bq == size))   // auch Bergwerke
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
    assert(pt.x < gwb.GetWidth() && pt.y < gwb.GetHeight());

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    MapPoint best(0, 0);
    int best_value = -1;

    std::vector<MapPoint> pts = gwb.GetPointsInRadius(pt, radius);
    for(std::vector<MapPoint>::iterator it = pts.begin(); it != pts.end(); ++it)
    {
        const unsigned idx = gwb.GetIdx(*it);
        if (map[idx] > best_value)
        {
            if (!nodes[idx].reachable || (inTerritory && !nodes[idx].owned) || nodes[idx].farmed)
                continue;
            if ( (nodes[idx].bq >= size && nodes[idx].bq < BQ_MINE) // normales Gebäude
                || (nodes[idx].bq == size))   // auch Bergwerke
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
