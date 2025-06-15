// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LuaPlayerBase.h"
#include "SafeEnum.h"
#include "gameTypes/Nation.h"

struct JoinPlayerInfo;
class IGameLobbyController;

class LuaServerPlayer : public LuaPlayerBase
{
    IGameLobbyController& lobbyServerController_;
    const unsigned playerId;
    JoinPlayerInfo& player;

protected:
    const BasePlayerInfo& GetPlayer() const override;

public:
    LuaServerPlayer(IGameLobbyController& lobbyServerController, unsigned playerId);
    static void Register(kaguya::State& state);

    void SetNation(lua::SafeEnum<Nation> nat);
    void SetTeam(lua::SafeEnum<Team> team);
    void SetColor(unsigned colorOrIdx);
    void Close();
    void SetAI(unsigned level);
    void SetName(const std::string& name);
    void SetPortrait(unsigned int portraitIndex);
};
