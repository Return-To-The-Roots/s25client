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

#include "LuaInterfaceSettings.h"
#include "GlobalGameSettings.h"
#include "LuaServerPlayer.h"
#include "addons/const_addons.h"
#include "lua/LuaHelpers.h"
#include "network/IGameLobbyController.h"
#include "gameTypes/GameSettingTypes.h"
#include "s25util/Log.h"

/// Wrapper used to make sure LUA can only return on of the predefined values
struct AddonIdWrapper
{
    AddonId value;
    constexpr operator AddonId() const { return value; }
};

LuaInterfaceSettings::LuaInterfaceSettings(IGameLobbyController& lobbyServerController, const ILocalGameState& localGameState)
    : LuaInterfaceGameBase(localGameState), lobbyServerController_(lobbyServerController)
{
    Register(lua);
    LuaServerPlayer::Register(lua);
    lua["rttr"] = this;
}

LuaInterfaceSettings::~LuaInterfaceSettings() = default;

void LuaInterfaceSettings::Register(kaguya::State& state)
{
    state["RTTRSettings"].setClass(
      kaguya::UserdataMetatable<LuaInterfaceSettings, LuaInterfaceGameBase>()
        .addFunction("GetNumPlayers", &LuaInterfaceSettings::GetNumPlayers)
        .addFunction("GetPlayer", &LuaInterfaceSettings::GetPlayer)
        .addOverloadedFunctions("SetAddon", &LuaInterfaceSettings::SetAddon, &LuaInterfaceSettings::SetBoolAddon)
        .addFunction("ResetAddons", &LuaInterfaceSettings::ResetAddons)
        .addFunction("ResetGameSettings", &LuaInterfaceSettings::ResetGameSettings)
        .addFunction("SetGameSettings", &LuaInterfaceSettings::SetGameSettings)
        // Old name
        .addFunction("GetPlayerCount", &LuaInterfaceSettings::GetNumPlayers));

    state["AddonId"].setClass(kaguya::UserdataMetatable<AddonIdWrapper>());

    for(AddonId id : rttrEnum::EnumData<AddonId>::values)
    {
        state[std::string("ADDON_") + rttrEnum::toString(id)] = AddonIdWrapper{id};
    }

#pragma region ConstDefs
#define ADD_LUA_CONST(name) state[#name] = name

    ADD_LUA_CONST(GS_VERYSLOW);
    ADD_LUA_CONST(GS_SLOW);
    ADD_LUA_CONST(GS_NORMAL);
    ADD_LUA_CONST(GS_FAST);
    ADD_LUA_CONST(GS_VERYFAST);

    ADD_LUA_CONST(GO_NONE);
    ADD_LUA_CONST(GO_CONQUER3_4);
    ADD_LUA_CONST(GO_TOTALDOMINATION);

    ADD_LUA_CONST(SWR_VLOW);
    ADD_LUA_CONST(SWR_LOW);
    ADD_LUA_CONST(SWR_NORMAL);
    ADD_LUA_CONST(SWR_ALOT);

    ADD_LUA_CONST(EXP_DISABLED);
    ADD_LUA_CONST(EXP_CLASSIC);
    ADD_LUA_CONST(EXP_FOGOFWAR);
    ADD_LUA_CONST(EXP_FOGOFWARE_EXPLORED);

#undef ADD_LUA_CONST
#pragma endregion ConstDefs
}

unsigned LuaInterfaceSettings::GetNumPlayers() const
{
    return lobbyServerController_.GetMaxNumPlayers();
}

LuaServerPlayer LuaInterfaceSettings::GetPlayer(int idx)
{
    lua::assertTrue(idx >= 0 && static_cast<unsigned>(idx) < GetNumPlayers(), "Invalid player idx");
    return LuaServerPlayer(lobbyServerController_, static_cast<unsigned>(idx));
}

void LuaInterfaceSettings::SetAddon(AddonIdWrapper id, unsigned value)
{
    GlobalGameSettings ggs = lobbyServerController_.GetGGS();
    ggs.setSelection(id, value);
    lobbyServerController_.ChangeGlobalGameSettings(ggs);
}

void LuaInterfaceSettings::SetBoolAddon(AddonIdWrapper id, bool value)
{
    SetAddon(id, value ? 1 : 0);
}

void LuaInterfaceSettings::ResetAddons()
{
    GlobalGameSettings ggs = lobbyServerController_.GetGGS();
    ggs.resetAddons();
    lobbyServerController_.ChangeGlobalGameSettings(ggs);
}

void LuaInterfaceSettings::ResetGameSettings()
{
    // Simply create a new instance
    lobbyServerController_.ChangeGlobalGameSettings(GlobalGameSettings());
}

void LuaInterfaceSettings::SetGameSettings(const kaguya::LuaTable& settings)
{
    GlobalGameSettings ggs = lobbyServerController_.GetGGS();
    std::vector<std::string> keys = settings.keys<std::string>();

    if(std::find(keys.begin(), keys.end(), "speed") != keys.end())
    {
        unsigned speed = settings.getField("speed");
        lua::assertTrue(speed <= GS_VERYFAST, "Speed is invalid");
        ggs.speed = GameSpeed(speed);
    }

    if(std::find(keys.begin(), keys.end(), "objective") != keys.end())
    {
        unsigned objective = settings.getField("objective");
        lua::assertTrue(objective <= GO_TOTALDOMINATION, "Objective is invalid");
        ggs.objective = GameObjective(objective);
    }

    if(std::find(keys.begin(), keys.end(), "startWares") != keys.end())
    {
        unsigned wares = settings.getField("startWares");
        lua::assertTrue(wares <= SWR_ALOT, "Start wares is invalid");
        ggs.startWares = StartWares(wares);
    }

    if(std::find(keys.begin(), keys.end(), "fow") != keys.end())
    {
        unsigned fow = settings.getField("fow");
        lua::assertTrue(fow <= EXP_FOGOFWARE_EXPLORED, "FoW is invalid");
        ggs.exploration = Exploration(fow);
    }

    if(std::find(keys.begin(), keys.end(), "lockedTeams") != keys.end())
        ggs.lockedTeams = settings.getField("lockedTeams");

    if(std::find(keys.begin(), keys.end(), "teamView") != keys.end())
        ggs.teamView = settings.getField("teamView");

    if(std::find(keys.begin(), keys.end(), "randomStartPosition") != keys.end())
        ggs.randomStartPosition = settings.getField("randomStartPosition");

    lobbyServerController_.ChangeGlobalGameSettings(ggs);
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

bool LuaInterfaceSettings::EventSettingsInit(bool isSinglePlayer, bool isSavegame)
{
    kaguya::LuaRef func = lua["onSettingsInit"];
    if(func.type() == LUA_TFUNCTION)
        return func.call<bool>(isSinglePlayer, isSavegame);
    else
        return true;
}

void LuaInterfaceSettings::EventSettingsReady()
{
    kaguya::LuaRef func = lua["onSettingsReady"];
    if(func.type() == LUA_TFUNCTION)
        func.call<void>();
}

void LuaInterfaceSettings::EventPlayerJoined(unsigned playerIdx)
{
    kaguya::LuaRef func = lua["onPlayerJoined"];
    if(func.type() == LUA_TFUNCTION)
        func.call<void>(playerIdx);
}

void LuaInterfaceSettings::EventPlayerLeft(unsigned playerIdx)
{
    kaguya::LuaRef func = lua["onPlayerLeft"];
    if(func.type() == LUA_TFUNCTION)
        func.call<void>(playerIdx);
}

void LuaInterfaceSettings::EventPlayerReady(unsigned playerIdx)
{
    kaguya::LuaRef func = lua["onPlayerReady"];
    if(func.type() == LUA_TFUNCTION)
        func.call<void>(playerIdx);
}

bool LuaInterfaceSettings::IsChangeAllowed(const std::string& name, const bool defaultVal /* = false*/)
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

std::vector<AddonId> LuaInterfaceSettings::GetAllowedAddons()
{
    kaguya::LuaRef getAllowedAddons = lua["getAllowedAddons"];
    if(getAllowedAddons.type() == LUA_TFUNCTION)
    {
        kaguya::LuaRef addons = getAllowedAddons();
        if(addons.typeTest<std::vector<AddonIdWrapper>>())
        {
            const std::vector<AddonIdWrapper> wrappers = addons;
            return {wrappers.begin(), wrappers.end()};
        } else
            LOG.write("Invalid type returned by getAllowedAddons");
    }
    return std::vector<AddonId>();
}
