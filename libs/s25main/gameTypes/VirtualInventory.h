// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
