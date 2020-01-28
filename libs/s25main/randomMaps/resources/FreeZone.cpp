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
#include "randomMaps/resources/FreeZone.h"
#include "randomMaps/algorithm/DistanceField.h"
#include "randomMaps/algorithm/GridUtility.h"

bool FreeZone::HeadQuarterOrHarbor(const Map* map, int index)
{
    for (auto harborIndex: map->harborsLsd)
    {
        if (index == harborIndex)
        {
            return true;
        }
    }

    for (auto harborIndex: map->harborsRsu)
    {
        if (index == harborIndex)
        {
            return true;
        }
    }

    auto position = GridUtility::GetPosition(index, map->size);

    for (auto hq : map->hqPositions)
    {
        if (hq.x == position.x && hq.y == position.y)
        {
            return true;
        }
    }
    
    return false;
}

std::vector<int> FreeZone::ComputeDistance(Map& map)
{
    return DistanceField(HeadQuarterOrHarbor).Compute(&map);
}
