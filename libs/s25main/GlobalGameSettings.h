// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
