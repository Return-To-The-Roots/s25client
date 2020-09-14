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

#include "IngameWindow.h"
#include "gameTypes/SettingsTypes.h"
#include <array>

class ITexture;
class GameWorldViewer;
class GameCommandFactory;

class iwTransport : public IngameWindow
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

    /// Einstellungen nach dem letzten Netzwerk-Versenden nochmal verändert?
    bool settings_changed;

public:
    iwTransport(const GameWorldViewer& gwv, GameCommandFactory& gcFactory);
    ~iwTransport() override;

private:
    /// Updatet die Steuerelemente mit den aktuellen Einstellungen aus dem Spiel
    void UpdateSettings();
    /// Sendet veränderte Einstellungen (an den Client), falls sie verändert wurden
    void TransmitSettings();

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Timer(unsigned ctrl_id) override;
};
