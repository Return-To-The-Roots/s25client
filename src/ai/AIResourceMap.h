// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "ai/AIInterface.h"
#include "ai/AIResource.h"
#include <vector>
namespace AIJH {
struct Node;
}

class AIResourceMap
{
public:
    AIResourceMap() : res(AIJH::NOTHING), aii(NULL), nodes(NULL), resRadius(0) {} // Default ctor to allow storage in arrays
    AIResourceMap(const AIJH::Resource res, const AIInterface& aii, const std::vector<AIJH::Node>& nodes);
    ~AIResourceMap();

    /// Initialize the resource map
    void Init();
    void Recalc();

    /// Changes a single resource map around point pt in radius; to every point around pt distanceFromCenter * value is added
    void Change(const MapPoint pt, unsigned radius, int value);
    void Change(const MapPoint pt, int value) { Change(pt, resRadius, value); }

    /// Finds a good position for a specific resource in an area using the resource maps,
    /// first position satisfying threshold is returned, returns false if no such position found
    bool FindGoodPosition(MapPoint& pt, int threshold, BuildingQuality size, int radius = -1, bool inTerritory = true);

    /// Finds the best position for a specific resource in an area using the resource maps,
    /// satisfying the minimum value, returns false if no such position is found
    bool FindBestPosition(MapPoint& pt, BuildingQuality size, int minimum, int radius = -1, bool inTerritory = true);
    bool FindBestPosition(MapPoint& pt, BuildingQuality size, int radius = -1, bool inTerritory = true)
    {
        return FindBestPosition(pt, size, 1, radius, inTerritory);
    }

    int& operator[](const MapPoint& pt) { return map[aii->GetIdx(pt)]; }
    int operator[](const MapPoint& pt) const { return map[aii->GetIdx(pt)]; }

private:
    void AdjustRatingForBlds(BuildingType bld, unsigned radius, int value);

    std::vector<int> map;
    AIJH::Resource res; // Do not change! const ommited to to able to store this in a vector
    const AIInterface* aii;
    const std::vector<AIJH::Node>* nodes;
    unsigned resRadius; // Do not change! const ommited to to able to store this in a vector
};

#endif //! AIRESOURCEMAP_H_INCLUDED
