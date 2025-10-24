// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

enum class NodalObjectType : uint8_t
{
    Nothing,         // nothing
    Granite,         // granite
    Tree,            // tree
    Grainfield,      // grain field
    Environment,     // other environmental objects without a special function (dead trees, mushrooms, bushes)
    Object,          // other solid objects without a special function (stalagmites, ruins, etc.)
    Building,        // building
    Flag,            // flag
    Buildingsite,    // construction site
    Figure,          // settlers
    Extension,       // extension of large buildings
    Fire,            // a fire from a burning (destroyed) building
    Fighting,        // combat
    Animal,          // animal
    BurnedWarehouse, // burned-out warehouse from which people are pouring out
    Ship,            // ship
    CharburnerPile,  // wood/charcoal pile from the charcoal burner
    Grapefield,
};
constexpr auto maxEnumValue(NodalObjectType)
{
    return NodalObjectType::Grapefield;
}
