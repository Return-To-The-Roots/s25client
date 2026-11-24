// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GoodTypes.h"
#include "GoodsAndPeopleArray.h"
#include "JobTypes.h"

/// Struct for wares and people (for HQs, warehouses etc)
struct Inventory : private GoodsAndPeopleCounts
{
    using GoodsAndPeopleCounts::armoredSoldiers;
    using GoodsAndPeopleCounts::goods;
    using GoodsAndPeopleCounts::healedSoldiers;
    using GoodsAndPeopleCounts::people;
    // Write access is only allowed via Add and Remove, or explicitely via underlying arrays
    // to ensure amounts are non-negative and armored/healed soldier counts are valid
    auto operator[](GoodType good) const { return goods[good]; }
    auto operator[](Job job) const { return people[job]; }
    auto operator[](ArmoredSoldier soldier) const { return armoredSoldiers[soldier]; }
    auto operator[](HealedSoldier soldier) const { return healedSoldiers[soldier]; }

    GoodCounts& goodsOnly() { return *this; }
    const GoodCounts& goodsOnly() const { return *this; }
    PeopleCounts& peopleOnly() { return *this; }
    const PeopleCounts& peopleOnly() const { return *this; }

    /// Sets everything to 0
    void clear();
    void add(const GoodType good, unsigned amount = 1) { goods[good] += amount; }
    void add(const Job job, unsigned amount = 1) { people[job] += amount; }
    void add(const ArmoredSoldier soldier, unsigned amount = 1)
    {
        armoredSoldiers[soldier] += amount;
        RTTR_Assert(armoredSoldiers[soldier] <= people[amoredEnumToSoldierEnum(soldier)]);
    }
    void add(const HealedSoldier soldier, unsigned amount = 1)
    {
        healedSoldiers[soldier] += amount;
        RTTR_Assert(healedSoldiers[soldier] <= people[healedEnumToSoldierEnum(soldier)]);
    }
    void remove(const GoodType good, unsigned amount = 1)
    {
        RTTR_Assert(goods[good] >= amount);
        goods[good] -= amount;
    }
    void remove(const Job job, unsigned amount = 1)
    {
        RTTR_Assert(people[job] >= amount);
        people[job] -= amount;
    }
    void remove(const ArmoredSoldier soldier, unsigned amount = 1)
    {
        RTTR_Assert(armoredSoldiers[soldier] >= amount);
        armoredSoldiers[soldier] -= amount;
        RTTR_Assert(armoredSoldiers[soldier] <= people[amoredEnumToSoldierEnum(soldier)]);
    }
    void remove(const HealedSoldier soldier, unsigned amount = 1)
    {
        RTTR_Assert(healedSoldiers[soldier] >= amount);
        healedSoldiers[soldier] -= amount;
        RTTR_Assert(healedSoldiers[soldier] <= people[healedEnumToSoldierEnum(soldier)]);
    }
};
