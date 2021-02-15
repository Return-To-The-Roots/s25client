// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

class TransmitSettingsIgwAdapter : public IngameWindow
{
protected:
    /// whether any settings where changed after the last successful transmission
    bool settings_changed;

    static constexpr unsigned firstCtrlID = 1000000u;

public:
    TransmitSettingsIgwAdapter(unsigned id, const DrawPoint& pos, const Extent& size, const std::string& title,
                               glArchivItem_Bitmap* background, bool modal = false, bool closeOnRightClick = true,
                               Window* parent = nullptr);

    /// Updates the control elements with values from visual settings
    virtual void UpdateSettings() = 0;
    /// sends potential changes to the client
    virtual void TransmitSettings() = 0;

    void Close() override;

    void Msg_Timer(unsigned ctrl_id) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
};
