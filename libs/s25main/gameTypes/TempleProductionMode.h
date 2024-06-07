// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include <cstdint>

/// Production modi for the temple
enum class ProductionMode : uint8_t
{
    Default, // 25% gold, iron ore, coal, stone
    IronOre,
    Coal,
    Stone
};

constexpr auto maxEnumValue(ProductionMode)
{
    return ProductionMode::Stone;
}

const helpers::EnumArray<ProductionMode, ProductionMode> TRANSITIONS{
  {ProductionMode::IronOre, ProductionMode::Coal, ProductionMode::Stone, ProductionMode::Default}};
