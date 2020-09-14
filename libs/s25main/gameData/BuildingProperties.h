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

#pragma once

#include "gameTypes/BuildingType.h"
#include <boost/container/static_vector.hpp>

/// Static class to query properties of buildings (building types)
class BuildingProperties
{
public:
    /// Static class only!
    BuildingProperties() = delete;

    static void Init();
    /// Stores the bld types that are military blds as a cache. Assumes that at most 1/4 of the blds are military
    static boost::container::static_vector<BuildingType, NUM_BUILDING_TYPES / 4u> militaryBldTypes;

    /// True iff the building type is used (not nothing)
    static bool IsValid(BuildingType bld);
    /// True iff this is a regular military building
    static bool IsMilitary(BuildingType bld);
    /// True iff this is a mine
    static bool IsMine(BuildingType bld);
    /// True iff wares can be stored in this building
    static bool IsWareHouse(BuildingType bld);
};
