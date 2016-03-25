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

#ifndef LuaInterfaceSettings_h__
#define LuaInterfaceSettings_h__

#include "LuaInterfaceBase.h"
#include <string>
#include <vector>

class LuaServerPlayer;
struct AddonId;

class LuaInterfaceSettings: public LuaInterfaceBase{
public:
    LuaInterfaceSettings();
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
    bool IsChangeAllowed(const std::string& name, const bool defaultVal = false);
    /// Get addons that are allowed to be changed
    std::vector<AddonId> GetAllowedAddons();

    // Callable from Lua
    unsigned GetPlayerCount();
    LuaServerPlayer GetPlayer(unsigned idx);
    void SetAddon(AddonId id, unsigned value);
    void SetBoolAddon(AddonId id, bool enabled); // Alias
    void ResetAddons();
    void SetGameSettings(const kaguya::LuaTable& settings);

private:
    kaguya::LuaRef GetAllowedChanges();
};

#endif // LuaInterfaceSettings_h__
