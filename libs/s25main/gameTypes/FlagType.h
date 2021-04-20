// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

enum class FlagType : uint8_t
{
    Normal,
    Large,
    Water
};
constexpr auto maxEnumValue(FlagType)
{
    return FlagType::Water;
}

enum class GraniteType : uint8_t
{
    One,
    Two
};
constexpr auto maxEnumValue(GraniteType)
{
    return GraniteType::Two;
}
