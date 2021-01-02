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

#include <cstdint>

enum class BuildingType : uint8_t
{
    Headquarters,   // 0
    Barracks,       // 1
    Guardhouse,     // 2
    Nothing2,       // 3
    Watchtower,     // 4
    Nothing3,       // 5
    Nothing4,       // 6
    Nothing5,       // 7
    Nothing6,       // 8
    Fortress,       // 9
    GraniteMine,    // 10
    CoalMine,       // 11
    IronMine,       // 12
    GoldMine,       // 13
    LookoutTower,   // 14
    Nothing7,       // 15
    Catapult,       // 16
    Woodcutter,     // 17
    Fishery,        // 18
    Quarry,         // 19
    Forester,       // 20
    Slaughterhouse, // 21
    Hunter,         // 22
    Brewery,        // 23
    Armory,         // 24
    Metalworks,     // 25
    Ironsmelter,    // 26
    Charburner,     // 27
    PigFarm,        // 28
    Storehouse,     // 29
    Nothing9,       // 30
    Mill,           // 31
    Bakery,         // 32
    Sawmill,        // 33
    Mint,           // 34
    Well,           // 35
    Shipyard,       // 36
    Farm,           // 37
    DonkeyBreeder,  // 38
    HarborBuilding, // 39
};

constexpr auto maxEnumValue(BuildingType)
{
    return BuildingType::HarborBuilding;
}

/// Number of NOTHING entries (currently unused buildings)
const unsigned NUM_UNUSED_BLD_TYPES = 7;
