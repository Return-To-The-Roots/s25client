// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    /// TNodeChecker must implement: bool IsNodeOk(MapPoint pt, unsigned char dirFromPrevPt) and bool
    /// IsNodeToDestOk(MapPoint pt, unsigned char dirFromPrevPt)
    template<class TNodeChecker>
    bool FindPath(MapPoint start, MapPoint dest, bool randomRoute, unsigned maxLength, std::vector<Direction>* route,
                  unsigned* length, Direction* firstDir, const TNodeChecker& nodeChecker);

    bool FindPathAlternatingConditions(MapPoint start, MapPoint dest, bool randomRoute, unsigned maxLength,
                                       std::vector<Direction>* route, unsigned* length, Direction* firstDir,
                                       FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeOKAlternate,
                                       FP_Node_OK_Callback IsNodeToDestOk, const void* param);

    /// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
    template<class TNodeChecker>
    bool CheckRoute(MapPoint start, const std::vector<Direction>& route, unsigned pos, const TNodeChecker& nodeChecker,
                    MapPoint* dest) const;

private:
    void IncreaseCurrentVisit();
};
