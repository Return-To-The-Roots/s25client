// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwSettings.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlOptionGroup.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/format.hpp"
#include "iwMsgbox.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"

namespace {
enum
{
    ID_txtResolution,
    ID_txtFullScreen,
    ID_grpFullscreen,
    ID_cbResolution,
    ID_cbInvertMouse,
    ID_cbSmartCursor,
    ID_cbStatisticScale,
};
constexpr auto ID_btOn = 1;
constexpr auto ID_btOff = 0;
} // namespace

iwSettings::iwSettings()
    : IngameWindow(CGI_SETTINGS, IngameWindow::posLastOrCenter, Extent(370, 228), _("Settings"),
                   LOADER.GetImageN("resource", 41))
{
    // Controls are in 2 columns, the left might be the label for the control on the right
    constexpr auto leftColOffset = 15u;   // X-position of the left column
    constexpr auto rightColOffset = 200u; // X-position of the right column
    constexpr Extent ctrlSize(150, 22);
    constexpr auto rowWidth = rightColOffset + ctrlSize.x;

    AddText(ID_txtResolution, DrawPoint(leftColOffset, 40), _("Fullscreen resolution:"), COLOR_YELLOW, FontStyle{},
            NormalFont);
    auto* cbResolution =
      AddComboBox(ID_cbResolution, DrawPoint(rightColOffset, 35), ctrlSize, TextureColor::Grey, NormalFont, 110);

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
    AddText(ID_txtFullScreen, DrawPoint(leftColOffset, 85), _("Mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlOptionGroup* optiongroup = AddOptionGroup(ID_grpFullscreen, GroupSelectType::Check);
    DrawPoint curPos(rightColOffset, 70);
    optiongroup->AddTextButton(ID_btOn, curPos, ctrlSize, TextureColor::Grey, _("Fullscreen"), NormalFont);
    curPos.y += ctrlSize.y + 3;
    optiongroup->AddTextButton(ID_btOff, curPos, ctrlSize, TextureColor::Grey, _("Windowed"), NormalFont);
    optiongroup->SetSelection(SETTINGS.video.fullscreen); //-V807

    curPos = DrawPoint(leftColOffset, curPos.y + ctrlSize.y + 5);
    const auto cbSize = Extent(rowWidth - curPos.x, 26);
    AddCheckBox(ID_cbInvertMouse, curPos, cbSize, TextureColor::Grey, _("Invert Mouse Pan"), NormalFont, false)
      ->setChecked(SETTINGS.interface.invertMouse);
    curPos.y += cbSize.y + 3;
    AddCheckBox(ID_cbSmartCursor, curPos, cbSize, TextureColor::Grey, _("Smart Cursor"), NormalFont, false)
      ->setChecked(SETTINGS.global.smartCursor)
      ->SetTooltip(_("Place cursor on default button for new dialogs / action windows (default)"));
    curPos.y += cbSize.y + 3;
    AddCheckBox(ID_cbStatisticScale, curPos, cbSize, TextureColor::Grey, _("Statistics Scale"), NormalFont, false)
      ->setChecked(SETTINGS.ingame.scale_statistics);
}

iwSettings::~iwSettings()
{
    try
    {
        auto* SizeCombo = GetCtrl<ctrlComboBox>(ID_cbResolution);
        SETTINGS.video.fullscreenSize = video_modes[SizeCombo->GetSelection().get()];

        if((SETTINGS.video.fullscreen && SETTINGS.video.fullscreenSize != VIDEODRIVER.GetWindowSize())
           || SETTINGS.video.fullscreen != VIDEODRIVER.IsFullscreen())
        {
            const auto screenSize =
              SETTINGS.video.fullscreen ? SETTINGS.video.fullscreenSize : SETTINGS.video.windowedSize;
            if(!VIDEODRIVER.ResizeScreen(screenSize, SETTINGS.video.fullscreen))
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

void iwSettings::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_grpFullscreen: SETTINGS.video.fullscreen = selection == ID_btOn; break;
    }
}

void iwSettings::Msg_CheckboxChange(const unsigned ctrl_id, const bool checked)
{
    switch(ctrl_id)
    {
        case ID_cbInvertMouse: SETTINGS.interface.invertMouse = checked; break;
        case ID_cbSmartCursor:
            SETTINGS.global.smartCursor = checked;
            VIDEODRIVER.SetMouseWarping(checked);
            break;
        case ID_cbStatisticScale: SETTINGS.ingame.scale_statistics = checked; break;
    }
}
