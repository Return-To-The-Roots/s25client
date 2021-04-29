// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Islands.h"
#include "mapGenerator/TextureHelper.h"
#include <cmath>

namespace rttr { namespace mapGenerator {

    Island CreateIsland(Map& map, RandomUtility& rnd, unsigned size, unsigned minLandDist, double mountainCoverage)
    {
        Island island;

        const auto land = [&map](const MapPoint& pt) { return map.z[pt] > map.height.minimum; };
        const auto distances = DistancesTo(map.size, land);
        const unsigned maxDistance = *std::max_element(distances.begin(), distances.end());
        const unsigned minDistance = std::min(maxDistance, minLandDist * 10);
        const auto possibleCenters = SelectPoints(
          [&distances, minDistance](const MapPoint& pt) { return distances[pt] >= minDistance; }, distances.GetSize());
        const MapPoint center = rnd.RandomItem(possibleCenters);

        const auto compare = [&distances, &center](const MapPoint& rhs, const MapPoint& lhs) {
            // computes prefered extension points for the island by considering distance to
            // center of the island and maximizing distance to other land

            const auto ratioRhs = distances[rhs] - distances.CalcDistance(center, rhs);
            const auto ratioLhs = distances[lhs] - distances.CalcDistance(center, lhs);

            return ratioRhs < ratioLhs;
        };

        std::priority_queue<MapPoint, std::vector<MapPoint>, decltype(compare)> queue(compare);

        queue.push(center);
        island.insert(center);

        while(!queue.empty() && island.size() < size)
        {
            const MapPoint& currentPoint = queue.top();

            queue.pop();

            const auto& points = map.z.GetPointsInRadius(currentPoint, minLandDist);

            for(const MapPoint& pt : points)
            {
                if(distances[pt] >= 2 * minLandDist)
                {
                    if(island.insert(pt).second)
                    {
                        queue.push(pt);
                    }
                }
            }
        }

        auto isCoast = [&map, &island](const MapPoint& pt) {
            return helpers::contains_if(map.z.GetNeighbours(pt),
                                        [&island](const MapPoint& pt) { return !helpers::contains(island, pt); });
        };

        const auto& coastDistance = Distances(map.size, island, 0, isCoast);

        for(const MapPoint& pt : island)
        {
            const auto base = map.height.minimum + coastDistance[pt];
            const auto minimum = rnd.RandomValue(base, base + 1u);
            map.z[pt] = std::min(static_cast<uint8_t>(minimum), map.height.maximum);
        }

        auto mountain = map.textureMap.FindAll(IsMinableMountain);
        auto mountainLevel = LimitFor(coastDistance, island, 1. - mountainCoverage, map.height.minimum + 1u);
        auto mountainRange = ValueRange<unsigned>(mountainLevel, GetMaximum(coastDistance, island));

        auto textures = map.textureMap.FindAll(IsBuildableLand);
        auto textureRange = ValueRange<unsigned>(0u, mountainLevel);

        map.textureMap.Sort(textures, ByHumidity);

        for(const MapPoint& pt : island)
        {
            if(coastDistance[pt] < mountainLevel)
            {
                map.textureMap.Set(pt, textures[MapValueToIndex(coastDistance[pt], textureRange, textures.size())]);
            } else
            {
                map.textureMap.Set(pt, mountain[MapValueToIndex(coastDistance[pt], mountainRange, mountain.size())]);
            }
        }

        return island;
    }

}} // namespace rttr::mapGenerator
