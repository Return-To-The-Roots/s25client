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

#include "defines.h" // IWYU pragma: keep
#include "world/TerritoryRegion.h"

#include "GamePlayer.h"
#include "buildings/nobBaseMilitary.h"
#include "buildings/nobMilitary.h"
#include "world/GameWorldBase.h"
#include "gameData/MilitaryConsts.h"

TerritoryRegion::TerritoryRegion(const PointI& startPt, const PointI& endPt, const GameWorldBase& gwb)
    : startPt(startPt), endPt(endPt), size(endPt - startPt), world(gwb)
{
    RTTR_Assert(startPt.x <= endPt.x);
    RTTR_Assert(startPt.y <= endPt.y);
    RTTR_Assert(size.x <= gwb.GetWidth());
    RTTR_Assert(size.y <= gwb.GetHeight());
    // Feld erzeugen
    nodes.resize(size.x * size.y);
}

TerritoryRegion::~TerritoryRegion()
{
}

bool TerritoryRegion::IsPointInPolygon(const std::vector<Point<int> >& polygon, const Point<int> pt)
{
    // Adapted from http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
    // The site contains a lot of details and information.

    bool inside = false;

    std::vector<Point<int> >::const_iterator it = polygon.begin();
    std::vector<Point<int> >::const_iterator prev = polygon.end() - 1;
    // Check each edge if a ray from the point to the right crosses the edge
    for(; it != polygon.end(); prev = it, ++it)
    {
        // Check if the edge crosses the horizontal line at height pt.y
        // Includes edge point at lower coord and excludes the one at higher coord
        if((it->y > pt.y) == (prev->y > pt.y))
            continue;
        // Check if the intersection point in X is right of our point
        // Original formula for (signed) floating point:
        // if (pt.x < (prev->x - it->x) * (pt.y - it->y) / (prev->y - it->y) + it->x)
        // ==>(pt.x - it->x < (prev->x - it->x) * (pt.y - it->y) / (prev->y - it->y))
        const int dy = prev->y - it->y;
        const int lhs = (pt.x - it->x) * dy;
        const int rhs = (prev->x - it->x) * (pt.y - it->y);

        if((dy < 0 && lhs > rhs) || (dy > 0 && lhs < rhs))
            inside = !inside;
    }

    return (inside);
}

bool TerritoryRegion::IsPointValid(const GameWorldBase& gwb, const std::vector<MapPoint>& polygon, const MapPoint pt)
{
    typedef Point<int> PointI;
    // This is for specifying polyons that wrap around corners:
    // - e.g. w=64, h=64, polygon = {(40,40), (40,80), (80,80), (80,40)}
    PointI pt2(pt.x + gwb.GetWidth(), pt.y), pt3(pt.x, pt.y + gwb.GetHeight()), pt4(pt.x + gwb.GetWidth(), pt.y + gwb.GetHeight());
    const std::vector<PointI> polygonInt(polygon.begin(), polygon.end());
    return (polygon.empty() || IsPointInPolygon(polygonInt, PointI(pt)) || IsPointInPolygon(polygonInt, pt2)
            || IsPointInPolygon(polygonInt, pt3) || IsPointInPolygon(polygonInt, pt4));
}

void TerritoryRegion::AdjustNode(MapPoint pt, const unsigned char player, const unsigned char radius, const bool check_barriers)
{
    PointI realPt(pt);

    // Check if this point is inside this region
    // Apply wrap-around if on either side
    if(realPt.x < startPt.x)
        realPt.x += world.GetWidth();
    else if(realPt.x >= endPt.x)
        realPt.x -= world.GetWidth();
    // Check the (possibly) adjusted point
    if(realPt.x < startPt.x || realPt.x >= endPt.x)
        return;

    // Apply wrap-around if on either side
    if(realPt.y < startPt.y)
        realPt.y += world.GetHeight();
    else if(realPt.y >= endPt.y)
        realPt.y -= world.GetHeight();
    // Check the (possibly) adjusted point
    if(realPt.y < startPt.y || realPt.y >= endPt.y)
        return;

    // check whether this node is within the area we may have territory in
    if(check_barriers && !IsPointValid(world, world.GetPlayer(player).GetRestrictedArea(), pt))
        return;

    /// Wenn das Militargebäude jetzt näher dran ist, dann geht dieser Punkt in den Besitz vom jeweiligen Spieler
    /// oder wenn es halt gar nicht besetzt ist
    TRNode& node = GetNode(realPt);

    if(radius < node.radius || !node.owner)
    {
        node.owner = player + 1;
        node.radius = radius;
    }
}

namespace {
struct GetMapPointWithRadius
{
    typedef std::pair<MapPoint, unsigned> result_type;

    result_type operator()(const MapPoint pt, unsigned r) { return std::make_pair(pt, r); }
};
} // namespace

void TerritoryRegion::CalcTerritoryOfBuilding(const noBaseBuilding& building)
{
    bool check_barriers = true;
    unsigned short radius;

    if(building.GetBuildingType() == BLD_HARBORBUILDING)
        radius = HARBOR_RADIUS;
    else
        radius = static_cast<const nobBaseMilitary&>(building).GetMilitaryRadius();

    if(building.GetGOT() == GOT_NOB_MILITARY)
    {
        // we don't check barriers for captured buildings
        check_barriers = !static_cast<const nobMilitary&>(building).WasCapturedOnce();
    }

    // Punkt, auf dem das Militärgebäude steht
    MapPoint pt = building.GetPos();
    AdjustNode(pt, building.GetPlayer(), 0, false); // no need to check barriers here. this point is on our territory.

    std::vector<GetMapPointWithRadius::result_type> pts = world.GetPointsInRadius(pt, radius, GetMapPointWithRadius());
    for(std::vector<GetMapPointWithRadius::result_type>::const_iterator it = pts.begin(); it != pts.end(); ++it)
        AdjustNode(it->first, building.GetPlayer(), it->second, check_barriers);
}
