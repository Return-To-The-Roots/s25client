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
    constexpr operator AddonId() const noexcept { return value; }
};

LuaInterfaceSettings::LuaInterfaceSettings(IGameLobbyController& lobbyServerController,
                                           const ILocalGameState& localGameState)
    : LuaInterfaceGameBase(localGameState), lobbyServerController_(lobbyServerController)
{
    registerClasses(lua);
    LuaServerPlayer::registerClasses(lua);
    lua["rttr"] = this;
}

LuaInterfaceSettings::~LuaInterfaceSettings() = default;

void LuaInterfaceSettings::registerClasses(kaguya::State& state)
{
    state["RTTRSettings"].setClass(
      kaguya::UserdataMetatable<LuaInterfaceSettings, LuaInterfaceGameBase>()
        .addFunction("GetNumPlayers", &LuaInterfaceSettings::getNumPlayers)
        .addFunction("GetPlayer", &LuaInterfaceSettings::getPlayer)
        .addOverloadedFunctions("SetAddon", &LuaInterfaceSettings::setAddon, &LuaInterfaceSettings::setBoolAddon)
        .addFunction("ResetAddons", &LuaInterfaceSettings::resetAddons)
        .addFunction("ResetGameSettings", &LuaInterfaceSettings::resetGameSettings)
        .addFunction("SetGameSettings", &LuaInterfaceSettings::setGameSettings)
        // Old name
        .addFunction("GetPlayerCount", &LuaInterfaceSettings::getNumPlayers));

    state["AddonId"].setClass(kaguya::UserdataMetatable<AddonIdWrapper>());

    for(AddonId id : rttrEnum::EnumData<AddonId>::values)
    {
        state[std::string("ADDON_") + rttrEnum::toString(id)] = AddonIdWrapper{id};
    }
    // Legacy alias: the bool addon was replaced by a list addon reusing the same id, where the old value 1 still means
    // "inexhaustible". Keeps existing map scripts working; may be removed once INEXHAUSTIBLE_MINES is dropped too.
    state["ADDON_INEXHAUSTIBLE_GRANITEMINES"] = AddonIdWrapper{AddonId::GRANITEMINE_RESOURCE_BEHAVIOR};

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

unsigned LuaInterfaceSettings::getNumPlayers() const
{
    return lobbyServerController_.getMaxNumPlayers();
}

LuaServerPlayer LuaInterfaceSettings::getPlayer(int idx)
{
    lua::assertTrue(idx >= 0 && static_cast<unsigned>(idx) < getNumPlayers(), "Invalid player idx");
    return LuaServerPlayer(lobbyServerController_, static_cast<unsigned>(idx));
}

void LuaInterfaceSettings::setAddon(AddonIdWrapper id, unsigned value)
{
    GlobalGameSettings ggs = lobbyServerController_.getGGS();
    // The global "inexhaustible mines" addon was replaced by per-mine-type settings and is no longer registered.
    // Apply the same migration used for old settings and savegames instead of reporting an unknown addon.
    if(static_cast<AddonId>(id) == AddonId::INEXHAUSTIBLE_MINES)
    {
        if(value != 0)
            ggs.applyLegacyInexhaustibleMines();
    } else
        ggs.setSelection(id, value);
    lobbyServerController_.changeGlobalGameSettings(ggs);
}

void LuaInterfaceSettings::setBoolAddon(AddonIdWrapper id, bool value)
{
    setAddon(id, value ? 1 : 0);
}

void LuaInterfaceSettings::resetAddons()
{
    GlobalGameSettings ggs = lobbyServerController_.getGGS();
    ggs.resetAddons();
    lobbyServerController_.changeGlobalGameSettings(ggs);
}

void LuaInterfaceSettings::resetGameSettings()
{
    // Simply create a new instance
    lobbyServerController_.changeGlobalGameSettings(GlobalGameSettings());
}

void LuaInterfaceSettings::setGameSettings(const kaguya::LuaTable& settings)
{
    GlobalGameSettings ggs = lobbyServerController_.getGGS();
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

    lobbyServerController_.changeGlobalGameSettings(ggs);
}

kaguya::LuaRef LuaInterfaceSettings::getAllowedChanges()
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

bool LuaInterfaceSettings::eventSettingsInit(bool isSinglePlayer, bool isSavegame)
{
    kaguya::LuaRef func = lua["onSettingsInit"];
    if(func.type() == LUA_TFUNCTION)
        return func.call<bool>(isSinglePlayer, isSavegame);
    else
        return true;
}

void LuaInterfaceSettings::eventSettingsReady()
{
    kaguya::LuaRef func = lua["onSettingsReady"];
    if(func.type() == LUA_TFUNCTION)
        func.call<void>();
}

void LuaInterfaceSettings::eventPlayerJoined(unsigned playerIdx)
{
    kaguya::LuaRef func = lua["onPlayerJoined"];
    if(func.type() == LUA_TFUNCTION)
        func.call<void>(playerIdx);
}

void LuaInterfaceSettings::eventPlayerLeft(unsigned playerIdx)
{
    kaguya::LuaRef func = lua["onPlayerLeft"];
    if(func.type() == LUA_TFUNCTION)
        func.call<void>(playerIdx);
}

void LuaInterfaceSettings::eventPlayerReady(unsigned playerIdx)
{
    kaguya::LuaRef func = lua["onPlayerReady"];
    if(func.type() == LUA_TFUNCTION)
        func.call<void>(playerIdx);
}

bool LuaInterfaceSettings::isChangeAllowed(const std::string& name, const bool defaultVal /* = false*/)
{
    kaguya::LuaRef cfg = getAllowedChanges();
    if(cfg.isNilref())
        return defaultVal;
    cfg = cfg.getField(name);
    if(cfg.typeTest<bool>())
        return cfg;
    else
        return defaultVal;
}

std::vector<AddonId> LuaInterfaceSettings::getAllowedAddons()
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

bool LuaInterfaceSettings::isMapPreviewEnabled()
{
    kaguya::LuaRef func = lua["isMapPreviewEnabled"];
    if(func.type() == LUA_TFUNCTION)
    {
        return func.call<bool>();
    }
    return true;
}
