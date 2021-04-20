// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

enum class PlayerState : uint8_t
{
    Free,
    Occupied,
    Locked,
    AI
};
constexpr auto maxEnumValue(PlayerState)
{
    return PlayerState::AI;
}
