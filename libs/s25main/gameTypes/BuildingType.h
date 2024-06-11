// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

enum class BuildingType : uint8_t
{
    Headquarters,   // 0
    Barracks,       // 1
    Guardhouse,     // 2
    Nothing2,       // 3
    Watchtower,     // 4
    Vineyard,       // 5
    Winery,         // 6
    Temple,         // 7
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
const unsigned NUM_UNUSED_BLD_TYPES = 4;
