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

#include "ai/AIResource.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"
#include <queue>
#include <vector>

namespace AIJH {

class AIPlayerJH;

enum PositionSearchState
{
    SEARCH_IN_PROGRESS,
    SEARCH_SUCCESSFUL,
    SEARCH_FAILED
};

class PositionSearch
{
public:
    PositionSearch(const AIPlayerJH& player, MapPoint pt, AIResource res, int minimum, BuildingType bld,
                   bool searchGlobalOptimum = false);

    PositionSearchState execute(const AIPlayerJH& player);
    BuildingType GetBld() const { return bld; }
    MapPoint GetResultPt() const { return resultPt; }

private:
    /// where did the search start?
    MapPoint startPt;
    /// what do we want to find?
    AIResource res;
    /// and how much of that at least?
    int minimum;
    /// how much space do we need?
    BuildingQuality size;
    /// what to we want to build there?
    BuildingType bld;
    /// If false, the first point matching the conditions will be returned. Otherwise it looks further for even better
    /// points
    bool searchGlobalOptimum;
    /// how many nodes should we test each cycle?
    int nodesPerStep;
    /// which nodes have already been tested or will be tested next (=already in queue)?
    std::vector<bool> tested;
    /// which nodes are currently queued to be tested next?
    std::queue<MapPoint> toTest;
    /// results
    MapPoint resultPt;
    int resultValue;
};

} // namespace AIJH
