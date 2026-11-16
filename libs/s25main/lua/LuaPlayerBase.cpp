// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaPlayerBase.h"
#include "BasePlayerInfo.h"
#include "RTTR_Assert.h"
#include "luaRegistrationHelpers.h"
#include <kaguya/kaguya.hpp>

void LuaPlayerBase::Register(kaguya::State& state)
{
    state["PlayerBase"].setClass(kaguya::UserdataMetatable<LuaPlayerBase>()
                                   .addFunction("GetName", &LuaPlayerBase::getName)
                                   .addFunction("GetNation", &LuaPlayerBase::getNation)
                                   .addFunction("GetTeam", &LuaPlayerBase::getTeam)
                                   .addFunction("GetColor", &LuaPlayerBase::getColor)
                                   .addFunction("IsFree", &LuaPlayerBase::isFree)
                                   .addFunction("IsHuman", &LuaPlayerBase::isHuman)
                                   .addFunction("IsAI", &LuaPlayerBase::isAI)
                                   .addFunction("IsClosed", &LuaPlayerBase::isClosed)
                                   .addFunction("GetAILevel", &LuaPlayerBase::getAILevel));

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

std::string LuaPlayerBase::getName() const
{
    return getPlayer().name;
}

Nation LuaPlayerBase::getNation() const
{
    return getPlayer().nation;
}

Team LuaPlayerBase::getTeam() const
{
    return getPlayer().team;
}

unsigned LuaPlayerBase::getColor() const
{
    int colorIdx = getPlayer().getColorIdx();
    if(colorIdx < 0)
        return getPlayer().color;
    else
        return static_cast<unsigned>(colorIdx);
}

bool LuaPlayerBase::isHuman() const
{
    return getPlayer().ps == PlayerState::Occupied;
}

bool LuaPlayerBase::isAI() const
{
    return getPlayer().ps == PlayerState::AI;
}

bool LuaPlayerBase::isClosed() const
{
    return getPlayer().ps == PlayerState::Locked;
}

bool LuaPlayerBase::isFree() const
{
    return getPlayer().ps == PlayerState::Free;
}

int LuaPlayerBase::getAILevel() const
{
    if(getPlayer().ps != PlayerState::AI)
        return -1;
    if(getPlayer().aiInfo.type == AI::Type::Dummy)
        return 0;
    switch(getPlayer().aiInfo.level)
    {
        case AI::Level::Easy: return 1;
        case AI::Level::Medium: return 2;
        case AI::Level::Hard: return 3;
    }
    RTTR_Assert(false);
    return -1;
}
