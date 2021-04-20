// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "BasePlayerInfo.h"

/// Holds all information about a player (Not specific to game, match-making etc.)
struct PlayerInfo : public BasePlayerInfo
{
    bool isHost;
    unsigned ping;

    PlayerInfo();
    explicit PlayerInfo(const BasePlayerInfo& baseInfo);
    explicit PlayerInfo(Serializer& ser);

    // Serialize complete struct
    void Serialize(Serializer& ser) const;
};
