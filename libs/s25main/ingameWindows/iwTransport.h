// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "TransmitSettingsIgwAdapter.h"
#include "gameTypes/SettingsTypes.h"
#include <array>

class ITexture;
class GameWorldViewer;
class GameCommandFactory;

class iwTransport : public TransmitSettingsIgwAdapter
{
private:
    struct ButtonData
    {
        ITexture* sprite;
        std::string tooltip;
    };
    const GameWorldViewer& gwv;
    GameCommandFactory& gcFactory;

    static constexpr auto numButtons = std::tuple_size<TransportOrders>::value;
    std::array<ButtonData, numButtons> buttonData;

    TransportOrders pendingOrder;

public:
    iwTransport(const GameWorldViewer& gwv, GameCommandFactory& gcFactory);

private:
    /// Updatet die Steuerelemente mit den übergebenen Einstellungen
    void UpdateSettings() override;
    /// Sendet veränderte Einstellungen (an den Client), falls sie verändert wurden
    void TransmitSettings() override;

    void Msg_ButtonClick(unsigned ctrl_id) override;
};
