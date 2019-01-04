// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef SettingsTypes_h__
#define SettingsTypes_h__

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include <array>
#include <tuple>

// TODO: Make this structs so meanings are obvious

/// 1 mapping of a required good to its building and default setting
typedef std::tuple<GoodType, BuildingType, uint8_t> DistributionMapping;
/// List of all possible distribution mappings ordered by GoodType
typedef std::array<DistributionMapping, 23> DistributionMap;
extern const DistributionMap SUPPRESS_UNUSED distributionMap;
/// List of the percentage a building should get from a specific ware
typedef std::array<uint8_t, std::tuple_size<DistributionMap>::value> Distributions;
/// Ordering of building types by priority. All buildings in here except unused and HQ
typedef std::array<BuildingType, NUM_BUILDING_TYPES - NUM_UNUSED_BLD_TYPES - 1> BuildOrders;
/// Mapping transport priority -> standard transport priority of ware(group):
/// E.g. std prio of coins = 0 -> TransportOrders[0] = stdPrio[COINS] = 0
/// New prio of coins = 1 -> TransportOrders[1] = stdPrio[COINS] = 0
typedef std::array<uint8_t, 14> TransportOrders;
typedef std::array<uint8_t, NUM_WARE_TYPES> TransportPriorities;
/// Priority of each tool
typedef std::array<uint8_t, NUM_TOOLS> ToolSettings;
/// Value of each military slider
/// 0: Recruiting ratio (to max possible recruits)
/// 1: Defender strength (ratio to max available rank)
/// 2: Active defenders (engaging attackers by leaving building): Chance that one is sent
/// 3: Ratio of used attackers to available attackers
/// 4-7: Ratio of soldiers in buildings to full occupation for inland, middle region, harbor spots, border regions
typedef std::array<uint8_t, 8> MilitarySettings;

#endif // SettingsTypes_h__
