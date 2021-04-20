// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GoodTypes.h"
#include "GoodsAndPeopleArray.h"
#include "JobTypes.h"
#include "RTTR_Assert.h"

/// Struct for wares and people (for HQs, warehouses etc)
struct Inventory : GoodsAndPeopleArray<unsigned>
{
    /// Sets everything to 0
    void clear();
    void Add(const GoodType good, unsigned amount = 1) { goods[good] += amount; }
    void Add(const Job job, unsigned amount = 1) { people[job] += amount; }
    void Remove(const GoodType good, unsigned amount = 1)
    {
        RTTR_Assert(goods[good] >= amount);
        goods[good] -= amount;
    }
    void Remove(const Job job, unsigned amount = 1)
    {
        RTTR_Assert(people[job] >= amount);
        people[job] -= amount;
    }
};
