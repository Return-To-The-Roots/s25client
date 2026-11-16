// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
                               glArchivItem_Bitmap* background, bool modal = false);

    /// Updates the control elements with values from visual settings
    virtual void updateSettings() = 0;
    /// sends potential changes to the client
    virtual void transmitSettings() = 0;

    void close() override;

    void msgTimer(unsigned ctrl_id) override;
    void msgMsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
};
