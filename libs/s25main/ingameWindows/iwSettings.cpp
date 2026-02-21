// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwSettings.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlOptionGroup.h"
#include "driver/VideoInterface.h"
#include "drivers/VideoDriverWrapper.h"
#include "enum_cast.hpp"
#include "helpers/format.hpp"
#include "iwMsgbox.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"

namespace {
enum
{
    ID_txtResolution,
    ID_txtFullScreen,
    ID_cbDisplayMode,
    ID_cbResolution,
    ID_txtMapScrollMode,
    ID_cbMapScrollMode,
    ID_cbSmartCursor,
    ID_cbStatisticScale,
};
} // namespace

iwSettings::iwSettings()
    : IngameWindow(CGI_SETTINGS, IngameWindow::posLastOrCenter, Extent(370, 228), _("Settings"),
                   LOADER.GetImageN("resource", 41))
{
    // Controls are in 2 columns, the left might be the label for the control on the right
    constexpr auto rowWidth = 350;
    constexpr auto leftColOffset = 15u;   // X-position of the left column
    constexpr auto rightColOffset = 180u; // X-position of the right column
    constexpr Extent ctrlSize(rowWidth - rightColOffset, 22);
    constexpr Extent cbSize = Extent(rowWidth - leftColOffset, 26);

    DrawPoint curPos(leftColOffset, 40);
    AddText(ID_txtResolution, curPos, _("Fullscreen resolution:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    auto* cbResolution = AddComboBox(ID_cbResolution, DrawPoint(rightColOffset, curPos.y - 5), ctrlSize,
                                     TextureColor::Grey, NormalFont, 110);
    curPos.y += ctrlSize.y + 5;

    VIDEODRIVER.ListVideoModes(video_modes);
    for(unsigned i = 0; i < video_modes.size(); ++i)
    {
        // >=800x600, alles andere macht keinen Sinn
        if(video_modes[i].width >= 800 && video_modes[i].height >= 600)
        {
            cbResolution->AddString(helpers::format("%ux%u", video_modes[i].width, video_modes[i].height));
            if(video_modes[i] == SETTINGS.video.fullscreenSize)
                cbResolution->SetSelection(i);
        } else
        {
            video_modes.erase(video_modes.begin() + i);
            --i;
        }
    }

    AddText(ID_txtFullScreen, curPos, _("Mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* cbDisplayMode = AddComboBox(ID_cbDisplayMode, DrawPoint(rightColOffset, curPos.y - 5), ctrlSize,
                                              TextureColor::Grey, NormalFont, 100);
    cbDisplayMode->AddString(_("Windowed"));
    cbDisplayMode->AddString(_("Fullscreen"));
    cbDisplayMode->AddString(_("Borderless window"));
    cbDisplayMode->SetSelection(rttr::enum_cast(SETTINGS.video.displayMode));
    curPos.y += ctrlSize.y + 10;

    AddText(ID_txtMapScrollMode, curPos, _("Map scroll mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* cbMapScrollMode = AddComboBox(ID_cbMapScrollMode, DrawPoint(rightColOffset, curPos.y - 5), ctrlSize,
                                                TextureColor::Grey, NormalFont, 100);
    cbMapScrollMode->AddString(
      _("Scroll same (Map moves in the same direction the mouse is moved when scrolling/panning.)"));
    cbMapScrollMode->AddString(
      _("Scroll opposite (Map moves in the opposite direction the mouse is moved when scrolling/panning.)"));
    cbMapScrollMode->AddString(_("Grab and drag (Map moves with your cursor when scrolling/panning.)"));
    cbMapScrollMode->SetSelection(static_cast<int>(SETTINGS.interface.mapScrollMode));

    curPos.y += ctrlSize.y + 10;
    AddCheckBox(ID_cbSmartCursor, curPos, cbSize, TextureColor::Grey, _("Smart Cursor"), NormalFont, false)
      ->setChecked(SETTINGS.global.smartCursor)
      ->SetTooltip(_("Place cursor on default button for new dialogs / action windows (default)"));
    curPos.y += cbSize.y + 3;
    AddCheckBox(ID_cbStatisticScale, curPos, cbSize, TextureColor::Grey, _("Statistics Scale"), NormalFont, false)
      ->setChecked(SETTINGS.ingame.scaleStatistics);
}

iwSettings::~iwSettings()
{
    try
    {
        auto* MouseMdCombo = GetCtrl<ctrlComboBox>(ID_cbMapScrollMode);
        SETTINGS.interface.mapScrollMode = static_cast<MapScrollMode>(MouseMdCombo->GetSelection().get());

        auto* SizeCombo = GetCtrl<ctrlComboBox>(ID_cbResolution);
        SETTINGS.video.fullscreenSize = video_modes[SizeCombo->GetSelection().get()];

        const auto fullscreen = SETTINGS.video.displayMode == DisplayMode::Fullscreen;
        if((fullscreen && SETTINGS.video.fullscreenSize != VIDEODRIVER.GetWindowSize())
           || SETTINGS.video.displayMode != VIDEODRIVER.GetDisplayMode())
        {
            const auto screenSize = fullscreen ? SETTINGS.video.fullscreenSize : SETTINGS.video.windowedSize;
            if(!VIDEODRIVER.ResizeScreen(screenSize, SETTINGS.video.displayMode))
            {
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                  _("Sorry!"), _("You need to restart your game to change the screen resolution!"), this,
                  MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
            }
        }
    } catch(...)
    {
        // ignored
    }
}

void iwSettings::Msg_ComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    RTTR_Assert(ctrl_id == ID_cbDisplayMode);
    SETTINGS.video.displayMode = DisplayMode(selection);
}

void iwSettings::Msg_CheckboxChange(const unsigned ctrl_id, const bool checked)
{
    switch(ctrl_id)
    {
        case ID_cbSmartCursor:
            SETTINGS.global.smartCursor = checked;
            VIDEODRIVER.SetMouseWarping(checked);
            break;
        case ID_cbStatisticScale: SETTINGS.ingame.scaleStatistics = checked; break;
    }
}
