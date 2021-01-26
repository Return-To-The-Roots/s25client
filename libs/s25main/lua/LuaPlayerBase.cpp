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
#include "luaRegistrationHelpers.h"
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

    RTTR_LUA_REGISTER_ENUM(Nation, "NAT_", Africans, Japanese, Romans, Vikings, Babylonians);
    RTTR_LUA_REGISTER_ENUM(Team, "TM_", None, Random, Team1, Team2, Team3, Team4, Random1To2, Random1To3, Random1To4);

    // Legacy variables
    state["TM_NOTEAM"] = Team::None;
    state["TM_RANDOMTEAM"] = Team::Team1;
    state["TM_RANDOMTEAM2"] = Team::Team2;
    state["TM_RANDOMTEAM3"] = Team::Team3;
    state["TM_RANDOMTEAM4"] = Team::Team4;
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
    return GetPlayer().ps == PlayerState::Occupied;
}

bool LuaPlayerBase::IsAI() const
{
    return GetPlayer().ps == PlayerState::AI;
}

bool LuaPlayerBase::IsClosed() const
{
    return GetPlayer().ps == PlayerState::Locked;
}

bool LuaPlayerBase::IsFree() const
{
    return GetPlayer().ps == PlayerState::Free;
}

int LuaPlayerBase::GetAILevel() const
{
    if(GetPlayer().ps != PlayerState::AI)
        return -1;
    if(GetPlayer().aiInfo.type == AI::Type::Dummy)
        return 0;
    switch(GetPlayer().aiInfo.level)
    {
        case AI::Level::Easy: return 1;
        case AI::Level::Medium: return 2;
        case AI::Level::Hard: return 3;
    }
    RTTR_Assert(false);
    return -1;
}
