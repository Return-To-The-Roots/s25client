// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "LuaServerPlayer.h"
#include "GameMessages.h"
#include "GameServerInterface.h"
#include "JoinPlayerInfo.h"
#include "helpers/converters.h"
#include "lua/LuaHelpers.h"
#include "libutil/src/Log.h"
#include "libutil/src/colors.h"
#include <stdexcept>

const BasePlayerInfo& LuaServerPlayer::GetPlayer() const
{
    return player;
}

LuaServerPlayer::LuaServerPlayer(GameServerInterface& gameServer, unsigned playerId)
    : gameServer_(gameServer), playerId(playerId), player(gameServer_.GetJoinPlayer(playerId))
{
}

void LuaServerPlayer::Register(kaguya::State& state)
{
    LuaPlayerBase::Register(state);
    state["Player"].setClass(kaguya::UserdataMetatable<LuaServerPlayer, LuaPlayerBase>()
                               .addFunction("SetNation", &LuaServerPlayer::SetNation)
                               .addFunction("SetTeam", &LuaServerPlayer::SetTeam)
                               .addFunction("SetColor", &LuaServerPlayer::SetColor)
                               .addFunction("Close", &LuaServerPlayer::Close)
                               .addFunction("SetAI", &LuaServerPlayer::SetAI));
}

void LuaServerPlayer::SetNation(Nation nat)
{
    lua::assertTrue(unsigned(nat) < NAT_COUNT, "Invalid Nation");
    player.nation = nat;
    gameServer_.SendToAll(GameMessage_Player_Set_Nation(playerId, nat));
}

void LuaServerPlayer::SetTeam(Team team)
{
    lua::assertTrue(unsigned(team) < TEAM_COUNT, "Invalid team");
    player.team = team;
    gameServer_.SendToAll(GameMessage_Player_Set_Team(playerId, team));
}

void LuaServerPlayer::SetColor(unsigned colorOrIdx)
{
    if(GetAlpha(colorOrIdx) == 0)
    {
        lua::assertTrue(colorOrIdx < PLAYER_COLORS.size(), "Invalid color");
        player.color = PLAYER_COLORS[colorOrIdx];
    } else
        player.color = colorOrIdx;
    gameServer_.SendToAll(GameMessage_Player_Set_Color(playerId, player.color));
}

void LuaServerPlayer::Close()
{
    if(player.ps == PS_LOCKED)
        return;
    if(player.ps == PS_OCCUPIED)
        gameServer_.KickPlayer(playerId);
    player.ps = PS_LOCKED;
    player.isReady = false;

    gameServer_.SendToAll(GameMessage_Player_Set_State(playerId, player.ps, player.aiInfo));
    gameServer_.AnnounceStatusChange();
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
    if(player.ps == PS_OCCUPIED)
        gameServer_.KickPlayer(playerId);
    bool wasUsed = player.isUsed();
    player.ps = PS_AI;
    player.aiInfo = info;
    player.isReady = true;
    player.SetAIName(playerId);
    gameServer_.SendToAll(GameMessage_Player_Set_State(playerId, player.ps, player.aiInfo));
    // If we added a new AI, set an initial color
    // Do this after(!) the player state was set
    if(!wasUsed)
        gameServer_.CheckAndSetColor(playerId, player.color);
    gameServer_.AnnounceStatusChange();
}
