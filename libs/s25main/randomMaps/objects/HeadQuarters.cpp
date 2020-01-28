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
#include "randomMaps/objects/HeadQuarters.h"
#include "randomMaps/objects/HeadQuarterFinder.h"
#include "randomMaps/algorithm/GridUtility.h"
#include "randomMaps/algorithm/Filter.h"

void HeadQuarters::Reset(Map& map)
{
    for (auto hq : map.hqPositions)
    {
        if (hq.isValid())
        {
            map.objectType[hq.x + hq.y * map.size.x] = 0x0;
            map.objectInfo[hq.x + hq.y * map.size.x] = 0x0;
        }
    }
}

bool HeadQuarters::Place(Map& map, int number, int remainingTries)
{
    if (remainingTries == 0)
    {
        return false;
    }
    
    auto area = GridUtility::Positions(map.size);
    
    rnd_.Shuffle(area);
    
    map.hqPositions = std::vector<MapPoint>(number);
    
    for (int i = 0; i < number; i++)
    {
        if (!Place(map, i, area))
        {
            Reset(map);
            return Place(map, number, remainingTries - 1);
        }
    }
    
    return true;
}

bool HeadQuarters::Place(Map& map, int index, const std::vector<Position>& area)
{
    auto position = HeadQuarterFinder::FindHeadQuarterPosition(map, area);
    if (!position.isValid())
    {
        return false;
    }
    
    auto hq = MapPoint(position);
    
    map.objectType[hq.x + hq.y * map.size.x] = index;
    map.objectInfo[hq.x + hq.y * map.size.x] = libsiedler2::OI_HeadquarterMask;
    map.hqPositions[index] = hq;
    
    return true;
}
