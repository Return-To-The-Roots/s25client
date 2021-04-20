// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// Animal species
enum class Species : uint8_t
{
    PolarBear,
    RabbitWhite,
    RabbitGrey,
    Fox,
    Stag,
    Deer,
    Duck,
    Sheep
};
constexpr auto maxEnumValue(Species)
{
    return Species::Sheep;
}
