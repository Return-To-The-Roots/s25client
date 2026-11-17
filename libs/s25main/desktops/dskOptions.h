// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"
#include "GlobalGameSettings.h"
#include "driver/VideoMode.h"

/// Klasse des Optionen Desktops.
class dskOptions : public Desktop
{
public:
    dskOptions();
    ~dskOptions() override;

    void msgScreenResize(const ScreenResizeEvent& sr) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;

private:
    void msgOptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgMsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;

    void msgGroupButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void msgGroupProgressChange(unsigned group_id, unsigned ctrl_id, unsigned short position) override;
    void msgGroupComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void msgGroupOptionGroupChange(unsigned group_id, unsigned ctrl_id, unsigned selection) override;

    void updateGuiScale();
    void scrollGuiScale(bool up);
    void updateWindowSizeComboBox();
    void updateResolutionGroups();

    GlobalGameSettings ggs;
    std::vector<VideoMode> videoModes_;    ///< Sorted list of filtered video modes
    std::vector<VideoMode> windowSizes_;   ///< Sorted list of default window sizes
    std::vector<unsigned> guiScales_;      ///< Generated GUI scale percentages
    std::size_t recommendedGuiScaleIndex_; ///< Index of the recommended GUI scale percentage

    void loadVideoModes();
    void updatePortraitControls();
};
