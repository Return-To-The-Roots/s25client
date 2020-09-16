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

#include "gameTypes/Inventory.h"

/// Inventory which is divided into a real and a visual part
/// Mainly for warehouses, where the visual part is the amount currently in the warehouse (including those, that are to
/// be moved out) and the real part is the amount that is available for use
struct VirtualInventory
{
    Inventory visual, real;

    VirtualInventory() { clear(); }
    /// Sets everything to 0
    void clear()
    {
        visual.clear();
        real.clear();
    }
    /// Adds goods to both inventories
    void Add(const GoodType good, unsigned amount = 1)
    {
        visual.Add(good, amount);
        real.Add(good, amount);
    }
    /// Adds figures to both inventories
    void Add(const Job job, unsigned amount = 1)
    {
        visual.Add(job, amount);
        real.Add(job, amount);
    }
    /// Removes goods from both inventories
    void Remove(const GoodType good, unsigned amount = 1)
    {
        visual.Remove(good, amount);
        real.Remove(good, amount);
    }
    /// Removes jobs from both inventories
    void Remove(const Job job, unsigned amount = 1)
    {
        visual.Remove(job, amount);
        real.Remove(job, amount);
    }

    /// Returns the real number of people of the given type
    unsigned operator[](Job job) const { return real.people[job]; }
    /// Returns the real number of wares of the given type
    unsigned operator[](GoodType good) const { return real.goods[good]; }
};
