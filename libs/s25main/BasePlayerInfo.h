// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "gameTypes/AIInfo.h"
#include "gameTypes/Nation.h"
#include "gameTypes/PlayerState.h"
#include "gameTypes/TeamTypes.h"
#include <string>

class Serializer;

/// Basic player info (saved in replays and savegames)
struct BasePlayerInfo
{
    PlayerState ps;
    AI::Info aiInfo;
    std::string name;
    Nation nation;
    /// Actual color (ARGB)
    unsigned color;
    Team team;

    BasePlayerInfo();
    /// Deserialize data. If lightData is true, unused data is not read (e.g. unused slot -> Skip rest)
    BasePlayerInfo(Serializer& ser, bool lightData);
    /// Serialize data. If lightData is true, unused data is not written (e.g. unused slot -> Skip rest)
    void Serialize(Serializer& ser, bool lightData) const;

    /// Slot used by a human player (has socket etc)
    bool isHuman() const { return (ps == PS_OCCUPIED); }
    /// Slot filled (Used by human or AI, but excludes currently connecting humans)
    bool isUsed() const { return (ps == PS_AI || ps == PS_OCCUPIED); }

    /// Returns index of color in PLAYER_COLORS array or -1 if not found
    int GetColorIdx() const;
    static int GetColorIdx(unsigned color);
};
