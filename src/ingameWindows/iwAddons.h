// Copyright (c) 2005-2010 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.
#ifndef iwADDONS_H_INCLUDED
#define iwADDONS_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "addons/const_addons.h"
#include <vector>

class GlobalGameSettings;
class MouseCoords;

class iwAddons : public IngameWindow
{
        /// Breite der Scrollbar
        static const unsigned short SCROLLBAR_WIDTH = 20;
    public:
        enum ChangePolicy
        {
            HOSTGAME,
            /// Allow only whitelisted addons to change
            HOSTGAME_WHITELIST,
            READONLY,
            SETDEFAULTS
        };

    public:
        iwAddons(GlobalGameSettings& ggs, ChangePolicy policy = SETDEFAULTS, const std::vector<AddonId>& addonIds = std::vector<AddonId>());
        ~iwAddons() override;

    protected:
        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        void Msg_OptionGroupChange(const unsigned int ctrl_id, const int selection) override;
        void Msg_ScrollChange(const unsigned int ctrl_id, const unsigned short position) override;
        bool Msg_WheelUp(const MouseCoords& mc) override;
        bool Msg_WheelDown(const MouseCoords& mc) override;

        /// Aktualisiert die Addons, die angezeigt werden sollen
        void UpdateView(const unsigned short selection);

    private:
        /// settings we edit in this window
        GlobalGameSettings& ggs;
        ChangePolicy policy;
        std::vector<AddonId> addonIds;
        unsigned short numAddonsInCurCategory_;
};

#endif // !iwENHANCEMENTS_H_INCLUDED
