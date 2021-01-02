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
