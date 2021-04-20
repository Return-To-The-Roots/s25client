// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AIResource.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"
#include <queue>
#include <vector>

namespace AIJH {

class AIPlayerJH;

enum class PositionSearchState
{
    InProgress,
    Successfull,
    Failed
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
