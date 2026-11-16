// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaServerPlayer.h"
#include "JoinPlayerInfo.h"
#include "lua/LuaHelpers.h"
#include "network/IGameLobbyController.h"
#include "s25util/colors.h"
#include <kaguya/kaguya.hpp>

const BasePlayerInfo& LuaServerPlayer::getPlayer() const
{
    return player;
}

LuaServerPlayer::LuaServerPlayer(IGameLobbyController& lobbyServerController, unsigned playerId)
    : lobbyServerController_(lobbyServerController), playerId(playerId),
      player(lobbyServerController_.getJoinPlayer(playerId))
{}

void LuaServerPlayer::Register(kaguya::State& state)
{
    LuaPlayerBase::Register(state);
    state["Player"].setClass(kaguya::UserdataMetatable<LuaServerPlayer, LuaPlayerBase>()
                               .addFunction("SetNation", &LuaServerPlayer::setNation)
                               .addFunction("SetTeam", &LuaServerPlayer::setTeam)
                               .addFunction("SetColor", &LuaServerPlayer::setColor)
                               .addFunction("Close", &LuaServerPlayer::Close)
                               .addFunction("SetAI", &LuaServerPlayer::setAI)
                               .addFunction("SetName", &LuaServerPlayer::setName)
                               .addFunction("SetPortrait", &LuaServerPlayer::setPortrait));
}

void LuaServerPlayer::setNation(lua::SafeEnum<Nation> nat)
{
    player.nation = nat;
    lobbyServerController_.setNation(playerId, nat);
}

void LuaServerPlayer::setTeam(lua::SafeEnum<Team> team)
{
    player.team = team;
    lobbyServerController_.setTeam(playerId, team);
}

void LuaServerPlayer::setColor(unsigned colorOrIdx)
{
    if(GetAlpha(colorOrIdx) == 0)
    {
        lua::assertTrue(colorOrIdx < PLAYER_COLORS.size(), "Invalid color");
        player.color = PLAYER_COLORS[colorOrIdx];
    } else
        player.color = colorOrIdx;
    lobbyServerController_.setColor(playerId, player.color);
}

void LuaServerPlayer::Close()
{
    if(player.ps == PlayerState::Locked)
        return;
    lobbyServerController_.CloseSlot(playerId);
}

void LuaServerPlayer::setAI(unsigned level)
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
    lobbyServerController_.setPlayerState(playerId, PlayerState::AI, info);
}

void LuaServerPlayer::setName(const std::string& name)
{
    lobbyServerController_.setName(playerId, name);
}

void LuaServerPlayer::setPortrait(unsigned int portraitIndex)
{
    lobbyServerController_.setPortrait(playerId, portraitIndex);
}
