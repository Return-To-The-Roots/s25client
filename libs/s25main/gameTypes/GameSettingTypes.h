// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

enum class GameSpeed : uint8_t
{
    VerySlow,
    Slow,
    Normal,
    Fast,
    VeryFast
};
constexpr auto maxEnumValue(GameSpeed)
{
    return GameSpeed::VeryFast;
}

enum class GameObjective : uint8_t
{
    None,
    Conquer3_4,
    TotalDomination,
    EconomyMode,
    // Different tournament mode lengths, see TOURNAMENT_MODES_DURATION
    Tournament1,
    Tournament2,
    Tournament3,
    Tournament4,
    Tournament5
};
constexpr auto maxEnumValue(GameObjective)
{
    return GameObjective::Tournament5;
}
constexpr unsigned NUM_TOURNAMENT_MODES = 5;

enum class StartWares : uint8_t
{
    VLow,
    Low,
    Normal,
    ALot
};
constexpr auto maxEnumValue(StartWares)
{
    return StartWares::ALot;
}

enum class Exploration : uint8_t
{
    Disabled,
    Classic,
    FogOfWar,
    FogOfWarExplored
};
constexpr auto maxEnumValue(Exploration)
{
    return Exploration::FogOfWarExplored;
}
