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
