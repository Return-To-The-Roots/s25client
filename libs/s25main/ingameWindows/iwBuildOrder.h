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

#include "TransmitSettingsIgwAdapter.h"
#include "gameTypes/SettingsTypes.h"

class GameWorldViewer;

class iwBuildOrder : public TransmitSettingsIgwAdapter
{
    const GameWorldViewer& gwv;

    BuildOrders pendingBuildOrder;
    bool useCustomBuildOrder;

public:
    iwBuildOrder(const GameWorldViewer& gwv);

private:
    /// Updates the control elements with the pending build order
    void UpdateSettings() override;
    /// Sendet veränderte Einstellungen (an den Client), falls sie verändert wurden
    void TransmitSettings() override;

    void Msg_ListSelectItem(unsigned ctrl_id, int selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
};
