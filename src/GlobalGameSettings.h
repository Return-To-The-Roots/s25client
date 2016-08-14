// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef GlobalGameSettings_H_INCLUDED
#define GlobalGameSettings_H_INCLUDED

#include "gameTypes/GameSettingTypes.h"
#include <vector>

class Serializer;
class Addon;
struct AddonId;

class GlobalGameSettings
{
    public:
        GlobalGameSettings();
        GlobalGameSettings(const GlobalGameSettings& ggs);
        ~GlobalGameSettings();

        GlobalGameSettings& operator=(const GlobalGameSettings& ggs);

        /// Serialisierung und Deserialisierung
        void Serialize(Serializer& ser) const;
        void Deserialize(Serializer& ser);

    public:
        GameSpeed speed;
        GameObjective objective;
        StartWares startWares;
        bool lockedTeams;
        Exploration exploration;
        bool teamView;
        bool randomStartPosition;

        unsigned int getNumAddons() const { return addons.size(); }
        const Addon* getAddon(unsigned int nr, unsigned int& status) const;
        const Addon* getAddon(unsigned int nr) const;
        /// clears the addon memory.
        void clearAddons(bool recreate = true);

        bool isEnabled(AddonId id) const;
        unsigned int getSelection(AddonId id) const;
        void setSelection(AddonId id, unsigned int selection);

        /// loads the saved addon configuration from the SETTINGS.
        void LoadSettings();
        /// saves the current addon configuration to the SETTINGS.
        void SaveSettings() const;

        /// Get current maximum rank for soldiers
        /// 0 = Private, 1 = Private First Class, ...
        unsigned int GetMaxMilitaryRank() const;
        /// Returns number of scouts required for exploration expeditions
        unsigned int GetNumScoutsExedition() const;

    private:
        void registerAddon(Addon* addon);

        struct AddonWithState
        {
            AddonWithState() : addon(NULL), status(0) {}
            explicit AddonWithState(Addon* addon);

            Addon* addon;
            unsigned status;

            inline bool operator==(const AddonId& rhs) const;
            inline bool operator<(const AddonWithState& rhs) const;
        };

        typedef std::vector<AddonWithState> AddonContainer;

        AddonContainer addons;
};

#endif // !GlobalGameSettings_H_INCLUDED
