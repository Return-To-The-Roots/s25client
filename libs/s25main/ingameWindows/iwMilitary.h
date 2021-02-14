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

#include "TransmittingSettingsWindow.h"
#include "gameTypes/SettingsTypes.h"

class GameCommandFactory;
class GameWorldViewer;

/// Fenster mit den Militäreinstellungen.
class iwMilitary : public TransmittingSettingsWindow
{
    GameCommandFactory& gcFactory;

public:
    iwMilitary(const GameWorldViewer& gwv, GameCommandFactory& gcFactory);

private:
    /// Updatet die Steuerelemente mit den übergebenen Einstellungen
    void UpdateSettings(const MilitarySettings& military_settings);
    void UpdateSettings() override;
    /// Sendet veränderte Einstellungen (an den Client), falls sie verändert wurden
    void TransmitSettings() override;

    void Msg_ProgressChange(unsigned ctrl_id, unsigned short position) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
