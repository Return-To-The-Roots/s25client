// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "world/TerritoryRegion.h"
#include "GamePlayer.h"
#include "MapGeometry.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/nobMilitary.h"
#include "helpers/EnumRange.h"
#include "world/GameWorldBase.h"
#include <stdexcept>

TerritoryRegion::TerritoryRegion(const Position& startPt, const Extent& size, const GameWorldBase& gwb)
    : startPt(startPt), size(size), world(gwb)
{
    if(size.x > gwb.GetWidth() || size.y > gwb.GetHeight())
        throw std::runtime_error("Size to big for TerritoryRegion");
    // Feld erzeugen
    nodes.resize(size.x * size.y);
}

TerritoryRegion::~TerritoryRegion() = default;

bool TerritoryRegion::IsPointInPolygon(const std::vector<Position>& polygon, const Position& pt)
{
    // Adapted from http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
    // The site contains a lot of details and information.

    bool inside = false;

    auto it = polygon.begin();
    auto prev = polygon.end() - 1;
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

bool TerritoryRegion::IsPointValid(const MapExtent& mapSize, const std::vector<MapPoint>& polygon, const MapPoint pt)
{
    // This is for specifying polyons that wrap around corners:
    // - e.g. w=64, h=64, polygon = {(40,40), (40,80), (80,80), (80,40)}
    Position pt2(pt.x + mapSize.x, pt.y), pt3(pt.x, pt.y + mapSize.y), pt4(pt + mapSize);
    const std::vector<Position> polygonInt(polygon.begin(), polygon.end());
    return (polygon.empty() || IsPointInPolygon(polygonInt, Position(pt)) || IsPointInPolygon(polygonInt, pt2)
            || IsPointInPolygon(polygonInt, pt3) || IsPointInPolygon(polygonInt, pt4));
}

void TerritoryRegion::AdjustNode(MapPoint pt, uint8_t player, uint16_t radius, const std::vector<MapPoint>* allowedArea)
{
    TRNode* node = TryGetNode(pt);
    // Not in our region -> Out
    if(!node)
        return;

    // check whether this node is within the area we may have territory in
    if(allowedArea && !IsPointValid(world.GetSize(), *allowedArea, pt))
        return;

    /// If the new distance is less then the old, then we claim this point.
    /// If the node did not had an owner, we still claim it
    if(radius < node->radius || !node->owner)
    {
        node->owner = player + 1;
        node->radius = radius;
    }
}

TerritoryRegion::TRNode* TerritoryRegion::TryGetNode(const MapPoint& pt)
{
    return TryGetNode(GetPosFromMapPos(pt));
}

TerritoryRegion::TRNode* TerritoryRegion::TryGetNode(Position realPt)
{
    if(!AdjustCoords(realPt))
        return nullptr;

    return &GetNode(realPt);
}

const TerritoryRegion::TRNode* TerritoryRegion::TryGetNode(Position realPt) const
{
    if(!AdjustCoords(realPt))
        return nullptr;

    return &GetNode(realPt);
}

bool TerritoryRegion::AdjustCoords(Position& pt) const
{
    // The region might wrap around world boundaries. So we have to adjust the point so it will still be inside this region even if it is on
    // "the other side" of the world wrap Note: Only 1 time wrapping around is allowed which is ensured by the assertion, that this size is
    // at most the world size

    // Check if this point is inside this region
    // Apply wrap-around if on either side
    if(pt.x < 0)
        pt.x += world.GetWidth();
    else if(static_cast<unsigned>(pt.x) >= size.x)
        pt.x -= world.GetWidth();
    // Check the (possibly) adjusted point
    if(pt.x < 0 || static_cast<unsigned>(pt.x) >= size.x)
        return false;

    // Apply wrap-around if on either side
    if(pt.y < 0)
        pt.y += world.GetHeight();
    else if(static_cast<unsigned>(pt.y) >= size.y)
        pt.y -= world.GetHeight();
    // Check the (possibly) adjusted point
    if(pt.y < 0 || static_cast<unsigned>(pt.y) >= size.y)
        return false;
    return true;
}

namespace {
struct GetMapPointWithRadius
{
    using result_type = std::pair<MapPoint, unsigned>;

    result_type operator()(const MapPoint pt, unsigned r) { return std::make_pair(pt, r); }
};
} // namespace

void TerritoryRegion::CalcTerritoryOfBuilding(const noBaseBuilding& building)
{
    unsigned radius = building.GetMilitaryRadius();
    // Does not hold territory? -> Out
    if(radius == 0u)
        return;
    // Also ignore non-occupied military buildings
    if(building.GetGOT() == GOT_NOB_MILITARY && static_cast<const nobMilitary&>(building).IsNewBuilt())
        return;

    const std::vector<MapPoint>* allowedArea = &world.GetPlayer(building.GetPlayer()).GetRestrictedArea();
    if(allowedArea->empty())
        allowedArea = nullptr;

    // Punkt, auf dem das Militärgebäude steht
    MapPoint bldPos = building.GetPos();
    AdjustNode(bldPos, building.GetPlayer(), 0, nullptr); // no need to check barriers here. this point is on our territory.

    std::vector<GetMapPointWithRadius::result_type> pts = world.GetPointsInRadius(bldPos, radius, GetMapPointWithRadius());
    for(const auto& ptWithRadius : pts)
        AdjustNode(ptWithRadius.first, building.GetPlayer(), ptWithRadius.second, allowedArea);
}

uint8_t TerritoryRegion::SafeGetOwner(const Position& pt) const
{
    const TRNode* node = TryGetNode(pt);
    if(!node)
        return world.GetNode(world.MakeMapPoint(pt + startPt)).owner;
    return node->owner;
}

bool TerritoryRegion::WillBePlayerTerritory(const Position& mapPos, uint8_t owner, Direction exceptDir)
{
    for(const auto d : helpers::EnumRange<Direction>{})
    {
        if(d == exceptDir)
            continue;
        if(SafeGetOwner(::GetNeighbour(mapPos, d) - startPt) != owner)
            return false;
    }
    return true;
}
