// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "LuaPlayerBase.h"
#include "BasePlayerInfo.h"
#include "RTTR_Assert.h"
#include <kaguya/kaguya.hpp>

void LuaPlayerBase::Register(kaguya::State& state)
{
    state["PlayerBase"].setClass(kaguya::UserdataMetatable<LuaPlayerBase>()
                                   .addFunction("GetName", &LuaPlayerBase::GetName)
                                   .addFunction("GetNation", &LuaPlayerBase::GetNation)
                                   .addFunction("GetTeam", &LuaPlayerBase::GetTeam)
                                   .addFunction("GetColor", &LuaPlayerBase::GetColor)
                                   .addFunction("IsFree", &LuaPlayerBase::IsFree)
                                   .addFunction("IsHuman", &LuaPlayerBase::IsHuman)
                                   .addFunction("IsAI", &LuaPlayerBase::IsAI)
                                   .addFunction("IsClosed", &LuaPlayerBase::IsClosed)
                                   .addFunction("GetAILevel", &LuaPlayerBase::GetAILevel));

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
    int colorIdx = GetPlayer().GetColorIdx();
    if(colorIdx < 0)
        return GetPlayer().color;
    else
        return static_cast<unsigned>(colorIdx);
}

bool LuaPlayerBase::IsHuman() const
{
    return GetPlayer().ps == PS_OCCUPIED;
}

bool LuaPlayerBase::IsAI() const
{
    return GetPlayer().ps == PS_AI;
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
    if(GetPlayer().ps != PS_AI)
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
