// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/TerritoryRegion.h"
#include "GamePlayer.h"
#include "MapGeometry.h"
#include "ReturnMapPointWithRadius.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/nobMilitary.h"
#include "helpers/EnumRange.h"
#include "world/GameWorldBase.h"
#include <stdexcept>

TerritoryRegion::TerritoryRegion(const Position& startPt, const Extent& size, const GameWorldBase& gwb)
    : startPt(startPt), size(size), world(gwb)
{
    if(size.x > gwb.getWidth() || size.y > gwb.getHeight())
        throw std::runtime_error("Size to big for TerritoryRegion");
    // Feld erzeugen
    nodes.resize(size.x * size.y);
}

TerritoryRegion::~TerritoryRegion() = default;

bool TerritoryRegion::isPointInPolygon(const std::vector<Position>& polygon, const Position& pt)
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

bool TerritoryRegion::isPointValid(const MapExtent& mapSize, const std::vector<MapPoint>& polygon, const MapPoint pt)
{
    // This is for specifying polyons that wrap around corners:
    // - e.g. w=64, h=64, polygon = {(40,40), (40,80), (80,80), (80,40)}
    Position pt2(pt.x + mapSize.x, pt.y), pt3(pt.x, pt.y + mapSize.y), pt4(pt + mapSize);
    const std::vector<Position> polygonInt(polygon.begin(), polygon.end());
    return (polygon.empty() || isPointInPolygon(polygonInt, Position(pt)) || isPointInPolygon(polygonInt, pt2)
            || isPointInPolygon(polygonInt, pt3) || isPointInPolygon(polygonInt, pt4));
}

void TerritoryRegion::AdjustNode(MapPoint pt, uint8_t player, uint16_t radius, const std::vector<MapPoint>* allowedArea)
{
    TRNode* node = TryGetNode(pt);
    // Not in our region -> Out
    if(!node)
        return;

    // check whether this node is within the area we may have territory in
    if(allowedArea && !isPointValid(world.getSize(), *allowedArea, pt))
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
    return TryGetNode(getPosFromMapPos(pt));
}

TerritoryRegion::TRNode* TerritoryRegion::TryGetNode(Position realPt)
{
    if(!AdjustCoords(realPt))
        return nullptr;

    return &getNode(realPt);
}

const TerritoryRegion::TRNode* TerritoryRegion::TryGetNode(Position realPt) const
{
    if(!AdjustCoords(realPt))
        return nullptr;

    return &getNode(realPt);
}

bool TerritoryRegion::AdjustCoords(Position& pt) const
{
    // The region might wrap around world boundaries. So we have to adjust the point so it will still be inside this
    // region even if it is on "the other side" of the world wrap Note: Only 1 time wrapping around is allowed which is
    // ensured by the assertion, that this size is at most the world size

    // Check if this point is inside this region
    // Apply wrap-around if on either side
    if(pt.x < 0)
        pt.x += world.getWidth();
    else if(static_cast<unsigned>(pt.x) >= size.x)
        pt.x -= world.getWidth();
    // Check the (possibly) adjusted point
    if(pt.x < 0 || static_cast<unsigned>(pt.x) >= size.x)
        return false;

    // Apply wrap-around if on either side
    if(pt.y < 0)
        pt.y += world.getHeight();
    else if(static_cast<unsigned>(pt.y) >= size.y)
        pt.y -= world.getHeight();
    // Check the (possibly) adjusted point
    if(pt.y < 0 || static_cast<unsigned>(pt.y) >= size.y)
        return false;
    return true;
}

void TerritoryRegion::CalcTerritoryOfBuilding(const noBaseBuilding& building)
{
    unsigned radius = building.getMilitaryRadius();
    // Does not hold territory? -> Out
    if(radius == 0u)
        return;
    // Also ignore non-occupied military buildings
    if(building.getGOT() == GO_Type::NobMilitary && static_cast<const nobMilitary&>(building).isNewBuilt())
        return;

    const std::vector<MapPoint>* allowedArea = &world.getPlayer(building.getPlayer()).getRestrictedArea();
    if(allowedArea->empty())
        allowedArea = nullptr;

    // Punkt, auf dem das Militärgebäude steht
    MapPoint bldPos = building.getPos();
    AdjustNode(bldPos, building.getPlayer(), 0,
               nullptr); // no need to check barriers here. this point is on our territory.

    const auto pts = world.getPointsInRadius(bldPos, radius, ReturnMapPointWithRadius{});
    for(const auto& ptWithRadius : pts)
        AdjustNode(ptWithRadius.first, building.getPlayer(), ptWithRadius.second, allowedArea);
}

uint8_t TerritoryRegion::SafeGetOwner(const Position& pt) const
{
    const TRNode* node = TryGetNode(pt);
    if(!node)
        return world.getNode(world.MakeMapPoint(pt + startPt)).owner;
    return node->owner;
}

bool TerritoryRegion::WillBePlayerTerritory(const Position& mapPos, uint8_t owner, Direction exceptDir)
{
    for(const auto d : helpers::EnumRange<Direction>{})
    {
        if(d == exceptDir)
            continue;
        if(SafeGetOwner(::getNeighbour(mapPos, d) - startPt) != owner)
            return false;
    }
    return true;
}
