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
    bool EventSettingsInit(bool isSinglePlayer, bool isSavegame);
    // Other events are only called for the host
    /// Called when the settings screen is ready
    void EventSettingsReady();
    void EventPlayerJoined(unsigned playerIdx);
    void EventPlayerLeft(unsigned playerIdx);
    void EventPlayerReady(unsigned playerIdx);
    /// Return whether the named change is allowed
    bool IsChangeAllowed(const std::string& name, bool defaultVal = false);
    /// Get addons that are allowed to be changed
    std::vector<AddonId> GetAllowedAddons();
    bool IsMapPreviewEnabled();

private:
    IGameLobbyController& lobbyServerController_;
    kaguya::LuaRef GetAllowedChanges();

    // Callable from Lua
    unsigned GetNumPlayers() const;
    LuaServerPlayer GetPlayer(int idx);
    void SetAddon(AddonIdWrapper id, unsigned value);
    void SetBoolAddon(AddonIdWrapper id, bool value); // Alias
    void ResetAddons();
    void ResetGameSettings();
    void SetGameSettings(const kaguya::LuaTable& settings);
};
