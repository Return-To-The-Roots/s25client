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

#ifndef TERRITORY_REGION_H_
#define TERRITORY_REGION_H_

#include "gameTypes/MapTypes.h"
#include <vector>

/// TerritoryRegion ist ein Rechteck aus der Karte quasi "ausgeschnitten", die für die Berechnung bei Militärgebäuden-
/// aktionen (Neubau, Übernahme, Abriss) benötigt wird von RecalcTerritory

class noBaseBuilding;
class GameWorldBase;

class TerritoryRegion
{
    /// Beschreibung eines Knotenpunktes
    struct TRNode
    {
        /// Spieler-index (+1, da 0 = besitzlos!)
        unsigned char owner;
        /// Entfernung vom Militärgebäude
        unsigned char radius;

        TRNode(): owner(0), radius(0){}
    };

public:
    /// Position of the region on the map
    const int x1, y1, x2, y2;
    /// Size of the region (calculated from x2-x1, y2-y1)
    const unsigned width, height;

private:
    const GameWorldBase& gwb;
    std::vector<TRNode> nodes;

    /// Check whether the point x, y is part of the polygon
    static bool IsPointInPolygon(const std::vector<MapPoint>& polygon, const MapPoint pt);
    /// Testet einen Punkt, ob der neue Spieler ihn übernehmen kann und übernimmt ihn ggf.
    void AdjustNode(MapPoint pt, const unsigned char player, const unsigned char radius, const bool check_barriers);
    TRNode& GetNode(const int x, const int y) { return nodes[GetIdx(x, y)]; }
    const TRNode& GetNode(const int x, const int y) const { return nodes[GetIdx(x, y)]; }

public:
    TerritoryRegion(const int x1, const int y1, const int x2, const int y2, const GameWorldBase& gwb);
    ~TerritoryRegion();

    static bool IsPointValid(const GameWorldBase& gwb, const std::vector<MapPoint>& polygon, const MapPoint pt);

    /// Berechnet ein Militärgebäude mit ein
    void CalcTerritoryOfBuilding(const noBaseBuilding& building);

    unsigned GetIdx(const int x, const int y) const { return (y - y1) * width + (x - x1); }
    /// Liefert den Besitzer eines Punktes (mit absoluten Koordinaten, werden automatisch in relative umgerechnet!)
    unsigned char GetOwner(const int x, const int y) const { return GetNode(x, y).owner; }
    /// Liefert Radius mit dem der Punkt besetzt wurde
    unsigned char GetRadius(const int x, const int y) const { return GetNode(x, y).radius; }
};

#endif


