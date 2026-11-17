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
    const BasePlayerInfo& getPlayer() const override;

public:
    LuaServerPlayer(IGameLobbyController& lobbyServerController, unsigned playerId);
    static void registerClasses(kaguya::State& state);

    void setNation(lua::SafeEnum<Nation> nat);
    void setTeam(lua::SafeEnum<Team> team);
    void setColor(unsigned colorOrIdx);
    void close();
    void setAI(unsigned level);
    void setName(const std::string& name);
    void setPortrait(unsigned int portraitIndex);
};
