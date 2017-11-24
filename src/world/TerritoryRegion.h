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

#ifndef TERRITORY_REGION_H_
#define TERRITORY_REGION_H_

#include "Point.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

/// TerritoryRegion ist ein Rechteck aus der Karte quasi "ausgeschnitten", die für die Berechnung bei Militärgebäuden-
/// aktionen (Neubau, Übernahme, Abriss) benötigt wird von RecalcTerritory

class noBaseBuilding;
class GameWorldBase;

class TerritoryRegion
{
public:
    TerritoryRegion(const Position& startPt, const Extent& size, const GameWorldBase& gwb);
    ~TerritoryRegion();

    static bool IsPointValid(const MapExtent& mapSize, const std::vector<MapPoint>& polygon, const MapPoint pt);

    /// Berechnet ein Militärgebäude mit ein
    void CalcTerritoryOfBuilding(const noBaseBuilding& building);

    unsigned GetIdx(const Position& pt) const;
    Position GetPosFromMapPos(const MapPoint& pt) const;
    /// Liefert den Besitzer eines Punktes (mit absoluten Koordinaten, werden automatisch in relative umgerechnet!)
    unsigned char GetOwner(const Position& pt) const { return GetNode(pt).owner; }
    /// Liefert Radius mit dem der Punkt besetzt wurde
    unsigned char GetRadius(const Position& pt) const { return GetNode(pt).radius; }

    /// Start position(inclusive)
    const Position startPt;
    /// Size of the region (calculated from x2-x1, y2-y1)
    const Extent size;

private:
    /// Beschreibung eines Knotenpunktes
    struct TRNode
    {
        /// Spieler-index (+1, da 0 = besitzlos!)
        unsigned char owner;
        /// Entfernung vom Militärgebäude
        unsigned char radius;

        TRNode() : owner(0), radius(0) {}
    };

    /// Check whether the point is part of the polygon
    static bool IsPointInPolygon(const std::vector<Position>& polygon, const Position& pt);
    /// Testet einen Punkt, ob der neue Spieler ihn übernehmen kann und übernimmt ihn ggf.
    void AdjustNode(MapPoint pt, unsigned char player, unsigned char radius, const std::vector<MapPoint>* allowedArea);
    TRNode& GetNode(const Position& pt) { return nodes[GetIdx(pt)]; }
    const TRNode& GetNode(const Position& pt) const { return nodes[GetIdx(pt)]; }
    /// Return a pointer to the node, if it is inside this region
    TRNode* TryGetNode(const MapPoint& pt);

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

#endif
