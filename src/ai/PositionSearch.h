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

#ifndef PositionSearch_h__
#define PositionSearch_h__

#include "ai/AIResource.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"
#include <deque>
#include <vector>

enum PositionSearchState
{
    SEARCH_IN_PROGRESS,
    SEARCH_SUCCESSFUL,
    SEARCH_FAILED
};

struct PositionSearch
{
    // where did the search start?
    MapPoint start;

    // what do we want to find?
    AIJH::Resource res;

    // and how much of that at least?
    int minimum;

    // how much space do we need?
    BuildingQuality size;

    // how many nodes should we test each cycle?
    int nodesPerStep;

    // which nodes have already been tested or will be tested next (=already in queue)?
    std::vector<bool> tested;

    // which nodes are currently queued to be tested next?
    std::queue<MapPoint> toTest;

    // results
    MapPoint result;
    int resultValue;

    // what to we want to build there?
    BuildingType bld;

    bool best;

    PositionSearch(const MapPoint pt, AIJH::Resource res, int minimum, BuildingQuality size, BuildingType bld, bool best = false)
        : start(pt), res(res), minimum(minimum), size(size), nodesPerStep(32), result(MapPoint::Invalid()), resultValue(0), bld(bld),
          best(best)
    {
    }
};

#endif // PositionSearch_h__
