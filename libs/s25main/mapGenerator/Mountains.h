// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"
#include <functional>
#include <set>
#include <vector>

namespace rttr::mapGenerator {

using Mountain = std::set<MapPoint, MapPointLess>;

struct MountainSettings
{
    /// Land up to this distance to water or mountains stays free. Larger areas are filled up with mountains.
    unsigned minFreeLand;
    /// Number of nodes of the first mountain. Each following mountain is smaller than the previous one.
    unsigned maxSize;
    /// Lower limit for the number of nodes of a mountain (unless there isn't sufficient free land).
    unsigned minSize;
};

/// Desired number of nodes of the mountain with the given index (0 = first). Decreases with the index down to minSize.
unsigned getMountainSize(const MountainSettings& settings, unsigned index);

/// Distance of each node to the closest node which isn't free land, i.e. water, mountain or already textured.
NodeMapBase<unsigned> getFreeLandDistances(const Map& map, unsigned mountainLevel);

/**
 * Grows an irregular mountain around the center preferring nodes far away from obstacles.
 *
 * @param freeLand distances to the closest node which isn't free land
 * @param size maximum number of nodes
 * @param margin only nodes with a larger distance to obstacles become part of the mountain
 */
Mountain growMountain(const NodeMapBase<unsigned>& freeLand, RandomUtility& rnd, const MapPoint& center, unsigned size,
                      unsigned margin);

/**
 * Selects the nodes of new mountains until no node is further away from obstacles than settings.minFreeLand.
 * Nodes close to obstacles are never selected so passages stay open. Does not modify the map.
 *
 * @returns the nodes of each mountain. all mountains are disjoint.
 */
std::vector<Mountain> findMountains(const Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                    const MountainSettings& settings);

/// Same as above but queries the size of each mountain (by its index) right before growing it.
std::vector<Mountain> findMountains(const Map& map, RandomUtility& rnd, unsigned mountainLevel, unsigned minFreeLand,
                                    const std::function<unsigned(unsigned)>& getSize);

/// Distance of each node of the mountain to its foot (its outermost nodes). Zero for nodes outside the mountain.
NodeMapBase<unsigned> getFootDistances(const MapExtent& size, const Mountain& mountain);

/// Elevates the nodes of the mountain and applies mountain textures to them.
void raiseMountain(Map& map, RandomUtility& rnd, const Mountain& mountain);

/// Fills large areas of free land with mountains and returns their nodes.
std::vector<Mountain> createMountains(Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                      const MountainSettings& settings);

} // namespace rttr::mapGenerator
