// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"
#include <array>

/// Static class to query properties of buildings (building types)
class BuildingProperties
{
public:
    /// Static class only!
    BuildingProperties() = delete;

    /// Stores the bld types that are military blds as a cache
    static const std::array<BuildingType, 4> militaryBldTypes;

    /// True iff the building type is used (not nothing)
    static bool IsValid(BuildingType bld);
    /// True iff this is a regular military building
    static bool IsMilitary(BuildingType bld);
    /// True iff this is a mine
    static bool IsMine(BuildingType bld);
    /// True iff wares can be stored in this building
    static bool IsWareHouse(BuildingType bld);
    /// True iff this is a "usual" building, i.e. production.
    static bool IsUsual(BuildingType bld);
};
