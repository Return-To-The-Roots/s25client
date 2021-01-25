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

#include "helpers/mathFuncs.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/RandomUtility.h"
#include "mapGenerator/TextureHelper.h"
#include <stdexcept>

namespace rttr { namespace mapGenerator {

    /**
     * Find the largest connected area on the map. An area still counts as "connected" when it's divided by a tiny river
     * or small mountains. However, all nodes within a connected area should be reachable for any player. Even though
     * tiny rivers do not disconnect an area, water and mountain tiles are not considered part of the area.
     *
     * @param map reference to the map to look for the largest connected area
     *
     * @returns all map points within the largest connected area of the map.
     */
    std::vector<MapPoint> FindLargestConnectedArea(const Map& map);

    /**
     * Finds the most suitable position for a HQ in the specified area of the map. To find the most suitable position
     * for the entire map just leave the area empty. The resulting HQ positions are sorted by quality (highest quality
     * first). Good HQ positions are positions which are far away from other HQs and in a widely buildable area.
     *
     * @param map map to search for suitable HQ positions
     * @param area area within the HQ position should be
     * @param distance desired player distance to mountain
     *
     * @return all suitable HQ positions witihn the specified area (or the entire map if the area is empty).
     * @throw runtime_error
     */
    template<class T_Container>
    std::vector<MapPoint> FindHqPositions(const Map& map, const T_Container& area, MountainDistance distance)
    {
        if(area.empty())
        {
            throw std::runtime_error("Could not find any valid HQ position!");
        }

        const auto isObstacle = [&map](const MapPoint& pt) {
            return map.textureMap.Any(pt, [](auto t) { return !t.Is(ETerrain::Buildable); });
        };

        // Ensure possible HQ positions are far enough from any obstacles
        const auto obstacleDistance = DistancesTo(map.size, isObstacle);
        const auto minObstacleDistance = helpers::clamp(GetMaximum(obstacleDistance, area), 2u, 4u);
        const auto farFromObstacles = [&obstacleDistance, minObstacleDistance](const MapPoint& pt) {
            return obstacleDistance[pt] >= minObstacleDistance;
        };
        std::vector<MapPoint> possiblePositions;
        std::copy_if(area.begin(), area.end(), std::back_inserter(possiblePositions), farFromObstacles);

        if(possiblePositions.empty())
        {
            return possiblePositions;
        }

        // Ensure HQ positions keep desired distance to mountains
        const auto mountain = [&map](const MapPoint& pt) { return map.textureMap.Any(pt, IsMinableMountain); };
        const auto mountainDistance = DistancesTo(map.size, mountain);
        const auto desiredDistance =
          std::max(GetMinimum(mountainDistance, possiblePositions), static_cast<unsigned>(distance));
        const auto maxDistance = static_cast<unsigned>(map.size.x + map.size.y) / 4;

        std::vector<MapPoint> positions;
        auto allowedMountainDistance = desiredDistance;
        while(positions.empty() && allowedMountainDistance < maxDistance)
        {
            for(const MapPoint& pt : possiblePositions)
            {
                if(obstacleDistance[pt] >= minObstacleDistance && mountainDistance[pt] < allowedMountainDistance + 5
                   && mountainDistance[pt] > allowedMountainDistance - 5)
                {
                    positions.push_back(pt);
                }
            }
            allowedMountainDistance++;
        }

        if(positions.empty()) // fallback to ignore mountain distance
        {
            positions = possiblePositions;
        }

        // Sort available HQ positions by distance to other HQs (higher = better)
        std::vector<MapPoint> hqs;
        const auto isValid = [](const MapPoint& pt) { return pt.isValid(); };
        std::copy_if(map.hqPositions.begin(), map.hqPositions.end(), std::back_inserter(hqs), isValid);
        const auto distanceToOtherHqs = DistancesTo(hqs, map.size);
        const auto isBetter = [&distanceToOtherHqs](MapPoint p1, MapPoint p2) {
            return distanceToOtherHqs[p1] > distanceToOtherHqs[p2];
        };
        std::sort(positions.begin(), positions.end(), isBetter);
        return positions;
    }

    /**
     * Tries to place a head quarter (HQ) for a single player within the specified area.
     *
     * @param map reference to the map to place the HQ on
     * @param index player index for the HQ
     * @param area area to place the HQ in
     * @param distance desired player distance to mountain
     *
     * @throw runtime_error
     */
    template<class T_Container>
    void PlaceHeadquarter(Map& map, int index, const T_Container& area, MountainDistance distance)
    {
        const auto positions = FindHqPositions(map, area, distance);
        if(positions.empty())
        {
            throw std::runtime_error("Could not find any valid HQ position!");
        }
        map.hqPositions[index] = positions.front();
    }

    /**
     * Tries to place a number of headquarters on the specified map.
     *
     * @param map map to place head quarters for all players on
     * @param rnd random number generated used for retrying HQ placement on failures
     * @param number number of HQs to place - equal to the number of players
     * @param retries number of retries to place valid HQs on this map
     * @param distance desired player distance to mountain
     *
     * @throw runtime_error
     */
    void PlaceHeadquarters(Map& map, RandomUtility& rnd, int number, MountainDistance distance, int retries = 10);

}} // namespace rttr::mapGenerator
