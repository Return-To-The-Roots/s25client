// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// Nations (byte sized)
enum class Nation : uint8_t
{
    Africans,
    Japanese,
    Romans,
    Vikings,
    Babylonians
};
constexpr auto maxEnumValue(Nation)
{
    return Nation::Babylonians;
}

/// Number of native nations
constexpr unsigned NUM_NATIVE_NATIONS = 4;
