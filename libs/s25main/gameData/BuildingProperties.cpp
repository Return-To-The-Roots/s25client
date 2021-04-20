// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuildingProperties.h"
#include "helpers/EnumRange.h"

const std::array<BuildingType, 4> BuildingProperties::militaryBldTypes = []() {
    std::array<BuildingType, 4> result{};
    unsigned cur = 0;
    for(const auto bld : helpers::enumRange<BuildingType>())
    {
        if(BuildingProperties::IsMilitary(bld))
            result[cur++] = bld;
    }
    if(cur != result.size())
        throw "Invalid size";
    return result;
}();

bool BuildingProperties::IsMilitary(BuildingType bld)
{
    switch(bld)
    {
        case BuildingType::Barracks:
        case BuildingType::Guardhouse:
        case BuildingType::Watchtower:
        case BuildingType::Fortress: return true;
        default: return false;
    }
}

bool BuildingProperties::IsMine(BuildingType bld)
{
    switch(bld)
    {
        case BuildingType::GraniteMine:
        case BuildingType::CoalMine:
        case BuildingType::IronMine:
        case BuildingType::GoldMine: return true;
        default: return false;
    }
}

bool BuildingProperties::IsWareHouse(BuildingType bld)
{
    switch(bld)
    {
        case BuildingType::Headquarters:
        case BuildingType::HarborBuilding:
        case BuildingType::Storehouse: return true;
        default: return false;
    }
}

bool BuildingProperties::IsUsual(BuildingType bld)
{
    return IsValid(bld) && !IsMilitary(bld) && !IsWareHouse(bld);
}

bool BuildingProperties::IsValid(BuildingType bld)
{
    switch(bld)
    {
        case BuildingType::Nothing2:
        case BuildingType::Nothing3:
        case BuildingType::Nothing4:
        case BuildingType::Nothing5:
        case BuildingType::Nothing6:
        case BuildingType::Nothing7:
        case BuildingType::Nothing9: return false;
        default: return true;
    }
}
