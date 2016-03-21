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

LuaServerPlayer::LuaServerPlayer(unsigned playerIdx): player(*GAMESERVER.players.getElement(playerIdx))
{}

void LuaServerPlayer::Register(kaguya::State& state)
{
    state["Player"].setClass(kaguya::ClassMetatable<LuaServerPlayer>()
        .addMemberFunction("GetNation", &LuaServerPlayer::GetNation)
        .addMemberFunction("SetNation", &LuaServerPlayer::SetNation)
        .addMemberFunction("GetTeam", &LuaServerPlayer::GetTeam)
        .addMemberFunction("SetTeam", &LuaServerPlayer::SetTeam)
        .addMemberFunction("GetColor", &LuaServerPlayer::GetColor)
        .addMemberFunction("SetColor", &LuaServerPlayer::SetColor)
        .addMemberFunction("IsHuman", &LuaServerPlayer::IsHuman)
        .addMemberFunction("IsAI", &LuaServerPlayer::IsAI)
        .addMemberFunction("IsClosed", &LuaServerPlayer::IsClosed)
        .addMemberFunction("Close", &LuaServerPlayer::Close)
        .addMemberFunction("GetAILevel", &LuaServerPlayer::GetAILevel)
        .addMemberFunction("SetAI", &LuaServerPlayer::SetAI)
        );

#pragma region ConstDefs
#define ADD_LUA_CONST(name) state[#name] = name

    ADD_LUA_CONST(NAT_AFRICANS);
    ADD_LUA_CONST(NAT_JAPANESE);
    ADD_LUA_CONST(NAT_ROMANS);
    ADD_LUA_CONST(NAT_VIKINGS);
    ADD_LUA_CONST(NAT_BABYLONIANS);

    ADD_LUA_CONST(TM_NOTEAM);
    ADD_LUA_CONST(TM_RANDOMTEAM);
    ADD_LUA_CONST(TM_RANDOMTEAM2);
    ADD_LUA_CONST(TM_RANDOMTEAM3);
    ADD_LUA_CONST(TM_RANDOMTEAM4);
    ADD_LUA_CONST(TM_TEAM1);
    ADD_LUA_CONST(TM_TEAM2);
    ADD_LUA_CONST(TM_TEAM3);
    ADD_LUA_CONST(TM_TEAM4);

#undef ADD_LUA_CONST
#pragma endregion ConstDefs
}

Nation LuaServerPlayer::GetNation() const
{
    return player.nation;
}

void LuaServerPlayer::SetNation(Nation nat)
{
    check(unsigned(nat) < NAT_COUNT, "Invalid Nation");
    GAMESERVER.OnNMSPlayerToggleNation(GameMessage_Player_Toggle_Nation(player.getPlayerID(), nat));
}

Team LuaServerPlayer::GetTeam() const
{
    return player.team;
}

void LuaServerPlayer::SetTeam(Team team)
{
    check(unsigned(team) < TEAM_COUNT, "Invalid team");
    GAMESERVER.OnNMSPlayerToggleTeam(GameMessage_Player_Toggle_Team(player.getPlayerID(), team));
}

void LuaServerPlayer::SetColor(unsigned colorIdx)
{
    check(colorIdx < PLAYER_COLORS_COUNT, "Invalid color");
    player.color = colorIdx;
    GAMESERVER.SendToAll(GameMessage_Player_Toggle_Color(player.getPlayerID(), colorIdx));
}

bool LuaServerPlayer::IsHuman() const
{
    return player.ps == PS_OCCUPIED;
}

bool LuaServerPlayer::IsAI() const
{
    return player.ps == PS_KI;
}

bool LuaServerPlayer::IsClosed() const
{
    return player.ps == PS_LOCKED;
}

unsigned LuaServerPlayer::GetColor() const
{
    return player.color;
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

int LuaServerPlayer::GetAILevel() const
{
    if(player.ps != PS_KI)
        return -1;
    if(player.aiInfo.type == AI::DUMMY)
        return 0;
    switch(player.aiInfo.level)
    {
    case AI::EASY: return 1;
    case AI::MEDIUM: return 2;
    case AI::HARD: return 3;
    }
    RTTR_Assert(false);
    return -1;
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
    player.ps = PS_KI;
    player.aiInfo = info;
    player.ready = true;
    GAMESERVER.SetAIName(player.getPlayerID());
    GAMESERVER.SendToAll(GameMessage_Player_Set_State(player.getPlayerID(), player.ps, player.aiInfo));
    GAMESERVER.AnnounceStatusChange();
}
