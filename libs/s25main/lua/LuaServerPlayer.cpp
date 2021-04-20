// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaServerPlayer.h"
#include "JoinPlayerInfo.h"
#include "lua/LuaHelpers.h"
#include "network/IGameLobbyController.h"
#include "s25util/colors.h"
#include <kaguya/kaguya.hpp>

const BasePlayerInfo& LuaServerPlayer::GetPlayer() const
{
    return player;
}

LuaServerPlayer::LuaServerPlayer(IGameLobbyController& lobbyServerController, unsigned playerId)
    : lobbyServerController_(lobbyServerController), playerId(playerId),
      player(lobbyServerController_.GetJoinPlayer(playerId))
{}

void LuaServerPlayer::Register(kaguya::State& state)
{
    LuaPlayerBase::Register(state);
    state["Player"].setClass(kaguya::UserdataMetatable<LuaServerPlayer, LuaPlayerBase>()
                               .addFunction("SetNation", &LuaServerPlayer::SetNation)
                               .addFunction("SetTeam", &LuaServerPlayer::SetTeam)
                               .addFunction("SetColor", &LuaServerPlayer::SetColor)
                               .addFunction("Close", &LuaServerPlayer::Close)
                               .addFunction("SetAI", &LuaServerPlayer::SetAI)
                               .addFunction("SetName", &LuaServerPlayer::SetName));
}

void LuaServerPlayer::SetNation(lua::SafeEnum<Nation> nat)
{
    player.nation = nat;
    lobbyServerController_.SetNation(playerId, nat);
}

void LuaServerPlayer::SetTeam(lua::SafeEnum<Team> team)
{
    player.team = team;
    lobbyServerController_.SetTeam(playerId, team);
}

void LuaServerPlayer::SetColor(unsigned colorOrIdx)
{
    if(GetAlpha(colorOrIdx) == 0)
    {
        lua::assertTrue(colorOrIdx < PLAYER_COLORS.size(), "Invalid color");
        player.color = PLAYER_COLORS[colorOrIdx];
    } else
        player.color = colorOrIdx;
    lobbyServerController_.SetColor(playerId, player.color);
}

void LuaServerPlayer::Close()
{
    if(player.ps == PlayerState::Locked)
        return;
    lobbyServerController_.CloseSlot(playerId);
}

void LuaServerPlayer::SetAI(unsigned level)
{
    AI::Info info(AI::Type::Default);
    switch(level)
    {
        case 0: info.type = AI::Type::Dummy; break;
        case 1: info.level = AI::Level::Easy; break;
        case 2: info.level = AI::Level::Medium; break;
        case 3: info.level = AI::Level::Hard; break;
        default: lua::assertTrue(false, "Invalid AI level");
    }
    lobbyServerController_.SetPlayerState(playerId, PlayerState::AI, info);
}

void LuaServerPlayer::SetName(const std::string& name)
{
    lobbyServerController_.SetName(playerId, name);
}
