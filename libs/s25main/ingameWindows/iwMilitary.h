// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "TransmitSettingsIgwAdapter.h"
#include "gameTypes/SettingsTypes.h"

class GameCommandFactory;
class GameWorldViewer;

/// Fenster mit den Militäreinstellungen.
class iwMilitary : public TransmitSettingsIgwAdapter
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
