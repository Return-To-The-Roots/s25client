// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "RTTR_Assert.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class noBaseBuilding;
class GameWorldBase;

/// TerritoryRegion describes a rectangular region used for the calculation of the territory of military buildings
/// e.g. after build, capture or destruction
/// Important: Positions are relative to the startPt -> (0,0) == startPt.
/// Those can NOT be used for GetNeighbour etc. as startPt might be odd which would lead to wrong results in the
/// GetNeighbour calculations
class TerritoryRegion
{
public:
    TerritoryRegion(const Position& startPt, const Extent& size, const GameWorldBase& gwb);
    ~TerritoryRegion();

    static bool IsPointValid(const MapExtent& mapSize, const std::vector<MapPoint>& polygon, MapPoint pt);

    /// Adds the territory of the building
    void CalcTerritoryOfBuilding(const noBaseBuilding& building);

    unsigned GetIdx(const Position& pt) const;
    Position GetPosFromMapPos(const MapPoint& pt) const;
    /// Return the owner of the point from this region (pt is relative to startPt)
    uint8_t GetOwner(const Position& pt) const { return GetNode(pt).owner; }
    /// Return the owner. If the point is outside the region return owner from map
    uint8_t SafeGetOwner(const Position& pt) const;
    void SetOwner(const Position& pt, uint8_t owner) { GetNode(pt).owner = owner; }
    /// Return true, if all points surrounding the given point (relative to map origin!!!) have the same owner
    /// Direction exceptDir will not be checked
    bool WillBePlayerTerritory(const Position& mapPos, uint8_t owner, Direction exceptDir);

    /// Start position (inclusive)
    const Position startPt;
    /// Size of the region
    const Extent size;

private:
    /// A node in the region
    struct TRNode
    {
        /// Owner (= player index + 1, 0 = no owner)
        uint8_t owner;
        /// Distance to next military bld
        uint16_t radius;

        TRNode() : owner(0), radius(0) {}
    };

    /// Check whether the point is part of the polygon
    static bool IsPointInPolygon(const std::vector<Position>& polygon, const Position& pt);
    /// Check and set if a point belongs to the player, when a military bld in the given radius is added
    void AdjustNode(MapPoint pt, uint8_t player, uint16_t radius, const std::vector<MapPoint>* allowedArea);
    TRNode& GetNode(const Position& pt) { return nodes[GetIdx(pt)]; }
    const TRNode& GetNode(const Position& pt) const { return nodes[GetIdx(pt)]; }
    /// Return a pointer to the node, if it is inside this region
    TRNode* TryGetNode(const MapPoint& pt);
    TRNode* TryGetNode(Position realPt);
    const TRNode* TryGetNode(Position realPt) const;
    bool AdjustCoords(Position& pt) const;

    const GameWorldBase& world;
    std::vector<TRNode> nodes;
};

inline unsigned TerritoryRegion::GetIdx(const Position& pt) const
{
    RTTR_Assert(pt.x >= 0 && static_cast<unsigned>(pt.x) < size.x);
    RTTR_Assert(pt.y >= 0 && static_cast<unsigned>(pt.y) < size.y);
    return pt.y * size.x + pt.x;
}

inline Position TerritoryRegion::GetPosFromMapPos(const MapPoint& pt) const
{
    return pt - startPt;
}
