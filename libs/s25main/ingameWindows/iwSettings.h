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
    void msgButtonClick(unsigned ctrl_id) override;
    void msgComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void msgCheckboxChange(unsigned ctrl_id, bool checked) override;
    void msgGroupComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void msgScreenResize(const ScreenResizeEvent& sr) override;

private:
    std::vector<VideoMode> supportedResolutions_, windowSizes_;
    void updateWindowSizeComboBox();
    void updateResolutionGroups();
    DisplayMode getDisplayModeSelection() const;
};
