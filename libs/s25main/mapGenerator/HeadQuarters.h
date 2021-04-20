// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
     * Finds suitable positions for a HQ in the specified area of the map. The resulting HQ positions are sorted by
     * quality. To find suitable positions, this function does:
     * 1. look for points within a buildable area of radius 2 (min. req. of HQ)
     * 2. sort all those points by their distance to existing HQs
     * 3. filter out points within minimum distance to existing HQs
     * 4. if no remaining points just return result of 3.
     * 5. sort remaining points by how close they're to desired mountain distance
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

        // 1. look for points within a buildable area of radius 2 (min. req. of HQ)
        const auto obstacleDistance = DistancesTo(map.size, [&map](const MapPoint& pt) {
            return map.textureMap.Any(pt, [](auto t) { return !t.Is(ETerrain::Buildable); });
        });
        const auto hasEnoughSpace = [&obstacleDistance](const MapPoint& pt) { return obstacleDistance[pt] >= 2; };
        std::vector<MapPoint> possiblePositions;
        std::copy_if(area.begin(), area.end(), std::back_inserter(possiblePositions), hasEnoughSpace);

        if(possiblePositions.empty())
        {
            return possiblePositions;
        }

        // 2. sort all those points by their distance to existing HQs
        const auto distanceToOtherHqs = DistancesTo(map.hqPositions, map.size);
        const auto byDistanceToOtherHqs = [&distanceToOtherHqs](MapPoint p1, MapPoint p2) {
            return distanceToOtherHqs[p1] > distanceToOtherHqs[p2];
        };
        std::sort(possiblePositions.begin(), possiblePositions.end(), byDistanceToOtherHqs);

        // 3. filter out points within minimum distance to existing HQs
        const unsigned minHqDistance = (map.size.x + map.size.y) / 16;
        const auto farFromOtherHqs = [&distanceToOtherHqs, minHqDistance](const MapPoint& pt) {
            return distanceToOtherHqs[pt] > minHqDistance;
        };
        std::vector<MapPoint> positions;
        std::copy_if(possiblePositions.begin(), possiblePositions.end(), std::back_inserter(positions),
                     farFromOtherHqs);

        // 4. if no remaining points just return result of 3.
        if(positions.empty())
        {
            return possiblePositions;
        }

        // 5. sort remaining points by how close they're to desired mountain distance
        const auto mountain = [&map](const MapPoint& pt) { return map.textureMap.Any(pt, IsMinableMountain); };
        const auto mountainDistances = DistancesTo(map.size, mountain);
        const auto desiredDistance = static_cast<unsigned>(distance);
        const auto desiredDistanceOffset = [&mountainDistances, desiredDistance](const MapPoint& pt) {
            return mountainDistances[pt] > desiredDistance ? mountainDistances[pt] - desiredDistance :
                                                             desiredDistance - mountainDistances[pt];
        };
        const auto byDesiredMountainDistanceOffset = [desiredDistanceOffset](MapPoint p1, MapPoint p2) {
            return desiredDistanceOffset(p1) < desiredDistanceOffset(p2);
        };
        std::stable_sort(positions.begin(), positions.end(), byDesiredMountainDistanceOffset);

        return positions;
    }

    /**
     * Tries to place a head quarter (HQ) for a single player within the specified area.
     *
     * @param map reference to the map to place the HQ on
     * @param area area to place the HQ in
     * @param distance desired player distance to mountain
     *
     * @throw runtime_error
     */
    template<class T_Container>
    void PlaceHeadquarter(Map& map, const T_Container& area, MountainDistance distance)
    {
        const auto positions = FindHqPositions(map, area, distance);
        if(positions.empty())
            throw std::runtime_error("Could not find any valid HQ position!");
        FlattenForCastleBuilding(map.z, positions.front());
        map.hqPositions.push_back(positions.front());
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
