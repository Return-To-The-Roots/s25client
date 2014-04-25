// $Id: GlobalGameSettings.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef GlobalGameSettings_H_INCLUDED
#define GlobalGameSettings_H_INCLUDED

#include "Addons.h"
#include <iostream>
#pragma once

class GlobalGameSettings
{
    public:
        GlobalGameSettings();
        GlobalGameSettings(const GlobalGameSettings& ggs);
        ~GlobalGameSettings();

        void operator=(const GlobalGameSettings& ggs);

        /// Serialisierung und Deserialisierung
        void Serialize(Serializer* ser) const;
        void Deserialize(Serializer* ser);

    public:
        enum GameSpeed { GS_VERYSLOW = 0, GS_SLOW , GS_NORMAL, GS_FAST, GS_VERYFAST } game_speed;
        enum GameObjective { GO_NONE = 0, GO_CONQUER3_4, GO_TOTALDOMINATION } game_objective;
        enum StartWares {SWR_VLOW = 0, SWR_LOW, SWR_NORMAL, SWR_ALOT} start_wares;
        bool lock_teams;
        enum Exploration { EXP_DISABLED = 0, EXP_CLASSIC, EXP_FOGOFWAR, EXP_FOGOFWARE_EXPLORED } exploration;
        bool team_view;
        bool random_location;

        /// clears the addon memory.
        void reset(bool recreate = true);

        const Addon* getAddon(unsigned int nr, unsigned int& status) const
        {
            if(nr >= addons.size())
                return NULL;

            const item* i = &addons.at(nr);

            if(!i->addon)
                return NULL;

            status = i->status;
            return i->addon;
        }

        unsigned int getCount() const { return addons.size(); }

        bool isEnabled(AddonId id) const
        {
            std::vector<item>::const_iterator it = std::find(addons.begin(), addons.end(), id);
            if(it == addons.end() || it->status == it->addon->getDefaultStatus())
                return false;
            return true;
        }

        unsigned int getSelection(AddonId id) const
        {
            std::vector<item>::const_iterator it = std::find(addons.begin(), addons.end(), id);
            if(it == addons.end())
                return 0;
            return it->status;
        }

        void setSelection(AddonId id, unsigned int selection);

        /// loads the saved addon configuration from the SETTINGS.
        void LoadSettings();
        /// saves the current addon configuration to the SETTINGS.
        void SaveSettings() const;

    private:
        void registerAddon(Addon* addon)
        {
            if(!addon)
                return;

            if(std::find(addons.begin(), addons.end(), addon->getId()) == addons.end())
                addons.push_back(item(addon));

            std::sort(addons.begin(), addons.end());
        }

        struct item
        {
            item(void) : addon(NULL), status(0) {}
            item(Addon* addon) : addon(addon), status(addon->getDefaultStatus()) {}

            Addon* addon;
            unsigned int status;

            bool operator==(const AddonId& o) const { return (addon ? addon->getId() == o : false); }
            bool operator<(const item& o) const { return (addon->getName().compare(o.addon->getName()) < 0); }
        };

        std::vector<item> addons;
};

#endif // !GlobalGameSettings_H_INCLUDED
