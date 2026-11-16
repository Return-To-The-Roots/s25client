// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LuaInterfaceGameBase.h"
#include <string>
#include <vector>

class LuaServerPlayer;
class IGameLobbyController;
class ILocalGameState;
enum class AddonId;
struct AddonIdWrapper;

class LuaInterfaceSettings : public LuaInterfaceGameBase
{
public:
    LuaInterfaceSettings(IGameLobbyController& lobbyServerController, const ILocalGameState& localGameState);
    virtual ~LuaInterfaceSettings();

    static void Register(kaguya::State& state);

    // Interface for C++
    /// Called when settings are entered. Returns whether script should be executed
    bool eventSettingsInit(bool isSinglePlayer, bool isSavegame);
    // Other events are only called for the host
    /// Called when the settings screen is ready
    void eventSettingsReady();
    void eventPlayerJoined(unsigned playerIdx);
    void eventPlayerLeft(unsigned playerIdx);
    void eventPlayerReady(unsigned playerIdx);
    /// Return whether the named change is allowed
    bool isChangeAllowed(const std::string& name, bool defaultVal = false);
    /// Get addons that are allowed to be changed
    std::vector<AddonId> getAllowedAddons();
    bool isMapPreviewEnabled();

private:
    IGameLobbyController& lobbyServerController_;
    kaguya::LuaRef getAllowedChanges();

    // Callable from Lua
    unsigned getNumPlayers() const;
    LuaServerPlayer getPlayer(int idx);
    void setAddon(AddonIdWrapper id, unsigned value);
    void setBoolAddon(AddonIdWrapper id, bool value); // Alias
    void resetAddons();
    void resetGameSettings();
    void setGameSettings(const kaguya::LuaTable& settings);
};
