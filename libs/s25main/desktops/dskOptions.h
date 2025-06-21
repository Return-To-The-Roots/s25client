// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

    void Msg_ScreenResize(const ScreenResizeEvent& sr) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;

private:
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;

    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void Msg_Group_ProgressChange(unsigned group_id, unsigned ctrl_id, unsigned short position) override;
    void Msg_Group_ComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void Msg_Group_OptionGroupChange(unsigned group_id, unsigned ctrl_id, unsigned selection) override;

    void updateGuiScale();
    void scrollGuiScale(bool up);

    GlobalGameSettings ggs;
    std::vector<VideoMode> video_modes;    ///< Sorted list of filtered video modes
    std::vector<unsigned> guiScales_;      ///< Generated GUI scale percentages
    std::size_t recommendedGuiScaleIndex_; ///< Index of the recommended GUI scale percentage

    void loadVideoModes();
    void updatePortraitControls();
};
