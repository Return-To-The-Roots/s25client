// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/Mountains.h"
#include "mapGenerator/RandomUtility.h"
#include <vector>

namespace rttr::mapGenerator {

/// Types of landscape used to fill up large areas of free land
enum class LandFill
{
    Mountain,
    Desert,
    Swamp
};
constexpr auto maxEnumValue(LandFill)
{
    return LandFill::Swamp;
}

/// Desired amount of each type of landscape. Only the proportions matter.
using LandFillAmounts = helpers::EnumArray<unsigned, LandFill>;

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

/// Returns the area extended by all nodes which are mostly surrounded by it.
Mountain closeGaps(const MapExtent& size, const Mountain& area);

/**
 * Selects the type of landscape for the next area from the types which are below their desired share.
 * The first area is always a mountain (if desired at all) since mountains are required for mining.
 *
 * @param assigned number of nodes assigned to each type so far
 */
LandFill selectLandFill(RandomUtility& rnd, const LandFillAmounts& amounts, const LandFillAmounts& assigned);

/// Turns the area into a desert hill with buildable transition textures at its foot and some bones around its center.
void raiseDesertHill(Map& map, RandomUtility& rnd, const Mountain& area);

/// Turns the area into a flat swamp surrounded by gently descending land and some dead trees.
void createSwamp(Map& map, RandomUtility& rnd, const Mountain& area);

/// Surrounds deserts by rings of increasingly lush land. Must be called once all textures are in place.
void smoothDesertTransitions(Map& map);

/**
 * Fills large areas of free land with mountains, desert hills and swamps.
 *
 * @param mountainLevel minimum z-value of mountains
 * @param settings size of the new areas - swamps only get half the size as they are impassable
 *
 * @returns the type and the nodes of each new area.
 */
std::vector<FilledArea> fillFreeLand(Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                     const MountainSettings& settings, const LandFillAmounts& amounts);

} // namespace rttr::mapGenerator
