// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Harbors.h"
#include "commonDefines.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/Islands.h"
#include "mapGenerator/TextureHelper.h"

namespace rttr::mapGenerator {

void placeHarborPosition(Map& map, const MapPoint& position)
{
    flattenForCastleBuilding(map.z, position);

    auto& textures = map.textureMap;

    auto harborTexture = textures.find(isBuildableCoast);
    auto triangles = getTriangles(position, map.size);

    auto isWaterTerrain = [&textures](const MapPoint& pt) { return textures.any(pt, isWater); };

    for(const Triangle& triangle : triangles)
    {
        textures.set(triangle, harborTexture);
    }

    for(const Triangle& triangle : triangles)
    {
        if(helpers::containsIf(getTriangleEdges(triangle, map.size), isWaterTerrain))
        {
            map.harbors.push_back(triangle);
        }
    }
}

std::vector<std::vector<MapPoint>> findCoastlines(const Map& map, const std::vector<River>& rivers)
{
    std::vector<std::vector<MapPoint>> coasts;
    std::set<MapPoint, MapPointLess> visited;

    const auto isCoast = [&map](const MapPoint& pt) {
        // Swamps may consist of water textures but can't be used by ships
        const auto allWater = [&map](const MapPoint& p) {
            return map.textureMap.all(p, isWater) && !helpers::contains(map.barren, p);
        };
        return map.textureMap.any(pt, isLand) && map.textureMap.any(pt, isWater)
               && helpers::containsIf(map.getTextures().getNeighbours(pt), allWater);
    };

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(visited.insert(pt).second)
        {
            if(isCoast(pt))
            {
                const auto coast = collect(map.getTextures(), pt, isCoast);
                coasts.push_back(coast);
                visited.insert(coast.begin(), coast.end());
            }
        }
    }

    const auto distanceToRiver = distancesTo(join(rivers), map.size);
    const auto closeToRiver = [&distanceToRiver](const MapPoint& pt) { return distanceToRiver[pt] < 5; };

    for(std::vector<MapPoint>& coast : coasts)
    {
        helpers::eraseIf(coast, closeToRiver);
    }

    return coasts;
}

void placeHarbors(Map& map, const std::vector<River>& rivers, int coastSize, int nodesPerHarbor)
{
    const auto coasts = findCoastlines(map, rivers);
    std::vector<MapPoint> harbors;
    for(const std::vector<MapPoint>& coast : coasts)
    {
        if(coast.size() < static_cast<unsigned>(coastSize))
        {
            continue;
        }

        int numberOfHarborsForCoast = std::max(static_cast<int>(coast.size() / nodesPerHarbor), 1);
        int maxDistance = map.size.x * map.size.y;

        const auto distanceToHarbors = [&map, &harbors, maxDistance](const MapPoint& pt) {
            const auto calcDistance = [pt, &map](const MapPoint& hp1, const MapPoint& hp2) {
                return map.getTextures().calcDistance(hp1, pt) < map.getTextures().calcDistance(hp2, pt);
            };
            const auto closestHarbor = std::min_element(harbors.begin(), harbors.end(), calcDistance);
            return closestHarbor == harbors.end() ? maxDistance : map.getTextures().calcDistance(pt, *closestHarbor);
        };
        const auto compareHarbors = [&distanceToHarbors](const MapPoint& p1, const MapPoint& p2) {
            return distanceToHarbors(p1) < distanceToHarbors(p2);
        };

        for(int i = 0; i < numberOfHarborsForCoast; i++)
        {
            auto harbor = std::max_element(coast.begin(), coast.end(), compareHarbors);
            if(harbor != coast.end())
            {
                harbors.push_back(*harbor);
                placeHarborPosition(map, *harbor);
            }
        }
    }
}

} // namespace rttr::mapGenerator
