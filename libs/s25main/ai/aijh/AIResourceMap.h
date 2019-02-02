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
#ifndef AIRESOURCEMAP_H_INCLUDED
#define AIRESOURCEMAP_H_INCLUDED

#pragma once

#include "ai/AIResource.h"
#include "world/NodeMapBase.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"
#include <vector>

class AIInterface;
namespace AIJH {
class AIMap;

class AIResourceMap
{
public:
    AIResourceMap(const AIResource res, const AIInterface& aii, const AIMap& aiMap);
    ~AIResourceMap();

    /// Initialize the resource map
    void Init();
    void Recalc();
    /// Changes every point around pt in radius; to every point around pt distanceFromCenter * value is added
    void Change(const MapPoint pt, unsigned radius, int value);
    void Change(const MapPoint pt, int value) { Change(pt, resRadius, value); }
    /// Finds a good position for a specific resource in an area using the resource maps,
    /// first position satisfying threshold is returned, returns false if no such position found
    MapPoint FindGoodPosition(const MapPoint& pt, int threshold, BuildingQuality size, int radius = -1, bool inTerritory = true) const;
    /// Finds the best position for a specific resource in an area using the resource maps,
    /// satisfying the minimum value, returns false if no such position is found
    MapPoint FindBestPosition(const MapPoint& pt, BuildingQuality size, int minimum, int radius = -1, bool inTerritory = true) const;
    MapPoint FindBestPosition(const MapPoint& pt, BuildingQuality size, int radius = -1, bool inTerritory = true) const
    {
        return FindBestPosition(pt, size, 1, radius, inTerritory);
    }

    int& operator[](const MapPoint& pt) { return map[pt]; }
    int operator[](const MapPoint& pt) const { return map[pt]; }

private:
    void AdjustRatingForBlds(BuildingType bld, unsigned radius, int value);
    /// Which resource is stored in the map and radius of affected nodes
    const AIResource res;
    const unsigned resRadius;

    NodeMapBase<int> map;
    const AIInterface& aii;
    const AIMap& aiMap;
};

} // namespace AIJH

#endif //! AIRESOURCEMAP_H_INCLUDED
