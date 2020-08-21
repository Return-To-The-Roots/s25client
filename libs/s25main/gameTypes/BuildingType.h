// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "helpers/MaxEnumValue.h"
#include <cstdint>

enum BuildingType : uint8_t
{
    BLD_HEADQUARTERS,   // 0
    BLD_BARRACKS,       // 1
    BLD_GUARDHOUSE,     // 2
    BLD_NOTHING2,       // 3
    BLD_WATCHTOWER,     // 4
    BLD_NOTHING3,       // 5
    BLD_NOTHING4,       // 6
    BLD_NOTHING5,       // 7
    BLD_NOTHING6,       // 8
    BLD_FORTRESS,       // 9
    BLD_GRANITEMINE,    // 10
    BLD_COALMINE,       // 11
    BLD_IRONMINE,       // 12
    BLD_GOLDMINE,       // 13
    BLD_LOOKOUTTOWER,   // 14
    BLD_NOTHING7,       // 15
    BLD_CATAPULT,       // 16
    BLD_WOODCUTTER,     // 17
    BLD_FISHERY,        // 18
    BLD_QUARRY,         // 19
    BLD_FORESTER,       // 20
    BLD_SLAUGHTERHOUSE, // 21
    BLD_HUNTER,         // 22
    BLD_BREWERY,        // 23
    BLD_ARMORY,         // 24
    BLD_METALWORKS,     // 25
    BLD_IRONSMELTER,    // 26
    BLD_CHARBURNER,     // 27
    BLD_PIGFARM,        // 28
    BLD_STOREHOUSE,     // 29
    BLD_NOTHING9,       // 30
    BLD_MILL,           // 31
    BLD_BAKERY,         // 32
    BLD_SAWMILL,        // 33
    BLD_MINT,           // 34
    BLD_WELL,           // 35
    BLD_SHIPYARD,       // 36
    BLD_FARM,           // 37
    BLD_DONKEYBREEDER,  // 38
    BLD_HARBORBUILDING, // 39
};

constexpr auto maxEnumValue(BuildingType)
{
    return BLD_HARBORBUILDING;
}
/// Number of building types
constexpr unsigned NUM_BUILDING_TYPES = helpers::NumEnumValues_v<BuildingType>;

/// Number of NOTHING entries (currently unused buildings)
const unsigned NUM_UNUSED_BLD_TYPES = 7;
/// First usual building (building that produces something)
const unsigned FIRST_USUAL_BUILDING = BLD_GRANITEMINE;
