// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/Islands.h"
#include "mapGenerator/TextureHelper.h"
#include <cmath>

namespace rttr { namespace mapGenerator {

    Island CreateIsland(Map& map, RandomUtility& rnd, unsigned distanceToLand, unsigned size, unsigned radius,
                        double mountainCoverage)
    {
        Island island;

        const auto isLand = [&map](const MapPoint& pt) { return map.z[pt] > map.height.minimum; };
        const auto distances = DistancesTo(map.size, isLand);
        const auto center = GetMaximumPoint(distances);

        auto compare = [&distances, &center](const MapPoint& rhs, const MapPoint& lhs) {
            // computes prefered extension points for the island by considering distance to
            // center of the island and maximizing distance to other land

            const auto ratioRhs = distances[rhs] - distances.CalcDistance(center, rhs);
            const auto ratioLhs = distances[lhs] - distances.CalcDistance(center, lhs);

            return ratioRhs < ratioLhs;
        };

        std::priority_queue<MapPoint, std::vector<MapPoint>, decltype(compare)> queue(compare);

        const unsigned minimumDistance = distanceToLand + radius;

        queue.push(center);
        island.insert(center);

        while(!queue.empty() && island.size() < size)
        {
            const MapPoint& currentPoint = queue.top();

            queue.pop();

            const auto& points = map.z.GetPointsInRadius(currentPoint, radius);

            for(const MapPoint& pt : points)
            {
                if(distances[pt] >= minimumDistance)
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
