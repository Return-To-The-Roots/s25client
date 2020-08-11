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

boost::container::static_vector<BuildingType, NUM_BUILDING_TYPES / 4u> BuildingProperties::militaryBldTypes;

void BuildingProperties::Init()
{
    militaryBldTypes.clear();
    for(unsigned i = 0; i < NUM_BUILDING_TYPES; i++)
    {
        auto bld = BuildingType(i);
        if(IsMilitary(bld))
            militaryBldTypes.push_back(bld);
    }
}

bool BuildingProperties::IsMilitary(BuildingType bld)
{
    switch(bld)
    {
        case BLD_BARRACKS:
        case BLD_GUARDHOUSE:
        case BLD_WATCHTOWER:
        case BLD_FORTRESS: return true;
        default: return false;
    }
}

bool BuildingProperties::IsMine(BuildingType bld)
{
    switch(bld)
    {
        case BLD_GRANITEMINE:
        case BLD_COALMINE:
        case BLD_IRONMINE:
        case BLD_GOLDMINE: return true;
        default: return false;
    }
}

bool BuildingProperties::IsWareHouse(BuildingType bld)
{
    switch(bld)
    {
        case BLD_HEADQUARTERS:
        case BLD_HARBORBUILDING:
        case BLD_STOREHOUSE: return true;
        default: return false;
    }
}

bool BuildingProperties::IsValid(BuildingType bld)
{
    switch(bld)
    {
        case BLD_NOTHING2:
        case BLD_NOTHING3:
        case BLD_NOTHING4:
        case BLD_NOTHING5:
        case BLD_NOTHING6:
        case BLD_NOTHING7:
        case BLD_NOTHING9: return false;
        default: return true;
    }
}
