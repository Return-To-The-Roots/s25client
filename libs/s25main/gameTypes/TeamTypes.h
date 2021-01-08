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

enum Team : uint8_t
{
    TM_NOTEAM,
    TM_RANDOMTEAM,
    TM_TEAM1,
    TM_TEAM2,
    TM_TEAM3,
    TM_TEAM4,
    TM_RANDOMTEAM2,
    TM_RANDOMTEAM3,
    TM_RANDOMTEAM4
};
constexpr auto maxEnumValue(Team)
{
    return Team::TM_RANDOMTEAM4;
}

/// Anzahl der Team-Optionen
const unsigned NUM_TEAMS = 6; // teamrandom2,3,4 dont count
