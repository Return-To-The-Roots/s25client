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

#ifndef LuaInterfaceSettings_h__
#define LuaInterfaceSettings_h__

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

#endif // LuaInterfaceSettings_h__
