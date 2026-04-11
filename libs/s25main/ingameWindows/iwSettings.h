// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "driver/VideoMode.h"

class iwSettings : public IngameWindow
{
public:
    iwSettings();

protected:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_CheckboxChange(unsigned ctrl_id, bool checked) override;
    void Msg_Group_ComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void Msg_ScreenResize(const ScreenResizeEvent& sr) override;

private:
    std::vector<VideoMode> supportedResolutions_, windowSizes_;
    void updateWindowSizeComboBox();
    void updateResolutionGroups();
    DisplayMode getDisplayModeSelection() const;
};
