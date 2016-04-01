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
#include "GameServer.h"
#include "GameServerPlayer.h"
#include "GameMessages.h"
#include "helpers/converters.h"
#include "libutil/src/Log.h"
#include "libutil/src/colors.h"
#include <stdexcept>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

inline void check(bool testValue, const std::string& error)
{
    if(!testValue)
        throw std::runtime_error(error);
}

const GamePlayerInfo& LuaServerPlayer::GetPlayer() const
{
    return player;
}

LuaServerPlayer::LuaServerPlayer(unsigned playerIdx): player(*GAMESERVER.players.getElement(playerIdx))
{}

void LuaServerPlayer::Register(kaguya::State& state)
{
    LuaPlayerBase::Register(state);
    state["Player"].setClass(kaguya::ClassMetatable<LuaServerPlayer, LuaPlayerBase>()
        .addMemberFunction("SetNation", &LuaServerPlayer::SetNation)
        .addMemberFunction("SetTeam", &LuaServerPlayer::SetTeam)
        .addMemberFunction("SetColor", &LuaServerPlayer::SetColor)
        .addMemberFunction("Close", &LuaServerPlayer::Close)
        .addMemberFunction("SetAI", &LuaServerPlayer::SetAI)
        );
}

void LuaServerPlayer::SetNation(Nation nat)
{
    check(unsigned(nat) < NAT_COUNT, "Invalid Nation");
    player.nation = nat;
    GAMESERVER.SendToAll(GameMessage_Player_Set_Nation(player.getPlayerID(), nat));
}

void LuaServerPlayer::SetTeam(Team team)
{
    check(unsigned(team) < TEAM_COUNT, "Invalid team");
    player.team = team;
    GAMESERVER.SendToAll(GameMessage_Player_Set_Team(player.getPlayerID(), team));
}

void LuaServerPlayer::SetColor(unsigned colorOrIdx)
{
    if(GetAlpha(colorOrIdx) == 0)
    {
        check(colorOrIdx < PLAYER_COLORS.size(), "Invalid color");
        player.color = PLAYER_COLORS[colorOrIdx];
    } else
        player.color = colorOrIdx;
    GAMESERVER.SendToAll(GameMessage_Player_Set_Color(player.getPlayerID(), player.color));
}

void LuaServerPlayer::Close()
{
    if(player.ps == PS_LOCKED)
        return;
    if(player.ps == PS_OCCUPIED)
    {
        GAMESERVER.KickPlayer(player.getPlayerID(), NP_NOCAUSE, 0);
        return;
    }
    player.ps = PS_LOCKED;
    player.ready = false;

    GAMESERVER.SendToAll(GameMessage_Player_Set_State(player.getPlayerID(), player.ps, player.aiInfo));
    GAMESERVER.AnnounceStatusChange();
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
    default: check(false, "Invalid AI level");
    }
    if(player.ps == PS_OCCUPIED)
        GAMESERVER.KickPlayer(player.getPlayerID(), NP_NOCAUSE, 0);
    bool wasUsed = player.isUsed();
    player.ps = PS_KI;
    player.aiInfo = info;
    player.ready = true;
    GAMESERVER.SetAIName(player.getPlayerID());
    GAMESERVER.SendToAll(GameMessage_Player_Set_State(player.getPlayerID(), player.ps, player.aiInfo));
    // If we added a new AI, set an initial color
    // Do this after(!) the player state was set
    if(!wasUsed)
        GAMESERVER.CheckAndSetColor(player.getPlayerID(), player.color);
    GAMESERVER.AnnounceStatusChange();
}
