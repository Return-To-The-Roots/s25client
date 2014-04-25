// $Id: AIResourceMap.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
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
    for (unsigned short y = 0; y < height; ++y)
    {
        for (unsigned short x = 0; x < width; ++x)
        {
            unsigned i = y * width + x;
            //resourceMaps[res][i] = 0;
            if (nodes[i].res == (AIJH::Resource)res && (AIJH::Resource)res != AIJH::BORDERLAND)
            {
                ChangeResourceMap(x, y, AIJH::RES_RADIUS[res], 1);
            }

            // Grenzgebiet"ressource"
            else if (nodes[i].border && (AIJH::Resource)res == AIJH::BORDERLAND)
            {
                ChangeResourceMap(x, y, AIJH::RES_RADIUS[AIJH::BORDERLAND], 1);
            }
        }
    }
}


void AIResourceMap::ChangeResourceMap(MapCoord x, MapCoord y, unsigned radius, int value)
{
    unsigned short width = gwb->GetWidth();

    map[x + y * width] += value * radius;

    for(MapCoord tx = gwb->GetXA(x, y, 0), r = 1; r <= radius; tx = gwb->GetXA(tx, y, 0), ++r)
    {
        MapCoord tx2 = tx, ty2 = y;
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; gwb->GetPointA(tx2, ty2, i % 6), ++r2)
            {
                unsigned i = tx2 + ty2 * width;
                map[i] += value * (radius - r);
            }
        }
    }
}


bool AIResourceMap::FindGoodPosition(MapCoord& x, MapCoord& y, int threshold, BuildingQuality size, int radius, bool inTerritory)
{
    unsigned short width = gwb->GetWidth();
    unsigned short height = gwb->GetHeight();

    if (x >= width || y >= height)
    {
        assert(false);
        //x = player->hqx;
        //y = player->hqy;
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    for(MapCoord tx = gwb->GetXA(x, y, 0), r = 1; r <= radius; tx = gwb->GetXA(tx, y, 0), ++r)
    {
        MapCoord tx2 = tx, ty2 = y;
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; gwb->GetPointA(tx2, ty2, i % 6), ++r2)
            {
                unsigned i = tx2 + ty2 * width;
                if (map[i] >= threshold)
                {
                    if ((inTerritory && !nodes[i].owned) || nodes[i].farmed)
                        continue;
                    if ( (nodes[i].bq >= size && nodes[i].bq < BQ_MINE) // normales Gebäude
                            || (nodes[i].bq == size))   // auch Bergwerke
                    {
                        x = tx2;
                        y = ty2;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


bool AIResourceMap::FindBestPosition(MapCoord& x, MapCoord& y, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
    unsigned short width = gwb->GetWidth();
    unsigned short height = gwb->GetHeight();

    if (x >= width || y >= height)
    {
        assert(false);
        //x = player->hqx;
        //y = player->hqy;
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    int best_x = 0, best_y = 0, best_value;
    best_value = -1;

    for(MapCoord tx = gwb->GetXA(x, y, 0), r = 1; r <= radius; tx = gwb->GetXA(tx, y, 0), ++r)
    {
        MapCoord tx2 = tx, ty2 = y;
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; gwb->GetPointA(tx2, ty2, i % 6), ++r2)
            {
                unsigned i = tx2 + ty2 * width;
                if (map[i] > best_value)
                {
                    if (!nodes[i].reachable || (inTerritory && !nodes[i].owned) || nodes[i].farmed)
                        continue;
                    if ( (nodes[i].bq >= size && nodes[i].bq < BQ_MINE) // normales Gebäude
                            || (nodes[i].bq == size))   // auch Bergwerke
                    {
                        best_x = tx2;
                        best_y = ty2;
                        best_value = map[i];
                    }
                }
            }
        }
    }

    if (best_value >= minimum)
    {
        x = best_x;
        y = best_y;
        return true;
    }
    return false;
}