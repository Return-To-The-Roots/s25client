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

#include "rttrDefines.h" // IWYU pragma: keep
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
    : lobbyServerController_(lobbyServerController), playerId(playerId), player(lobbyServerController_.GetJoinPlayer(playerId))
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

void LuaServerPlayer::SetNation(Nation nat)
{
    lua::assertTrue(unsigned(nat) < NUM_NATS, "Invalid Nation");
    player.nation = nat;
    lobbyServerController_.SetNation(playerId, nat);
}

void LuaServerPlayer::SetTeam(Team team)
{
    lua::assertTrue(unsigned(team) < NUM_TEAMS, "Invalid team");
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
    if(player.ps == PS_LOCKED)
        return;
    lobbyServerController_.CloseSlot(playerId);
}

void LuaServerPlayer::SetAI(unsigned level)
{
    AI::Info info(AI::DEFAULT);
    switch(level)
    {
        case 0: info.type = AI::DUMMY; break;
        case 1: info.level = AI::EASY; break;
        case 2: info.level = AI::MEDIUM; break;
        case 3: info.level = AI::HARD; break;
        default: lua::assertTrue(false, "Invalid AI level");
    }
    lobbyServerController_.SetPlayerState(playerId, PS_AI, info);
}

void LuaServerPlayer::SetName(const std::string& name)
{
    lobbyServerController_.SetName(playerId, name);
}
