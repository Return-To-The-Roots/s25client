// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#include "AIMap.h"
#include "ai/AIResource.h"
#include "world/NodeMapBase.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"

class AIInterface;
namespace AIJH {

class AIResourceMap
{
public:
    AIResourceMap(AIResource res, bool isInfinite, const AIInterface& aii, const AIMap& aiMap);
    ~AIResourceMap();

    /// Initialize the resource map
    void init();

    void updateAround(const MapPoint& pt, int radius);

    /// Finds the best position for a specific resource in an area using the resource maps,
    /// satisfying the minimum value, returns false if no such position is found
    MapPoint findBestPosition(const MapPoint& pt, BuildingQuality size, unsigned radius, int minimum) const;

    /// Marks a position to be avoided.
    /// Only has an effect on diminishable resources where this blocks this point forever
    void avoidPosition(const MapPoint& pt);

    int operator[](const MapPoint& pt) const { return map[pt]; }

private:
    /// Update algorithm for resources which cannot be regrown
    void updateAroundDiminishable(const MapPoint& pt, int radius);
    /// Update algorithm for resources which can be replenished
    void updateAroundReplinishable(const MapPoint& pt, int radius);

    /// Which resource is stored in the map and radius of affected nodes
    const AIResource res;
    const bool isInfinite;
    const bool isDiminishableResource;
    const unsigned resRadius;

    NodeMapBase<int> map;
    const AIInterface& aii;
    const AIMap& aiMap;
};

} // namespace AIJH
