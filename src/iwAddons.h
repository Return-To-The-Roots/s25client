// $Id: iwAddons.h 9357 2014-04-25 15:35:25Z FloSoft $
//
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

class GlobalGameSettings;

class iwAddons : public IngameWindow
{
        /// Breite der Scrollbar
        static const unsigned short SCROLLBAR_WIDTH = 20;
        /// Pointer to the settings we edit in this window
        GlobalGameSettings* ggs;

    public:
        enum ChangePolicy
        {
            HOSTGAME,
            READONLY,
            SETDEFAULTS
        };

    public:
        iwAddons(GlobalGameSettings* ggs, ChangePolicy policy = SETDEFAULTS);
        ~iwAddons(void);

    protected:
        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_OptionGroupChange(const unsigned int ctrl_id, const unsigned short selection);
        void Msg_ScrollChange(const unsigned int ctrl_id, const unsigned short position);
        bool Msg_WheelUp(const MouseCoords& mc);
        bool Msg_WheelDown(const MouseCoords& mc);

        /// Aktualisiert die Addons, die angezeigt werden sollen
        void UpdateView(const unsigned short selection);

    private:
        ChangePolicy policy;
        unsigned short _inthiscategory;
};

#endif // !iwENHANCEMENTS_H_INCLUDED
