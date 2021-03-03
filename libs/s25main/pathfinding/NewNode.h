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

#pragma once

#include "pathfinding/OpenListBinaryHeap.h"
#include "pathfinding/PathfindingPoint.h"
#include <set>

/// Konstante für einen ungültigen Vorgängerknoten
const unsigned INVALID_PREV = 0xFFFFFFFF;

/// Class for a node used by the free pathfinding
struct NewNode
{
    /// Wegkosten, die vom Startpunkt bis zu diesem Knoten bestehen
    unsigned way = 0;
    unsigned wayEven = 0;
    /// Die Richtung, über die dieser Knoten erreicht wurde
    Direction dir = Direction::West;
    Direction dirEven = Direction::West;
    /// ID (gebildet aus y*Kartenbreite+x) des Vorgänngerknotens
    unsigned prev = INVALID_PREV;
    unsigned prevEven = INVALID_PREV;
    /// Iterator auf Position in der Prioritätswarteschlange (std::set), freies Pathfinding
    std::set<PathfindingPoint>::iterator it_p = {};
    /// Wurde Knoten schon besucht (für A*-Algorithmus), wenn lastVisited == currentVisit
    unsigned lastVisited = 0;
    unsigned lastVisitedEven = 0; // used for road pathfinding (for ai only for now)
    MapPoint mapPt = {};
};

struct FreePathNode : BinaryHeapPosMarker
{
    /// Indicator if node was visited (lastVisited == currentVisit)
    unsigned lastVisited;
    /// Previous node
    FreePathNode* prev;
    /// Distance from start to this node
    unsigned curDistance;
    /// Distance from node to target
    unsigned targetDistance;
    /// Distance from start over thise node to target (== curDistance + targetDistance)
    unsigned estimatedDistance;
    /// Point on map which this node represents
    MapPoint mapPt;
    /// Direction used to reach this node
    Direction dir;
};
