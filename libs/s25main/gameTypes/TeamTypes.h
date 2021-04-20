// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

enum class Team : uint8_t
{
    None,
    Random,
    Team1,
    Team2,
    Team3,
    Team4,
    Random1To2, // Insert "smart" into teams 1 or 2
    Random1To3, // Insert "smart" into teams 1 or 2 or 3
    Random1To4  // Insert "smart" into teams 1 or 2 or 3 or 4
};
constexpr auto maxEnumValue(Team)
{
    return Team::Random1To4;
}

/// Number of playable teams.
constexpr unsigned NUM_TEAMS = 4;

/// Return true iff this is a valid team (1-n)
constexpr auto isTeam(Team team)
{
    const int num = static_cast<int>(team) - static_cast<int>(Team::Team1);
    return num >= 0 && static_cast<unsigned>(num) < NUM_TEAMS;
}
