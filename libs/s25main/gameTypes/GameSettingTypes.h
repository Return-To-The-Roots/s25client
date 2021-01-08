// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
