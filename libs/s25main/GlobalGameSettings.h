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

#pragma once

#include "gameTypes/GameSettingTypes.h"
#include <memory>
#include <vector>

class Serializer;
class Addon;
enum class AddonId;

class GlobalGameSettings
{
public:
    GlobalGameSettings();
    GlobalGameSettings(const GlobalGameSettings& ggs);
    GlobalGameSettings(GlobalGameSettings&&) noexcept;
    ~GlobalGameSettings();

    GlobalGameSettings& operator=(const GlobalGameSettings& ggs);
    GlobalGameSettings& operator=(GlobalGameSettings&&) noexcept;

    /// Serialisierung und Deserialisierung
    void Serialize(Serializer& ser) const;
    void Deserialize(Serializer& ser);

    GameSpeed speed;
    GameObjective objective;
    StartWares startWares;
    bool lockedTeams;
    Exploration exploration;
    bool teamView;
    bool randomStartPosition;

    unsigned getNumAddons() const { return addons.size(); }
    const Addon* getAddon(unsigned idx, unsigned& status) const;
    const Addon* getAddon(unsigned idx) const;

    void registerAllAddons();

    /// Reset all addons to their defaults
    void resetAddons();

    bool isEnabled(AddonId id) const;
    unsigned getSelection(AddonId id) const;
    void setSelection(AddonId id, unsigned selection);

    /// loads the saved addon configuration from the SETTINGS.
    void LoadSettings();
    /// saves the current addon configuration to the SETTINGS.
    void SaveSettings() const;

    /// Get current maximum rank for soldiers
    /// 0 = Private, 1 = Private First Class, ...
    unsigned GetMaxMilitaryRank() const;
    /// Returns number of scouts required for exploration expeditions
    unsigned GetNumScoutsExpedition() const;

private:
    struct AddonWithState
    {
        explicit AddonWithState(std::unique_ptr<Addon> addon);

        std::unique_ptr<Addon> addon;
        unsigned status;
    };

    void registerAddon(std::unique_ptr<Addon> addon);
    const AddonWithState* getAddon(AddonId id) const;
    AddonWithState* getAddon(AddonId id);

    std::vector<AddonWithState> addons;
};
