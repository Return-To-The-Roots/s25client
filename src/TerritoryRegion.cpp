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
#include "main.h"
#include "TerritoryRegion.h"

#include "nobBaseMilitary.h"
#include "nobMilitary.h"
#include "MilitaryConsts.h"
#include "GameWorld.h"

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
    // Feld löschen
    delete [] nodes;
}

bool TerritoryRegion::IsPointInPolygon(GameWorldBase* gwb, std::vector< Point<MapCoord> > &polygon, MapCoord x, MapCoord y)
{
// Adapted from http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
// The site contains a lot of details and information.

    bool ret = false;

    std::vector< Point<MapCoord> >::iterator it = polygon.begin();
    std::vector< Point<MapCoord> >::iterator prev = polygon.end() - 1;

    for (; it < polygon.end(); prev = it, ++it)
    {
        if ((((*it).y > y) != ((*prev).y > y)) && (x < ((*prev).x - (*it).x) * (y - (*it).y) / ((*prev).y - (*it).y) + (*it).x))
        {
            ret = !ret;
        }
    }

    return(ret);
}

bool TerritoryRegion::IsPointValid(GameWorldBase* gwb, std::vector< Point<MapCoord> > &polygon, MapCoord x, MapCoord y)
{
    // This is for specifying polyons that wrap around corners:
    // - e.g. w=64, h=64, polygon = {(40,40), (40,80), (80,80), (80,40)}
    return(polygon.empty() ||
           IsPointInPolygon(gwb, polygon, x, y) ||
           IsPointInPolygon(gwb, polygon, x + gwb->GetWidth(), y) ||
           IsPointInPolygon(gwb, polygon, x, y + gwb->GetHeight()) ||
           IsPointInPolygon(gwb, polygon, x + gwb->GetWidth(), y + gwb->GetHeight()));
}


void TerritoryRegion::TestNode( int x,  int y, const unsigned char player, const unsigned char radius, const bool check_barriers)
{
    // Gucken, ob der Punkt überhaupt mit in diese Region gehört
    if(x + gwb->GetWidth() >= int(x1) && x + gwb->GetWidth() < int(x2))
        x += gwb->GetWidth();
    else if(x - gwb->GetWidth() >= int(x1) && x - gwb->GetWidth() < int(x2))
        x -= gwb->GetWidth();
    else if(x < int(x1) || x >= int(x2))
        return;

    if(y + gwb->GetHeight() >= int(y1) && y + gwb->GetHeight() < int(y2))
        y += gwb->GetHeight();
    else if(y - gwb->GetHeight() >= int(y1) && y - gwb->GetHeight() < int(y2))
        y -= gwb->GetHeight();
    else if(y < int(y1) || y >= int(y2))
        return;

    // check whether his node is within the area we may have territory in
    if (check_barriers && !IsPointValid(gwb, gwb->GetPlayer(player)->GetRestrictedArea(), x, y))
        return;

    /// Wenn das Militargebäude jetzt näher dran ist, dann geht dieser Punkt in den Besitz vom jeweiligen Spieler
    /// oder wenn es halt gar nicht besetzt ist
    unsigned idx = (y - y1) * (x2 - x1) + (x - x1);
    if(radius < nodes[idx].radius || !nodes[(y - y1) * (x2 - x1) + (x - x1)].owner)
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

    // Punkt, auf dem das Militärgebäude steht
    MapCoord x = building->GetX(), y = building->GetY();
    TestNode(x, y, building->GetPlayer(), 0, false);    // no need to check barriers here. this point is on our territory.

    for(unsigned r = 1; r <= radius; ++r)
    {
        // Eins weiter nach links gehen
        gwb->GetPointA(x, y, 0);

        for(unsigned dir = 0; dir < 6; ++dir)
        {
            for(unsigned short i = 0; i < r; ++i)
            {
                TestNode(x, y, building->GetPlayer(), r, check_barriers);
                // Nach rechts oben anfangen
                gwb->GetPointA(x, y, (2 + dir) % 6);
            }
        }
    }
}


//for(unsigned short x = fx;x < building->GetX()-radius+(y+(building->GetY()&1))/2+radius*2+1-y;++x)
//              TestNode(x,building->GetY()+y,building->GetPlayer(),y);
