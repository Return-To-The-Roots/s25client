// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
    void add(const GoodType good, unsigned amount = 1)
    {
        visual.add(good, amount);
        real.add(good, amount);
    }
    /// Adds figures to both inventories
    void add(const Job job, unsigned amount = 1)
    {
        visual.add(job, amount);
        real.add(job, amount);
    }
    /// Adds armored figures to both inventories
    void add(const ArmoredSoldier soldier, unsigned amount = 1)
    {
        visual.add(soldier, amount);
        real.add(soldier, amount);
    }
    /// Removes goods from both inventories
    void remove(const GoodType good, unsigned amount = 1)
    {
        visual.remove(good, amount);
        real.remove(good, amount);
    }
    /// Removes jobs from both inventories
    void remove(const Job job, unsigned amount = 1)
    {
        visual.remove(job, amount);
        real.remove(job, amount);
    }

    /// Removes armored figures from both inventories
    void remove(const ArmoredSoldier soldier, unsigned amount = 1)
    {
        visual.remove(soldier, amount);
        real.remove(soldier, amount);
    }

    /// Returns the real number of people of the given type
    unsigned operator[](Job job) const { return real[job]; }
    /// Returns the real number of wares of the given type
    unsigned operator[](GoodType good) const { return real[good]; }
    /// Returns the real number of armored soldiers of the given type
    unsigned operator[](ArmoredSoldier soldier) const { return real[soldier]; }
};
