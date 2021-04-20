// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "TransmitSettingsIgwAdapter.h"
#include "notifications/Subscription.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/SettingsTypes.h"
#include <array>

class GameCommandFactory;
class GameWorldViewer;

/// Tool settings window
class iwTools : public TransmitSettingsIgwAdapter
{
public:
    iwTools(const GameWorldViewer& gwv, GameCommandFactory& gcFactory);

private:
    const GameWorldViewer& gwv;
    GameCommandFactory& gcFactory;
    /// How the order for each tool should be changed (pending actual transmission)
    std::array<int8_t, NUM_TOOLS> pendingOrderChanges;
    /// Einstellungen nach dem letzten Netzwerk-Versenden nochmal verändert?
    bool ordersChanged;
    bool shouldUpdateTexts;
    bool isReplay;
    Subscription toolSubscription;

    void AddToolSettingSlider(unsigned id, GoodType ware);
    /// Updatet die Steuerelemente mit den übergebenen Einstellungen
    void UpdateSettings(const ToolSettings& tool_settings);
    void UpdateSettings() override;
    /// Sendet veränderte Einstellungen (an den Client), falls sie verändert wurden
    void TransmitSettings() override;

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_ProgressChange(unsigned ctrl_id, unsigned short position) override;

    void UpdateTexts();
    void Msg_PaintBefore() override;
};
