// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    unsigned portraitIndex;
    Nation nation;
    /// Actual color (ARGB)
    unsigned color;
    Team team;

    BasePlayerInfo();
    /// Deserialize data. If lightData is true, unused data is not read (e.g. unused slot -> Skip rest)
    BasePlayerInfo(Serializer& ser, int version, bool lightData);
    /// Serialize data. If lightData is true, unused data is not written (e.g. unused slot -> Skip rest)
    void Serialize(Serializer& ser, bool lightData) const;

    /// Slot used by a human player (has socket etc)
    bool isHuman() const { return (ps == PlayerState::Occupied); }
    /// Slot filled (Used by human or AI, but excludes currently connecting humans)
    bool isUsed() const { return (ps == PlayerState::AI || ps == PlayerState::Occupied); }

    /// Returns index of color in PLAYER_COLORS array or -1 if not found
    int GetColorIdx() const;
    static int GetColorIdx(unsigned color);

    // 0: Initial
    // 1: Added portraitIndex
    static inline constexpr int getCurrentVersion() { return 1; }
};
