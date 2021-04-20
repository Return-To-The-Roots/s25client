// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
