// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation,  either version 2 of the License,  or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not,  see <http://www.gnu.org/licenses/>.

#pragma once

#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class GameWorldBase;

using FP_Node_OK_Callback = bool (*)(const GameWorldBase&, const MapPoint, const Direction, const void*);

// There are 2 callback types:
// IsNodeToDestOk: Called for every point to check if this node is usable
// IsNodeOk: Additionally called for every point but the destination

class FreePathFinder
{
    GameWorldBase& gwb_;
    unsigned currentVisit;
    Extent size_;

public:
    FreePathFinder(GameWorldBase& gwb) : gwb_(gwb), currentVisit(0), size_(0, 0) {}
    void Init(const MapExtent& mapSize);

    /// Wegfindung in freiem Terrain - Template version. Users need to include FreePathFinderImpl.h
    /// TNodeChecker must implement: bool IsNodeOk(MapPoint pt, unsigned char dirFromPrevPt) and bool IsNodeToDestOk(MapPoint pt, unsigned
    /// char dirFromPrevPt)
    template<class TNodeChecker>
    bool FindPath(MapPoint start, MapPoint dest, bool randomRoute, unsigned maxLength, std::vector<Direction>* route, unsigned* length,
                  Direction* firstDir, const TNodeChecker& nodeChecker);

    bool FindPathAlternatingConditions(MapPoint start, MapPoint dest, bool randomRoute, unsigned maxLength, std::vector<Direction>* route,
                                       unsigned* length, Direction* firstDir, FP_Node_OK_Callback IsNodeOK,
                                       FP_Node_OK_Callback IsNodeOKAlternate, FP_Node_OK_Callback IsNodeToDestOk, const void* param);

    /// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
    template<class TNodeChecker>
    bool CheckRoute(MapPoint start, const std::vector<Direction>& route, unsigned pos, const TNodeChecker& nodeChecker,
                    MapPoint* dest) const;

private:
    void IncreaseCurrentVisit();
};
