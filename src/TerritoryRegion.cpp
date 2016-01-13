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
#include "TerritoryRegion.h"

#include "buildings/nobBaseMilitary.h"
#include "buildings/nobMilitary.h"
#include "GameClientPlayer.h"
#include "gameData/MilitaryConsts.h"
#include "GameWorldBase.h"
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
    RTTR_Assert(x1 <= x2);
    RTTR_Assert(y1 <= y2);
    // Feld erzeugen
    nodes.resize((x2 - x1) * (y2 - y1));
}

TerritoryRegion::~TerritoryRegion()
{}

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
    int x = static_cast<int>(pt.x), y = static_cast<int>(pt.y);

    // Check if this point is inside this region
    // Apply wrap-around if on either side
    if(x < x1)
        x += gwb->GetWidth();
    else if(x >= x2)
        x -= gwb->GetWidth();
    // Check the (possibly) adjusted point
    if(x < x1 || x >= x2)
        return;

    // Apply wrap-around if on either side
    if(y < y1)
        y += gwb->GetHeight();
    else if(y >= y2)
        y -= gwb->GetHeight();
    // Check the (possibly) adjusted point
    if(y < y1 || y >= y2)
        return;

    // check whether his node is within the area we may have territory in
    if (check_barriers && !IsPointValid(gwb, gwb->GetPlayer(player).GetRestrictedArea(), pt))
        return;

    /// Wenn das Militargebäude jetzt näher dran ist, dann geht dieser Punkt in den Besitz vom jeweiligen Spieler
    /// oder wenn es halt gar nicht besetzt ist
    unsigned idx = (y - y1) * (x2 - x1) + (x - x1);

    if(radius < nodes[idx].radius || !nodes[idx].owner)
    {
        nodes[idx].owner = player + 1;
        nodes[idx].radius = radius;
    }
}

struct GetMapPointWithRadius
{
    typedef std::pair<MapPoint, unsigned> result_type;

    result_type operator()(const MapPoint pt, unsigned r)
    {
        return std::make_pair(pt, r);
    }
};

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
    MapPoint pt = building->GetPos();
    TestNode(pt, building->GetPlayer(), 0, false);    // no need to check barriers here. this point is on our territory.

    std::vector<GetMapPointWithRadius::result_type> pts = gwb->GetPointsInRadius(pt, radius, GetMapPointWithRadius());
    for(std::vector<GetMapPointWithRadius::result_type>::const_iterator it = pts.begin(); it != pts.end(); ++it)
        TestNode(it->first, building->GetPlayer(), it->second, check_barriers);
}
