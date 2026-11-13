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

/**
 * Computes the desired number of nodes of a mountain.
 *
 * @param settings size limits for mountains
 * @param index index of the mountain - the first mountain has index 0
 *
 * @returns the desired size which decreases with the index but never drops below the minimum size.
 */
unsigned GetMountainSize(const MountainSettings& settings, unsigned index);

/**
 * Computes for each node the distance to the closest node which is not free land. Nodes are not considered free land
 * if they are water, reach the mountain level or already have a texture (e.g. rivers and islands).
 *
 * @param map map to compute the distances for
 * @param mountainLevel minimum z-value of mountains
 *
 * @returns the distance of each node to the closest node which isn't free land.
 */
NodeMapBase<unsigned> GetFreeLandDistances(const Map& map, unsigned mountainLevel);

/**
 * Grows a mountain around the specified center. Nodes close to the center and far away from other obstacles are
 * preferred. Skipping nodes by chance creates an irregular shape.
 *
 * @param freeLand distances to the closest node which isn't free land
 * @param rnd random number generator
 * @param center center of the mountain which is always part of the mountain
 * @param size maximum number of nodes of the mountain
 * @param margin only nodes with a distance greater than this value become part of the mountain
 *
 * @returns all nodes of the new mountain.
 */
Mountain GrowMountain(const NodeMapBase<unsigned>& freeLand, RandomUtility& rnd, const MapPoint& center, unsigned size,
                      unsigned margin);

/**
 * Selects the nodes of new mountains until no node is further away from water or mountains than the minimum of free
 * land to keep. Every mountain starts at the node with the largest distance to water and mountains. As nodes close to
 * obstacles are never selected, passages between areas of the map stay open.
 *
 * @param map map to search for free land (is not modified)
 * @param rnd random number generator
 * @param mountainLevel minimum z-value of mountains
 * @param settings settings of the new mountains
 *
 * @returns the nodes of each new mountain. All mountains are disjoint.
 */
std::vector<Mountain> FindMountains(const Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                    const MountainSettings& settings);

/**
 * Selects the nodes of new mountains just like the overload above but asks for the size of each mountain right before
 * growing it. This allows callers to vary the size, e.g. depending on what the nodes are going to be used for.
 *
 * @param map map to search for free land (is not modified)
 * @param rnd random number generator
 * @param mountainLevel minimum z-value of mountains
 * @param minFreeLand land up to this distance to water or mountains stays free
 * @param getSize returns the desired number of nodes for the mountain with the specified index
 *
 * @returns the nodes of each new mountain. All mountains are disjoint.
 */
std::vector<Mountain> FindMountains(const Map& map, RandomUtility& rnd, unsigned mountainLevel, unsigned minFreeLand,
                                    const std::function<unsigned(unsigned)>& getSize);

/**
 * Computes the distance of each node of the mountain to the foot of the mountain. The foot consists of all nodes of the
 * mountain with at least one neighbor which isn't part of the mountain.
 *
 * @param size size of the map
 * @param mountain nodes of the mountain
 *
 * @returns the distance to the foot for each node of the mountain and zero for all other nodes.
 */
NodeMapBase<unsigned> GetFootDistances(const MapExtent& size, const Mountain& mountain);

/**
 * Elevates the nodes of the mountain relative to the surrounding land and applies mountain textures to them.
 *
 * @param map reference to the map to place the mountain on (manipulates textures and z-values of the map)
 * @param rnd random number generator
 * @param mountain nodes of the mountain
 */
void RaiseMountain(Map& map, RandomUtility& rnd, const Mountain& mountain);

/**
 * Reduces the size of free land by adding new mountains to the map.
 *
 * @param map reference to the map to place the mountains on (manipulates textures and z-values of the map)
 * @param rnd random number generator
 * @param mountainLevel minimum z-value of mountains
 * @param settings settings of the new mountains
 *
 * @returns the nodes of each new mountain.
 */
std::vector<Mountain> CreateMountains(Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                      const MountainSettings& settings);

} // namespace rttr::mapGenerator
