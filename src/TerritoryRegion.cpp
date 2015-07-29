// $Id: TerritoryRegion.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "defines.h"
#include "TerritoryRegion.h"

#include "buildings/nobBaseMilitary.h"
#include "buildings/nobMilitary.h"
#include "gameData/MilitaryConsts.h"
#include "GameWorld.h"
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TerritoryRegion::TerritoryRegion(const int x1, const int y1, const int x2, const int y2, GameWorldBase* const gwb)
    : x1(x1), y1(y1), x2(x2), y2(y2), width(x2 - x1), height(y2 - y1), gwb(gwb)
{
    // Feld erzeugen
    nodes = new TRNode[(x2 - x1) * (y2 - y1)];

    // und erstmal hat es niemand im Besitz
    memset(nodes, 0, sizeof(TRNode) * (x2 - x1) * (y2 - y1));
}

TerritoryRegion::~TerritoryRegion()
{
    // Feld lï¿½schen
    delete [] nodes;
}

bool TerritoryRegion::IsPointInPolygon(GameWorldBase* gwb, std::vector< MapPoint > &polygon, const MapPoint pt)
{
// Adapted from http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
// The site contains a lot of details and information.

    bool ret = false;

    std::vector< MapPoint >::iterator it = polygon.begin();
    std::vector< MapPoint >::iterator prev = polygon.end() - 1;

    for (; it < polygon.end(); prev = it, ++it)
    {
        if (((it->y > pt.y) != (prev->y > pt.y)) && (pt.x < (prev->x - it->x) * (pt.y - it->y) / (prev->y - it->y) + it->x))
        {
            ret = !ret;
        }
    }

    return(ret);
}

bool TerritoryRegion::IsPointValid(GameWorldBase* gwb, std::vector< MapPoint > &polygon, const MapPoint pt)
{
    // This is for specifying polyons that wrap around corners:
    // - e.g. w=64, h=64, polygon = {(40,40), (40,80), (80,80), (80,40)}
    MapPoint pt2 = MapPoint(pt.x + gwb->GetWidth(), pt.y),
             pt3 = MapPoint(pt.x, pt.y + gwb->GetHeight()),
             pt4 = MapPoint(pt.x + gwb->GetWidth(), pt.y + gwb->GetHeight());
    return(polygon.empty() ||
           IsPointInPolygon(gwb, polygon, pt) ||
           IsPointInPolygon(gwb, polygon, pt2) ||
           IsPointInPolygon(gwb, polygon, pt3) ||
           IsPointInPolygon(gwb, polygon, pt4));
}


void TerritoryRegion::TestNode(MapPoint pt, const unsigned char player, const unsigned char radius, const bool check_barriers)
{
    // check whether his node is within the area we may have territory in
    if (check_barriers && !IsPointValid(gwb, gwb->GetPlayer(player)->GetRestrictedArea(), pt))
        return;

    // Gucken, ob der Punkt ï¿½berhaupt mit in diese Region gehï¿½rt
    if(pt.x + gwb->GetWidth() >= int(x1) && pt.x + gwb->GetWidth() < int(x2))
        pt.x += gwb->GetWidth();
    else if(pt.x - gwb->GetWidth() >= int(x1) && pt.x - gwb->GetWidth() < int(x2))
        pt.x -= gwb->GetWidth();
    else if(pt.x < int(x1) || pt.x >= int(x2))
        return;

    if(pt.y + gwb->GetHeight() >= int(y1) && pt.y + gwb->GetHeight() < int(y2))
        pt.y += gwb->GetHeight();
    else if(pt.y - gwb->GetHeight() >= int(y1) && pt.y - gwb->GetHeight() < int(y2))
        pt.y -= gwb->GetHeight();
    else if(pt.y < int(y1) || pt.y >= int(y2))
        return;

    /// Wenn das Militargebï¿½ude jetzt nï¿½her dran ist, dann geht dieser Punkt in den Besitz vom jeweiligen Spieler
    /// oder wenn es halt gar nicht besetzt ist
    unsigned idx = (pt.y - y1) * (x2 - x1) + (pt.x - x1);

    if(radius < nodes[idx].radius || !nodes[idx].owner)
    {
        nodes[idx].owner = player + 1;
        nodes[idx].radius = radius;
    }
}

void TerritoryRegion::CalcTerritoryOfBuilding(const noBaseBuilding* const building)
{
    bool check_barriers = true;
    unsigned short radius;

    if(building->GetBuildingType() == BLD_HARBORBUILDING)
        radius = HARBOR_ALONE_RADIUS;
    else
        radius = static_cast<const nobBaseMilitary*>(building)->GetMilitaryRadius();

    if (building->GetGOT() == GOT_NOB_MILITARY)
    {
        // we don't check barriers for captured buildings
        check_barriers = !(static_cast<const nobMilitary*>(building)->WasCapturedOnce());
    }

    // Punkt, auf dem das Militï¿½rgebï¿½ude steht
    MapPoint pt = building->GetPos();
    TestNode(pt, building->GetPlayer(), 0, false);    // no need to check barriers here. this point is on our territory.

    for(unsigned r = 1; r <= radius; ++r)
    {
        // Eins weiter nach links gehen
        pt = gwb->GetNeighbour(pt, 0);

        for(unsigned dir = 0; dir < 6; ++dir)
        {
            for(unsigned short i = 0; i < r; ++i)
            {
                TestNode(pt, building->GetPlayer(), r, check_barriers);
                // Nach rechts oben anfangen
                pt = gwb->GetNeighbour(pt, (2 + dir) % 6);
            }
        }
    }
}


//for(unsigned short x = fx;x < building->GetX()-radius+(y+(building->GetY()&1))/2+radius*2+1-y;++x)
//              TestNode(x,building->GetY()+y,building->GetPlayer(),y);
