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
#include "LuaInterfaceSettings.h"
#include "GameServer.h"
#include "LuaServerPlayer.h"
#include "addons/const_addons.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

inline void check(bool testValue, const std::string& error)
{
    if(!testValue)
        throw std::runtime_error(error);
}

LuaInterfaceSettings::LuaInterfaceSettings()
{
    Register(lua);
    lua["rttr"] = this;
}

LuaInterfaceSettings::~LuaInterfaceSettings()
{}

void LuaInterfaceSettings::Register(kaguya::State& state)
{
    state["RTTRSettings"].setClass(kaguya::ClassMetatable<LuaInterfaceSettings, LuaInterfaceBase>()
        .addMemberFunction("GetPlayerCount", &LuaInterfaceSettings::GetPlayerCount)
        .addMemberFunction("GetPlayer", &LuaInterfaceSettings::GetPlayer)
        );

    for(unsigned i = 0; i < AddonId::count_; ++i)
    {
        AddonId id = AddonId::type_(AddonId::values_()[i]);
        state[std::string("ADDON_") + id.toString()] = id;
    }
}

unsigned LuaInterfaceSettings::GetPlayerCount()
{
    return GAMESERVER.GetMaxPlayerCount();
}

LuaServerPlayer LuaInterfaceSettings::GetPlayer(unsigned idx)
{
    check(idx < GetPlayerCount(), "Invalid player idx");
    return LuaServerPlayer(idx);
}

void LuaInterfaceSettings::SetAddon(AddonId id, unsigned value)
{
    check(unsigned(id) < AddonId::count_, "Invalid addon id");
    GlobalGameSettings ggs = GAMESERVER.GetGGS();
    ggs.setSelection(id, value);
    GAMESERVER.ChangeGlobalGameSettings(ggs);
}

kaguya::LuaRef LuaInterfaceSettings::GetAllowedChanges()
{
    kaguya::LuaRef getGeneralConfig = lua["getAllowedChanges"];
    if(getGeneralConfig.type() == LUA_TFUNCTION)
    {
        kaguya::LuaRef cfg = getGeneralConfig.call<kaguya::LuaRef>();
        if(cfg.type() == LUA_TTABLE)
            return cfg;
    }
    return kaguya::LuaRef();
}

bool LuaInterfaceSettings::IsChangeAllowed(const std::string& name, const bool defaultVal/* = false*/)
{
    kaguya::LuaRef cfg = GetAllowedChanges();
    if(cfg.isNilref())
        return defaultVal;
    cfg = cfg.getField(name);
    if(cfg.typeTest<bool>())
        return cfg;
    else
        return defaultVal;
}

std::vector<unsigned> LuaInterfaceSettings::GetAllowedAddons()
{
    kaguya::LuaRef getAllowedAddons = lua["getAllowedAddons"];
    if(getAllowedAddons.type() == LUA_TFUNCTION)
        return getAllowedAddons();
    return std::vector<unsigned>();
}
