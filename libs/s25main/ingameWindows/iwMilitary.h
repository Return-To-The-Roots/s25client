// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "TransmitSettingsIgwAdapter.h"
#include "gameTypes/SettingsTypes.h"

class GameCommandFactory;
class GameWorldViewer;

/// Window for changing military settings, e.g. occupation distribution
class iwMilitary final : public TransmitSettingsIgwAdapter
{
    GameCommandFactory& gcFactory;

public:
    iwMilitary(const GameWorldViewer& gwv, GameCommandFactory& gcFactory);

private:
    /// Update controls with given military settings
    void UpdateSettings(const MilitarySettings& military_settings);
    void UpdateSettings() override;
    /// Send changed values to client, if any have changed
    void TransmitSettings() override;

    void Msg_ProgressChange(unsigned ctrl_id, unsigned short position) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
