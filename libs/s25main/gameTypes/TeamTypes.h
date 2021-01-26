// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
