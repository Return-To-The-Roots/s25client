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

class TransmittingSettingsWindow : public IngameWindow
{
protected:
    /// whether any settings where changed after the last successful transmission
    bool settings_changed;

public:
    TransmittingSettingsWindow(unsigned id, const DrawPoint& pos, const Extent& size, std::string title,
                               glArchivItem_Bitmap* background, bool modal = false, bool closeOnRightClick = true,
                               Window* parent = nullptr);

    virtual ~TransmittingSettingsWindow() {}

    /// Updates the control elements with values from visual settings
    virtual void UpdateSettings() {}
    /// sends potential changes to the client
    virtual void TransmitSettings() {}

    void Close() override;

    virtual void Msg_Timer(unsigned ctrl_id) override;
    void Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr) override;
};
