// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlayerInfo.h"

/// Holds information for players relevant during match-making
struct GamePlayerInfo : public PlayerInfo
{
    GamePlayerInfo(unsigned playerId, const PlayerInfo& playerInfo);

    unsigned GetPlayerId() const { return id; }
    bool IsDefeated() const { return isDefeated; }

private:
    unsigned id;

protected:
    bool isDefeated;
};
