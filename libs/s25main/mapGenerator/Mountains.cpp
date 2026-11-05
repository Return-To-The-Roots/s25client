// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Mountains.h"
#include "helpers/containerUtils.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/TextureHelper.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <queue>

namespace rttr::mapGenerator {

namespace {
    /// Size of each mountain relative to the previous one
    constexpr double sizeDecay = 0.9;
    /// Radius of the brush collecting nodes around a node of a growing mountain
    constexpr unsigned brushRadius = 2;
    /// Chance of the brush to collect a node - values below 100 create irregular shapes
    constexpr unsigned brushDensity = 75;

    /// Updates the distances after the specified nodes stopped being free land
    void MarkAsOccupied(NodeMapBase<unsigned>& freeLand, const Mountain& mountain)
    {
        std::queue<MapPoint> queue;
        for(const MapPoint& pt : mountain)
        {
            freeLand[pt] = 0;
            queue.push(pt);
        }

        // UpdateDistances only fills unset values so it can't be used to lower existing distances
        while(!queue.empty())
        {
            const MapPoint current = queue.front();
            queue.pop();

            const unsigned distance = freeLand[current] + 1;
            for(const MapPoint& neighbor : freeLand.GetNeighbours(current))
            {
                if(distance < freeLand[neighbor])
                {
                    freeLand[neighbor] = distance;
                    queue.push(neighbor);
                }
            }
        }
    }
} // namespace

unsigned GetMountainSize(const MountainSettings& settings, unsigned index)
{
    const auto size = static_cast<unsigned>(settings.maxSize * std::pow(sizeDecay, index));
    return std::max(size, settings.minSize);
}

NodeMapBase<unsigned> GetFreeLandDistances(const Map& map, unsigned mountainLevel)
{
    const auto& textures = map.getTextures();
    return DistancesTo(map.size, [&map, &textures, mountainLevel](const MapPoint& pt) {
        return map.z[pt] == map.height.minimum || map.z[pt] >= mountainLevel || textures[pt].rsu || textures[pt].lsd;
    });
}

Mountain GrowMountain(const NodeMapBase<unsigned>& freeLand, RandomUtility& rnd, const MapPoint& center, unsigned size,
                      unsigned margin)
{
    const auto priority = [&freeLand, &center](const MapPoint& pt) {
        return static_cast<int64_t>(freeLand[pt]) - freeLand.CalcDistance(center, pt);
    };
    const auto compare = [&priority](const MapPoint& lhs, const MapPoint& rhs) {
        return priority(lhs) < priority(rhs);
    };

    std::priority_queue<MapPoint, std::vector<MapPoint>, decltype(compare)> queue(compare);
    Mountain mountain{center};
    queue.push(center);

    while(!queue.empty() && mountain.size() < size)
    {
        const MapPoint current = queue.top();
        queue.pop();

        for(const MapPoint& pt : freeLand.GetPointsInRadius(current, brushRadius))
        {
            if(mountain.size() < size && freeLand[pt] > margin && rnd.ByChance(brushDensity)
               && mountain.insert(pt).second)
            {
                queue.push(pt);
            }
        }
    }

    return mountain;
}

std::vector<Mountain> FindMountains(const Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                    const MountainSettings& settings)
{
    std::vector<Mountain> mountains;
    auto freeLand = GetFreeLandDistances(map, mountainLevel);
    const auto maxDistance = [&freeLand] { return *std::max_element(freeLand.begin(), freeLand.end()); };

    for(unsigned distance = maxDistance(); distance > settings.minFreeLand; distance = maxDistance())
    {
        const auto isFreest = [&freeLand, distance](const MapPoint& pt) { return freeLand[pt] == distance; };
        const MapPoint center = rnd.RandomItem(SelectPoints(isFreest, map.size));

        // A larger margin than required varies the gaps. It must stay below the distance to include the center.
        const unsigned maxMargin = std::min(settings.minFreeLand + settings.minFreeLand / 2, distance - 1);
        const unsigned margin = rnd.RandomValue(settings.minFreeLand, maxMargin);
        const unsigned size = GetMountainSize(settings, static_cast<unsigned>(mountains.size()));

        mountains.push_back(GrowMountain(freeLand, rnd, center, size, margin));
        MarkAsOccupied(freeLand, mountains.back());
    }

    return mountains;
}

void RaiseMountain(Map& map, RandomUtility& rnd, const Mountain& mountain)
{
    const auto isFoot = [&map, &mountain](const MapPoint& pt) {
        return helpers::contains_if(map.z.GetNeighbours(pt),
                                    [&mountain](const MapPoint& pt) { return !helpers::contains(mountain, pt); });
    };
    const auto footDistance = Distances(map.size, mountain, 0, isFoot);
    const ValueRange<unsigned> range(0u, GetMaximum(footDistance, mountain));
    const auto textures = map.textureMap.FindAll(IsMinableMountain);

    for(const MapPoint& pt : mountain)
    {
        const unsigned elevation = rnd.RandomValue(footDistance[pt], footDistance[pt] + 1u);
        map.z[pt] = static_cast<uint8_t>(std::min<unsigned>(map.z[pt] + elevation, map.height.maximum));
        map.textureMap.Set(pt, textures[MapValueToIndex(footDistance[pt], range, textures.size())]);
    }
}

std::vector<Mountain> CreateMountains(Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                      const MountainSettings& settings)
{
    auto mountains = FindMountains(map, rnd, mountainLevel, settings);

    for(const Mountain& mountain : mountains)
    {
        RaiseMountain(map, rnd, mountain);
    }

    return mountains;
}

} // namespace rttr::mapGenerator
