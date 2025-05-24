// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaInterfaceSettings.h"
#include "GlobalGameSettings.h"
#include "LuaServerPlayer.h"
#include "addons/const_addons.h"
#include "helpers/containerUtils.h"
#include "lua/LuaHelpers.h"
#include "lua/SafeEnum.h"
#include "network/IGameLobbyController.h"
#include "gameTypes/GameSettingTypes.h"
#include "s25util/Log.h"
#include "s25util/strAlgos.h"

/// Wrapper used to make sure LUA can only return on of the predefined values
struct AddonIdWrapper
{
    AddonId value;
    constexpr operator AddonId() const { return value; }
};

LuaInterfaceSettings::LuaInterfaceSettings(IGameLobbyController& lobbyServerController,
                                           const ILocalGameState& localGameState)
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
#define ADD_LUA_CONST(name) state["GS_" + s25util::toUpper(#name)] = GameSpeed::name
    ADD_LUA_CONST(VerySlow);
    ADD_LUA_CONST(Slow);
    ADD_LUA_CONST(Normal);
    ADD_LUA_CONST(Fast);
    ADD_LUA_CONST(VeryFast);
#undef ADD_LUA_CONST

#define ADD_LUA_CONST(name) state["GO_" + s25util::toUpper(#name)] = GameObjective::name
    ADD_LUA_CONST(None);
    ADD_LUA_CONST(Conquer3_4);
    ADD_LUA_CONST(TotalDomination);
#undef ADD_LUA_CONST

#define ADD_LUA_CONST(name) state["SWR_" + s25util::toUpper(#name)] = StartWares::name
    ADD_LUA_CONST(VLow);
    ADD_LUA_CONST(Low);
    ADD_LUA_CONST(Normal);
    ADD_LUA_CONST(ALot);
#undef ADD_LUA_CONST

#define ADD_LUA_CONST(name) state["EXP_" + s25util::toUpper(#name)] = Exploration::name
    ADD_LUA_CONST(Disabled);
    ADD_LUA_CONST(Classic);
    ADD_LUA_CONST(FogOfWar);
    ADD_LUA_CONST(FogOfWarExplored);
    state["EXP_FOGOFWARE_EXPLORED"] = Exploration::FogOfWarExplored; // Legacy alias (mind the typo)
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

    if(helpers::contains(keys, "speed"))
    {
        const lua::SafeEnum<GameSpeed> speed = settings.getField("speed");
        ggs.speed = speed;
    }

    if(helpers::contains(keys, "objective"))
    {
        const lua::SafeEnum<GameObjective> objective = settings.getField("objective");
        ggs.objective = objective;
    }

    if(helpers::contains(keys, "startWares"))
    {
        const lua::SafeEnum<StartWares> wares = settings.getField("startWares");
        ggs.startWares = wares;
    }

    if(helpers::contains(keys, "fow"))
    {
        const lua::SafeEnum<Exploration> fow = settings.getField("fow");
        ggs.exploration = fow;
    }

    if(helpers::contains(keys, "lockedTeams"))
        ggs.lockedTeams = settings.getField("lockedTeams");

    if(helpers::contains(keys, "teamView"))
        ggs.teamView = settings.getField("teamView");

    if(helpers::contains(keys, "randomStartPosition"))
        ggs.randomStartPosition = settings.getField("randomStartPosition");

    lobbyServerController_.ChangeGlobalGameSettings(ggs);
}

kaguya::LuaRef LuaInterfaceSettings::GetAllowedChanges()
{
    kaguya::LuaRef getGeneralConfig = lua["getAllowedChanges"];
    if(getGeneralConfig.type() == LUA_TFUNCTION)
    {
        auto cfg = getGeneralConfig.call<kaguya::LuaRef>();
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

bool LuaInterfaceSettings::IsMapPreviewEnabled()
{
    kaguya::LuaRef func = lua["isMapPreviewEnabled"];
    if(func.type() == LUA_TFUNCTION)
    {
        return func.call<bool>();
    }
    return true;
}
