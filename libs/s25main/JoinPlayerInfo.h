// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlayerInfo.h"

/// Holds information for players relevant during match-making
struct JoinPlayerInfo : public PlayerInfo
{
    std::string originName;
    bool isReady = false;

    JoinPlayerInfo();
    explicit JoinPlayerInfo(const BasePlayerInfo& baseInfo);
    explicit JoinPlayerInfo(const PlayerInfo& playerInfo);
    explicit JoinPlayerInfo(Serializer& ser);

    // Serialize complete struct
    void serialize(Serializer& ser) const;

    void setAIName(unsigned playerId);
    // Recovers fixed data in savegames after player slots are swapped
    void fixSwappedSaveSlot(JoinPlayerInfo& other);

    static std::string makeAIName(const AI::Info& aiInfo, unsigned playerId);
};
