// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/objects/HeadQuarterFinder.h"

#include "randomMaps/algorithm/GridUtility.h"
#include "randomMaps/algorithm/DistanceField.h"

#include <cmath>
#include <iostream>

#define MIN_BUILDABLE_RADIUS 6

bool HeadQuarterFinder::IsBuildable(const Map* map, int index)
{
    if (!Texture::IsBuildable(map->textureRsu[index]) ||
        !Texture::IsBuildable(map->textureLsd[index]))
    {
        return true;
    }
    
    auto size = map->size;
    auto p = GridUtility::GetPosition(index, size);

    for (auto hq : map->hqPositions)
    {
        if (hq.isValid() && p.x == hq.x && p.y == hq.y)
        {
            return true;
        }
    }

    for (auto harborIndex: map->harborsRsu)
    {
        if (harborIndex == index)
        {
            return true;
        }
    }
    
    for (auto harborIndex: map->harborsLsd)
    {
        if (harborIndex == index)
        {
            return true;
        }
    }
    
    return false;
}

bool HeadQuarterFinder::IsHeadQuarter(const Map* map, int index)
{
    auto size = map->size;
    auto p = GridUtility::GetPosition(index, size);

    for (auto hq : map->hqPositions)
    {
        if (hq.isValid() && p.x == hq.x && p.y == hq.y)
        {
            return true;
        }
    }
    
    return false;
}

Position HeadQuarterFinder::FindHeadQuarterPosition(const Map& map, const std::vector<Position>& area)
{
    auto buildableArea = DistanceField(IsBuildable).Compute(&map, area);
    
    auto maximum = std::max_element(buildableArea.begin(),
                                    buildableArea.end());

    auto quality = DistanceField(IsHeadQuarter).Compute(&map, area);

    int minBuildableArea = std::min(*maximum, MIN_BUILDABLE_RADIUS);
    
    int size = area.size();
    int possibleHqPositions = 0;
    
    for (int i = 0; i < size; ++i)
    {
        if (buildableArea[i] >= minBuildableArea)
        {
            possibleHqPositions++;
        }
        else
        {
            quality[i] = 0;
        }
    }
    
    if (possibleHqPositions > 0)
    {
        auto max = std::max_element(quality.begin(), quality.end());
        auto index = std::distance(quality.begin(), max);
        
        return area[index];
    }

    return Position::Invalid();
}
