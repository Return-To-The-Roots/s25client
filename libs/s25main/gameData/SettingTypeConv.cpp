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

#include "SettingTypeConv.h"
#include "RTTR_Assert.h"
#include "helpers/EnumRange.h"
#include <stdexcept>

/// Max value for each setting.
/// Note: We skip the first 2 steppings for the occupation (last 4 values) as they had no effect in S2
const MilitarySettings SUPPRESS_UNUSED MILITARY_SETTINGS_SCALE = {{10, 5, 5, 5, 8, 8, 8, 8}};
const TransportPriorities STD_TRANSPORT_PRIO = {{2,  12, 12, 12, 12, 12, 12, 12, 12, 12, 10, 10, 12, 12, 12, 13, 1, 3,
                                                 11, 11, 11, 1,  9,  7,  8,  1,  1,  11, 0,  4,  5,  6,  11, 11, 1}};

unsigned GetTransportPrioFromOrdering(const TransportOrders& ordering, GoodType good)
{
    // The priority is the index into the ordering -> 0 = highest, first entry
    for(unsigned prio = 0; prio < ordering.size(); ++prio)
    {
        // The entry values are the standard priorities which makes it simple to use groups
        // of goods, that always have the same priority
        if(ordering[prio] == STD_TRANSPORT_PRIO[good])
            return prio;
    }
    RTTR_Assert(false);
    throw std::logic_error("Invalid ordering or ware");
}

TransportOrders GetOrderingFromTransportPrio(const TransportPriorities& priorities)
{
    TransportOrders result;
    // Map prio of each ware to STD prio
    for(const auto ware : helpers::EnumRange<GoodType>{})
    {
        RTTR_Assert(priorities[ware] < result.size());
        result[priorities[ware]] = STD_TRANSPORT_PRIO[ware];
    }
    return result;
}
