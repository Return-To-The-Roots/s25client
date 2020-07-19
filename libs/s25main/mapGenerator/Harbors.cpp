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

#include "mapGenerator/Harbors.h"
#include "commonDefines.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/Islands.h"
#include "mapGenerator/TextureHelper.h"

namespace rttr { namespace mapGenerator {

    void PlaceHarborPosition(Map& map, const MapPoint& position)
    {
        auto& z = map.z;
        auto& textures = map.textures;

        const auto& neighbors = z.GetNeighbours(position);
        const auto& farNeighbors = z.GetPointsInRadius(position, 2);

        auto compareHeight = [&z](const MapPoint& p1, const MapPoint& p2) { return z[p1] < z[p2]; };

        auto lowestPoint = *std::min_element(neighbors.begin(), neighbors.end(), compareHeight);
        auto lowestHeight = std::min(z[position], z[lowestPoint]);

        for(const MapPoint& farNeighbor : farNeighbors)
        {
            z[farNeighbor] = std::min(static_cast<unsigned>(z[farNeighbor]), static_cast<unsigned>(lowestHeight + 1));
        }

        z[position] = lowestHeight;
        z.SetValues(neighbors, lowestHeight);

        auto harborTexture = textures.Find(IsBuildableCoast);
        auto triangles = GetTriangles(position, map.size);

        auto containsWater = [&textures](const Triangle& triangle) { return textures.Check(triangle, IsWater); };

        for(Triangle triangle : triangles)
        {
            textures.Set(triangle, harborTexture);

            if(helpers::contains_if(GetTriangleNeighbors(triangle, map.size), containsWater))
            {
                map.harbors.push_back(triangle);
            }
        }
    }

    void PlaceHarbors(Map& map, int minimumIslandSize, int minimumCoastSize, const std::vector<River>& rivers, int maximumIslandHarbors)
    {
        auto islands = FindIslands(map.textures, minimumIslandSize);
        const auto& textures = map.textures;

        auto isPartOfRiver = [&rivers](const MapPoint& pt) {
            return helpers::contains_if(rivers, [&pt](const River& river) { return river.find(pt) != river.end(); });
        };

        auto distanceToRiver = Distances(map.size, isPartOfRiver);

        auto isCloseToRiver = [&distanceToRiver](const MapPoint& pt) { return distanceToRiver[pt] < 5; };

        for(const auto& island : islands)
        {
            const int islandSize = island.size();

            if(islandSize < minimumIslandSize)
            {
                continue;
            }

            std::vector<MapPoint> coastland;

            for(const MapPoint pt : island)
            {
                if(textures.Any(pt, IsLand) && textures.Any(pt, IsWater))
                {
                    coastland.push_back(pt);
                }
            }

            helpers::remove_if(coastland, isCloseToRiver);

            const int coastSize = coastland.size();

            if(coastSize < minimumCoastSize)
            {
                continue;
            }

            const int harborsForCoastSize = coastSize / minimumCoastSize;
            const int harborPositions =
              std::max(1, maximumIslandHarbors > 0 ? std::min(maximumIslandHarbors, harborsForCoastSize) : harborsForCoastSize);

            PlaceHarborPosition(map, coastland[0]);
            std::vector<MapPoint> harbors{coastland[0]};

            auto isHarborPosition = [&harbors](const MapPoint& pt) { return helpers::contains(harbors, pt); };

            for(int i = 1; i < harborPositions; i++)
            {
                const auto& distances = Distances(map.size, coastland, 0, isHarborPosition);
                const auto bestPosition = distances.GetMaximumPoint();

                RTTR_Assert(distances[bestPosition] > 0);

                PlaceHarborPosition(map, bestPosition);
                harbors.push_back(bestPosition);
            }
        }
    }

}} // namespace rttr::mapGenerator
