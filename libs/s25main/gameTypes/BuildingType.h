// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <map>
#include <string>

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

const std::map<BuildingType, std::string> BUILDING_NAMES_1 = {{BuildingType::Headquarters, "Headquarters"},
                                               {BuildingType::Barracks, "Barracks"},
                                               {BuildingType::Guardhouse, "Guardhouse"},
                                               {BuildingType::Nothing2, "Nothing2"},
                                               {BuildingType::Watchtower, "Watchtower"},
                                               {BuildingType::Vineyard, "Vineyard"},
                                               {BuildingType::Winery, "Winery"},
                                               {BuildingType::Temple, "Temple"},
                                               {BuildingType::Nothing6, "Nothing6"},
                                               {BuildingType::Fortress, "Fortress"},
                                               {BuildingType::GraniteMine, "GraniteMine"},
                                               {BuildingType::CoalMine, "CoalMine"},
                                               {BuildingType::IronMine, "IronMine"},
                                               {BuildingType::GoldMine, "GoldMine"},
                                               {BuildingType::LookoutTower, "LookoutTower"},
                                               {BuildingType::Nothing7, "Nothing7"},
                                               {BuildingType::Catapult, "Catapult"},
                                               {BuildingType::Woodcutter, "Woodcutter"},
                                               {BuildingType::Fishery, "Fishery"},
                                               {BuildingType::Quarry, "Quarry"},
                                               {BuildingType::Forester, "Forester"},
                                               {BuildingType::Slaughterhouse, "Slaughterhouse"},
                                               {BuildingType::Hunter, "Hunter"},
                                               {BuildingType::Brewery, "Brewery"},
                                               {BuildingType::Armory, "Armory"},
                                               {BuildingType::Metalworks, "Metalworks"},
                                               {BuildingType::Ironsmelter, "Ironsmelter"},
                                               {BuildingType::Charburner, "Charburner"},
                                               {BuildingType::PigFarm, "PigFarm"},
                                               {BuildingType::Storehouse, "Storehouse"},
                                               {BuildingType::Nothing9, "Nothing9"},
                                               {BuildingType::Mill, "Mill"},
                                               {BuildingType::Bakery, "Bakery"},
                                               {BuildingType::Sawmill, "Sawmill"},
                                               {BuildingType::Mint, "Mint"},
                                               {BuildingType::Well, "Well"},
                                               {BuildingType::Shipyard, "Shipyard"},
                                               {BuildingType::Farm, "Farm"},
                                               {BuildingType::DonkeyBreeder, "DonkeyBreeder"},
                                               {BuildingType::HarborBuilding, "HarborBuilding"}};
