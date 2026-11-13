// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/Mountains.h"
#include "mapGenerator/RandomUtility.h"
#include <array>
#include <vector>

namespace rttr::mapGenerator {

/// Types of landscape used to fill up large areas of free land
enum class LandFill
{
    Mountain,
    Desert,
    Swamp
};

/// Number of different types of landscape to fill free land with
constexpr unsigned numLandFills = 3;

/// Desired amount of each type of landscape - indexed by LandFill. Only the proportions matter.
using LandFillAmounts = std::array<unsigned, numLandFills>;

struct FilledArea
{
    LandFill type;
    Mountain nodes;
};

/// Object type of decorative objects
constexpr uint8_t OT_Decoration = 0xC8;
/// Object indices of the decorative objects used for deserts and swamps
constexpr uint8_t OI_DeadTreeTrunk = 0x05;
constexpr uint8_t OI_DeadTree = 0x06;
constexpr uint8_t OI_Bone1 = 0x07;
constexpr uint8_t OI_Bone2 = 0x08;

/**
 * Closes gaps of an area which has been grown with an irregular shape. Nodes which are mostly surrounded by the area
 * become part of it.
 *
 * @param size size of the map
 * @param area nodes of the area
 *
 * @returns the nodes of the area including its gaps.
 */
Mountain CloseGaps(const MapExtent& size, const Mountain& area);

/**
 * Selects the type of landscape for the next area. Types which are below their desired share of nodes are chosen by
 * chance according to the desired amounts. This keeps the proportions even for a low number of areas. The first area
 * always becomes a mountain (if desired at all) since mountains are required for mining.
 *
 * @param rnd random number generator
 * @param amounts desired amount of each type of landscape
 * @param assigned number of nodes assigned to each type of landscape so far (indexed by LandFill)
 *
 * @returns the type of landscape for the next area. A mountain if all amounts are zero.
 */
LandFill SelectLandFill(RandomUtility& rnd, const LandFillAmounts& amounts, const LandFillAmounts& assigned);

/**
 * Turns the area into a desert hill. The hill is lower than a mountain and rises smoothly towards its center. The outer
 * nodes get buildable transition textures while the inner nodes are covered by desert (or the equivalent texture of the
 * landscape). Triangles are shared between nodes with the texture closer to the center taking precedence. Bones are
 * placed around the center of the hill.
 *
 * @param map reference to the map to place the hill on (manipulates textures, objects and z-values of the map)
 * @param rnd random number generator
 * @param area nodes of the desert hill
 */
void RaiseDesertHill(Map& map, RandomUtility& rnd, const Mountain& area);

/**
 * Turns the area into a swamp (or the equivalent texture of the landscape). All nodes of the swamp are lowered to the
 * same height and the surrounding land is lowered to descend to the swamp by one level per node at most. Dead trees are
 * placed on the land right next to the swamp.
 *
 * @param map reference to the map to place the swamp on (manipulates textures, objects and z-values of the map)
 * @param rnd random number generator
 * @param area nodes of the swamp
 */
void CreateSwamp(Map& map, RandomUtility& rnd, const Mountain& area);

/**
 * Makes sure deserts never touch lush land: the nodes around a desert get the driest land texture followed by one ring
 * for each of the next textures unless the land is dry anyway. This function has to be called once all textures are
 * in place as texturing coasts may replace the transitions applied by desert hills.
 *
 * @param map reference to the map to update the textures for
 */
void SmoothDesertTransitions(Map& map);

/**
 * Reduces the size of free land by adding mountains, desert hills and swamps to the map.
 *
 * @param map reference to the map to fill (manipulates textures, objects and z-values of the map)
 * @param rnd random number generator
 * @param mountainLevel minimum z-value of mountains
 * @param settings settings for the size of the new areas - swamps only get half the size as they are impassable
 * @param amounts desired amount of each type of landscape
 *
 * @returns the type and the nodes of each new area. Gaps of desert hills and swamps are closed.
 */
std::vector<FilledArea> FillFreeLand(Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                     const MountainSettings& settings, const LandFillAmounts& amounts);

} // namespace rttr::mapGenerator
