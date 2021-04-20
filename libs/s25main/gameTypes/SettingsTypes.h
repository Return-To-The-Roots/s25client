// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "helpers/MaxEnumValue.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include <array>
#include <tuple>

// TODO: Make this structs so meanings are obvious

/// 1 mapping of a required good to its building and default setting
using DistributionMapping = std::tuple<GoodType, BuildingType, uint8_t>;
/// List of all possible distribution mappings ordered by GoodType
using DistributionMap = std::array<DistributionMapping, 23>;
extern const DistributionMap distributionMap;
/// List of the percentage a building should get from a specific ware
using Distributions = std::array<uint8_t, std::tuple_size<DistributionMap>::value>;
/// Ordering of building types by priority. All buildings in here except unused and HQ
using BuildOrders = std::array<BuildingType, helpers::NumEnumValues_v<BuildingType> - NUM_UNUSED_BLD_TYPES - 1>;
/// Mapping transport priority -> standard transport priority of ware(group):
/// E.g. std prio of coins = 0 -> TransportOrders[0] = stdPrio[COINS] = 0
/// New prio of coins = 1 -> TransportOrders[1] = stdPrio[COINS] = 0
using TransportOrders = std::array<uint8_t, 14>;
using TransportPriorities = helpers::EnumArray<uint8_t, GoodType>;
/// Priority of each tool
using ToolSettings = std::array<uint8_t, NUM_TOOLS>;
/// Value of each military slider
/// 0: Recruiting ratio (to max possible recruits)
/// 1: Defender strength (ratio to max available rank)
/// 2: Active defenders (engaging attackers by leaving building): Chance that one is sent
/// 3: Ratio of used attackers to available attackers
/// 4-7: Ratio of soldiers in buildings to full occupation for inland, middle region, harbor spots, border regions
using MilitarySettings = std::array<uint8_t, 8>;
