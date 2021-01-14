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
#include "mapGenerator/RandomUtility.h"
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
     *
     * @return all suitable HQ positions witihn the specified area (or the entire map if the area is empty).
     */
    template<class T_Container>
    std::vector<MapPoint> FindHqPositions(const Map& map, const T_Container& area)
    {
        if(area.empty())
        {
            throw std::runtime_error("could not find any valid HQ position");
        }

        std::vector<MapPoint> headQuarters;
        for(const MapPoint& hq : map.hqPositions)
        {
            if(hq.isValid())
            {
                headQuarters.push_back(hq);
            }
        }
        auto isObstacle = [&map](MapPoint point) {
            return map.textureMap.Any(point, [](auto t) { return !t.Is(ETerrain::Buildable); });
        };

        // Quality of a map point as one player's HQ is a mix of:
        // 1. distance to other players' HQs (higher = better)
        // 2. distance to any other obstacle e.g. mountain & water (higher = better)
        NodeMapBase<unsigned> potentialHeadQuarterQuality = DistancesTo(headQuarters, map.size);
        const auto& obstacleDistance = Distances(map.size, isObstacle);
        const auto minDistance = helpers::clamp(GetMaximum(obstacleDistance, area), 2u, 4u);

        std::vector<MapPoint> positions;
        for(const MapPoint& pt : area)
        {
            if(obstacleDistance[pt] >= minDistance)
            {
                positions.push_back(pt);
            } else
            {
                potentialHeadQuarterQuality[pt] = 0;
            }
        }
        auto isBetter = [&potentialHeadQuarterQuality](MapPoint p1, MapPoint p2) {
            return potentialHeadQuarterQuality[p1] > potentialHeadQuarterQuality[p2];
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
     *
     * @throw NoHqPositionFoundException
     */
    template<class T_Container>
    void PlaceHeadQuarter(Map& map, int index, const T_Container& area)
    {
        const auto& positions = FindHqPositions(map, area);
        if(positions.empty())
        {
            throw std::runtime_error("could not find any valid HQ position");
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
     *
     * @throw NoHqPositionFoundException
     */
    void PlaceHeadQuarters(Map& map, RandomUtility& rnd, int number, int retries = 10);

}} // namespace rttr::mapGenerator
