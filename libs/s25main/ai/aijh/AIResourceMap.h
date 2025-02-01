// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    unsigned calcResources() const;
    unsigned calcResources(const MapPoint& pt, unsigned radius) const;

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
