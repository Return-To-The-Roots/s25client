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

#include "mapGenerator/HeadQuarters.h"
#include "mapGenerator/GridUtility.h"
#include "mapGenerator/DistanceByProperty.h"
#include "mapGenerator/Algorithms.h"
#include "helpers/containerUtils.h"

#define MIN_BUILDABLE_RADIUS 6u

namespace rttr {
namespace mapGenerator {

// NEW WORLD

std::vector<MapPoint> FindHqPositions(const Map& map, const std::vector<MapPoint>& area)
{
    auto isHeadQuarter = [&map] (const MapPoint& pt) {
        return map.objectInfos[pt] == libsiedler2::OI_HeadquarterMask;
    };

    auto isObstacle = [&map] (MapPoint point) {
        return map.AnyTexture(point, [] (auto t) {
            return !t.Is(ETerrain::Buildable);
        });
    };
    
    auto quality = Distances(map.size, isHeadQuarter);
    auto obstacleDistance = Distances(map.size, isObstacle);
    
    auto minDistance = std::max(std::min(obstacleDistance.GetMaximum(), 6u), 2u);
    
    std::vector<MapPoint> positions;
    
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if (obstacleDistance[pt] >= minDistance)
        {
            positions.push_back(pt);
        }
        else
        {
            quality[pt] = 0;
        }
    }
    
    if (!area.empty())
    {
        helpers::remove_if(positions, [&area] (MapPoint p) {
            return !helpers::contains(area, p);
        });
    }
    
    auto isBetter = [&quality] (MapPoint p1, MapPoint p2) {
        return quality[p1] > quality[p2];
    };
    
    std::sort(positions.begin(), positions.end(), isBetter);
    
    return positions;
}

bool PlaceHeadQuarters(Map& map, RandomUtility& rnd, int number, int retries)
{
    auto maxRetries = retries;
    auto success = false;

    while (!success && retries > 0)
    {
        success = true;
        
        for (int index = 0; index < number; index++)
        {
            auto possiblePositions = FindHqPositions(map, {});
            
            if (possiblePositions.empty())
            {
                for (int i = 0; i < number; i++)
                {
                    map.MarkAsHeadQuarter(MapPoint::Invalid(), i);
                }
                success = false;
                break;
            }
            
            auto i = retries == maxRetries ? 0 : rnd.Index(possiblePositions.size());
            auto hq = possiblePositions[i];
            
            map.MarkAsHeadQuarter(hq, index);
        }
        
        retries--;
    }
    
    return success;
}

bool PlaceHeadQuarter(Map& map, int index, const std::vector<MapPoint>& area)
{
    auto positions = FindHqPositions(map, area);
    if (positions.empty())
    {
        return false;
    }
    
    map.MarkAsHeadQuarter(positions[0], index);
    return true;
}

// OLD WORLD

Position FindHeadQuarterPosition(const Map_& map,
                                 const TextureMapping_& mapping,
                                 const std::vector<Position>& area)
{
    auto isNotBuildable = [&map, &mapping] (int index) {
        
        if (!mapping.IsBuildable(map.textureRsu[index]) ||
            !mapping.IsBuildable(map.textureLsd[index]))
        {
            return true;
        }
        
        MapPoint position = (MapPoint)GridPosition(index, map.size);

        return
            helpers::contains(map.hqPositions, position) ||
            helpers::contains(map.harborsRsu, index) ||
            helpers::contains(map.harborsLsd, index);
    };
    
    auto isHeadQuarter = [&map] (int index) { return IsHeadQuarter(map, index); };

    auto obstacleDistance = DistanceByProperty(area, map.size, isNotBuildable);
    auto maximumDistance = std::max_element(obstacleDistance.begin(), obstacleDistance.end());
    auto quality = DistanceByProperty(area, map.size, isHeadQuarter);

    const int minimumBuildableArea = std::min(*maximumDistance, (int)MIN_BUILDABLE_RADIUS);
    
    int size = area.size();
    int possibleHqPositions = 0;
    
    for (int i = 0; i < size; ++i)
    {
        if (obstacleDistance[i] >= minimumBuildableArea)
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
        auto maximumQuality = std::max_element(quality.begin(), quality.end());
        auto index = std::distance(quality.begin(), maximumQuality);
        
        return area[index];
    }

    return Position::Invalid();
}

void ResetHeadQuarters(Map_& map)
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

bool PlaceHeadQuarters(Map_& map,
                       TextureMapping_& mapping,
                       RandomUtility& rnd,
                       int number,
                       int retries)
{
    auto remainingTries = retries;
    auto area = GridPositions(map.size);
    
    rnd.Shuffle(area);
    map.hqPositions = std::vector<MapPoint>(number);
    
    for (int i = 0; i < number; i++)
    {
        bool hqPlaced = PlaceHeadQuarter(map, mapping, i, area);
        
        while (!hqPlaced && remainingTries > 0)
        {
            ResetHeadQuarters(map);
            remainingTries--;
            
            rnd.Shuffle(area);
            hqPlaced = PlaceHeadQuarter(map, mapping, i, area);
        }
        
        if (!hqPlaced)
        {
            return false;
        }
    }
    
    return true;
}

bool PlaceHeadQuarter(Map_& map,
                      TextureMapping_& mapping,
                      int index,
                      const std::vector<Position>& area)
{
    auto position = FindHeadQuarterPosition(map, mapping, area);
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

}}
