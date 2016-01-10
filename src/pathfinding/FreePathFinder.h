// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef FreePathFinder_h__
#define FreePathFinder_h__

#include "gameTypes/MapTypes.h"
#include <vector>

class GameWorldBase;

typedef bool (*FP_Node_OK_Callback)(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param);

// There are 2 callback types:
// IsNodeToDestOk: Called for every point to check if this node is usable
// IsNodeOk: Additionally called for every point but the destination

class FreePathFinder
{
    GameWorldBase& gwb_;
    unsigned currentVisit;
    unsigned width_, height_;
public:
    FreePathFinder(GameWorldBase& gwb): gwb_(gwb), currentVisit(0) {}
    void Init(const unsigned mapWidth, const unsigned mapHeight);

    /// Wegfindung in freiem Terrain - Basisroutine
    bool FindPath(const MapPoint start, const MapPoint dest,
        const bool randomRoute, const unsigned maxLength,
        std::vector<unsigned char> * route, unsigned* length, unsigned char* firstDir,
        FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk, const void* param,
        const bool record);

    /// Wegfindung in freiem Terrain - Template version. Users need to include FreePathFinderImpl.h
    /// TNodeChecker must implement: bool IsNodeOk(MapPoint pt, unsigned char dirFromPrevPt) and bool IsNodeToDestOk(MapPoint pt, unsigned char dirFromPrevPt)
    template<class TNodeChecker>
    bool FindPath(const MapPoint start, const MapPoint dest,
        const bool randomRoute, const unsigned maxLength,
        std::vector<unsigned char> * route, unsigned* length, unsigned char* firstDir,
        const TNodeChecker& nodeChecker,
        const bool record);

    bool FindPathAlternatingConditions(const MapPoint start, const MapPoint dest,
        const bool randomRoute, const unsigned maxLength,
        std::vector<unsigned char> * route, unsigned* length, unsigned char* firstDir,
        FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeOKAlternate, FP_Node_OK_Callback IsNodeToDestOk, const void* param,
        const bool record);

    /// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
    template<class TNodeChecker>
    bool CheckRoute(const MapPoint start, const std::vector<unsigned char>& route, const unsigned pos, const TNodeChecker& nodeChecker, MapPoint* dest) const;

private:
    void IncreaseCurrentVisit();
};

#endif // FreePathFinder_h__
