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

AIResourceMap::AIResourceMap(const GameWorldBase* const gwb, const std::vector<AIJH::Node> &nodes)
    : gwb(gwb), nodes(nodes)
{
}

AIResourceMap::~AIResourceMap(void)
{
}

void AIResourceMap::Init()
{
    unsigned short width = gwb->GetWidth();
    unsigned short height = gwb->GetHeight();

    map.resize(AIJH::RES_TYPE_COUNT);

    map.resize(width * height);
    for (MapPoint pt(0, 0); pt.y < height; ++pt.y)
    {
        for (pt.x = 0; pt.x < width; ++pt.x)
        {
            unsigned i = gwb->GetIdx(pt);
            //resourceMaps[res][i] = 0;
            if (nodes[i].res == (AIJH::Resource)res && (AIJH::Resource)res != AIJH::BORDERLAND)
            {
                ChangeResourceMap(pt, AIJH::RES_RADIUS[res], 1);
            }

            // Grenzgebiet"ressource"
            else if (nodes[i].border && (AIJH::Resource)res == AIJH::BORDERLAND)
            {
                ChangeResourceMap(pt, AIJH::RES_RADIUS[AIJH::BORDERLAND], 1);
            }
        }
    }
}


void AIResourceMap::ChangeResourceMap(const MapPoint pt, unsigned radius, int value)
{
    map[gwb->GetIdx(pt)] += value * radius;

    for(MapCoord tx = gwb->GetXA(pt, 0), r = 1; r <= radius; tx = gwb->GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb->GetNeighbour(t2, i % 6), ++r2)
            {
                unsigned i = gwb->GetIdx(t2);
                map[i] += value * (radius - r);
            }
        }
    }
}


bool AIResourceMap::FindGoodPosition(MapPoint& pt, int threshold, BuildingQuality size, int radius, bool inTerritory)
{
    assert(pt.x < gwb->GetWidth() && pt.y < gwb->GetHeight());

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    for(MapCoord tx = gwb->GetXA(pt, 0), r = 1; r <= radius; tx = gwb->GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb->GetNeighbour(t2, i % 6), ++r2)
            {
                unsigned i = gwb->GetIdx(t2);
                if (map[i] >= threshold)
                {
                    if ((inTerritory && !nodes[i].owned) || nodes[i].farmed)
                        continue;
                    if ( (nodes[i].bq >= size && nodes[i].bq < BQ_MINE) // normales Gebäude
                            || (nodes[i].bq == size))   // auch Bergwerke
                    {
                        pt = t2;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


bool AIResourceMap::FindBestPosition(MapPoint& pt, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
    assert(pt.x < gwb->GetWidth() && pt.y < gwb->GetHeight());

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    MapPoint best(0, 0);
    int best_value = -1;

    for(MapCoord tx = gwb->GetXA(pt, 0), r = 1; r <= radius; tx = gwb->GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb->GetNeighbour(t2, i % 6), ++r2)
            {
                unsigned i = gwb->GetIdx(t2);
                if (map[i] > best_value)
                {
                    if (!nodes[i].reachable || (inTerritory && !nodes[i].owned) || nodes[i].farmed)
                        continue;
                    if ( (nodes[i].bq >= size && nodes[i].bq < BQ_MINE) // normales Gebäude
                            || (nodes[i].bq == size))   // auch Bergwerke
                    {
                        best = t2;
                        best_value = map[i];
                    }
                }
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
