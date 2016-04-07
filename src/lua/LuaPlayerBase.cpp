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
#include "LuaPlayerBase.h"
#include "GamePlayerInfo.h"
#include <stdexcept>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

inline void check(bool testValue, const std::string& error)
{
    if(!testValue)
        throw std::runtime_error(error);
}

void LuaPlayerBase::Register(kaguya::State& state)
{
    state["PlayerBase"].setClass(kaguya::ClassMetatable<LuaPlayerBase>()
        .addMemberFunction("GetName", &LuaPlayerBase::GetName)
        .addMemberFunction("GetNation", &LuaPlayerBase::GetNation)
        .addMemberFunction("GetTeam", &LuaPlayerBase::GetTeam)
        .addMemberFunction("GetColor", &LuaPlayerBase::GetColor)
        .addMemberFunction("IsFree", &LuaPlayerBase::IsFree)
        .addMemberFunction("IsHuman", &LuaPlayerBase::IsHuman)
        .addMemberFunction("IsAI", &LuaPlayerBase::IsAI)
        .addMemberFunction("IsClosed", &LuaPlayerBase::IsClosed)
        .addMemberFunction("GetAILevel", &LuaPlayerBase::GetAILevel)
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

std::string LuaPlayerBase::GetName() const
{
    return GetPlayer().name;
}

Nation LuaPlayerBase::GetNation() const
{
    return GetPlayer().nation;
}

Team LuaPlayerBase::GetTeam() const
{
    return GetPlayer().team;
}

unsigned LuaPlayerBase::GetColor() const
{
    return GetPlayer().color;
}

bool LuaPlayerBase::IsHuman() const
{
    return GetPlayer().ps == PS_OCCUPIED;
}

bool LuaPlayerBase::IsAI() const
{
    return GetPlayer().ps == PS_KI;
}

bool LuaPlayerBase::IsClosed() const
{
    return GetPlayer().ps == PS_LOCKED;
}

bool LuaPlayerBase::IsFree() const
{
    return GetPlayer().ps == PS_FREE;
}

int LuaPlayerBase::GetAILevel() const
{
    if(GetPlayer().ps != PS_KI)
        return -1;
    if(GetPlayer().aiInfo.type == AI::DUMMY)
        return 0;
    switch(GetPlayer().aiInfo.level)
    {
    case AI::EASY: return 1;
    case AI::MEDIUM: return 2;
    case AI::HARD: return 3;
    }
    RTTR_Assert(false);
    return -1;
}
