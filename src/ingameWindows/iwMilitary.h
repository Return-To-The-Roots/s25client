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
#ifndef iwMILITARY_H_INCLUDED
#define iwMILITARY_H_INCLUDED

#pragma once

#include "IngameWindow.h"

/// Fenster mit den Militäreinstellungen.
class iwMilitary : public IngameWindow
{
        /// Einstellungen nach dem letzten Netzwerk-Versenden nochmal verändert?
        bool settings_changed;

    public:

        iwMilitary();
        ~iwMilitary() override;

    private:

        /// Updatet die Steuerelemente mit den aktuellen Einstellungen aus dem Spiel
        void UpdateSettings();
        /// Sendet veränderte Einstellungen (an den Client), falls sie verändert wurden
        void TransmitSettings();

        void Msg_Timer(const unsigned int ctrl_id) override;
        void Msg_ProgressChange(const unsigned int ctrl_id, const unsigned short position) override;
        void Msg_ButtonClick(const unsigned ctrl_id) override;
};

#endif // !iwMILITARY_H_INCLUDED
