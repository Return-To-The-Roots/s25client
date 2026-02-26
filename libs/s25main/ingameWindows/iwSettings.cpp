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
#include "helpers/containerUtils.h"
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
    ID_cbLockWindowSize,
    ID_cbResolution,
    ID_txtMapScrollMode,
    ID_cbMapScrollMode,
    ID_cbSmartCursor,
    ID_cbStatisticScale,
    ID_btApply,
    ID_btAbort,
};
} // namespace

iwSettings::iwSettings()
    : IngameWindow(CGI_SETTINGS, IngameWindow::posLastOrCenter, Extent(370, 254), _("Settings"),
                   LOADER.GetImageN("resource", 41), false)
{
    // Controls are in 2 columns, the left might be the label for the control on the right
    constexpr auto rowWidth = 350;
    constexpr auto leftColOffset = 17u;   // X-position of the left column
    constexpr auto rightColOffset = 180u; // X-position of the right column
    constexpr Extent ctrlSize(rowWidth - rightColOffset, 22);
    constexpr Extent cbSize = Extent(rowWidth - leftColOffset, 26);
    // 2 buttons evenly spaced
    constexpr Extent btSize = Extent(rowWidth / 3, 22);
    constexpr auto btSpacing = btSize.x / 3;

    DrawPoint curPos(leftColOffset, 30);
    AddText(ID_txtResolution, curPos, _("Fullscreen resolution:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    auto* cbResolution = AddComboBox(ID_cbResolution, DrawPoint(rightColOffset, curPos.y - 5), ctrlSize,
                                     TextureColor::Grey, NormalFont, 110);
    curPos.y += ctrlSize.y + 5;

    supportedVideoModes = VIDEODRIVER.ListVideoModes();
    // Remove everything below 800x600
    helpers::erase_if(supportedVideoModes, [](const auto& it) { return it.width < 800 && it.height < 600; });
    for(const auto& videoMode : supportedVideoModes)
    {
        cbResolution->AddItem(helpers::format("%ux%u", videoMode.width, videoMode.height));
        if(videoMode == SETTINGS.video.fullscreenSize)
            cbResolution->SetSelection(cbResolution->GetNumItems() - 1);
    }

    AddText(ID_txtFullScreen, curPos, _("Mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* cbDisplayMode = AddComboBox(ID_cbDisplayMode, DrawPoint(rightColOffset, curPos.y - 5), ctrlSize,
                                              TextureColor::Grey, NormalFont, 100);
    cbDisplayMode->AddItem(_("Windowed"));
    cbDisplayMode->AddItem(_("Fullscreen"));
    cbDisplayMode->AddItem(_("Borderless window"));
    cbDisplayMode->SetSelection(rttr::enum_cast(SETTINGS.video.displayMode.type));

    curPos.y += ctrlSize.y + 1;
    AddCheckBox(ID_cbLockWindowSize, curPos, cbSize, TextureColor::Grey, _("Lock window size:"), NormalFont, false)
      ->setChecked(!SETTINGS.video.displayMode.resizeable);

    curPos.y += cbSize.y + 15;

    AddText(ID_txtMapScrollMode, curPos, _("Map scroll mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* cbMapScrollMode = AddComboBox(ID_cbMapScrollMode, DrawPoint(rightColOffset, curPos.y - 5), ctrlSize,
                                                TextureColor::Grey, NormalFont, 100);
    cbMapScrollMode->AddItem(
      _("Scroll same (Map moves in the same direction the mouse is moved when scrolling/panning.)"));
    cbMapScrollMode->AddItem(
      _("Scroll opposite (Map moves in the opposite direction the mouse is moved when scrolling/panning.)"));
    cbMapScrollMode->AddItem(_("Grab and drag (Map moves with your cursor when scrolling/panning.)"));
    cbMapScrollMode->SetSelection(static_cast<int>(SETTINGS.interface.mapScrollMode));

    curPos.y += ctrlSize.y + 10;
    AddCheckBox(ID_cbSmartCursor, curPos, cbSize, TextureColor::Grey, _("Smart Cursor"), NormalFont, false)
      ->setChecked(SETTINGS.global.smartCursor)
      ->SetTooltip(_("Place cursor on default button for new dialogs / action windows (default)"));
    curPos.y += cbSize.y + 3;
    AddCheckBox(ID_cbStatisticScale, curPos, cbSize, TextureColor::Grey, _("Statistics Scale"), NormalFont, false)
      ->setChecked(SETTINGS.ingame.scaleStatistics);

    curPos = DrawPoint(leftColOffset + btSpacing, GetSize().y - 40);
    AddTextButton(ID_btApply, curPos, btSize, TextureColor::Green2, _("Apply"), NormalFont, _("Apply Changes"));
    curPos.x += btSize.x + btSpacing;
    AddTextButton(ID_btAbort, curPos, btSize, TextureColor::Red1, _("Abort"), NormalFont, _("Close Without Saving"));
}

void iwSettings::Msg_ButtonClick(unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btApply:
        {
            SETTINGS.global.smartCursor = GetCtrl<ctrlCheck>(ID_cbSmartCursor)->isChecked();
            VIDEODRIVER.SetMouseWarping(SETTINGS.global.smartCursor);
            SETTINGS.ingame.scaleStatistics = GetCtrl<ctrlCheck>(ID_cbStatisticScale)->isChecked();
            SETTINGS.video.displayMode = DisplayMode(*GetCtrl<ctrlComboBox>(ID_cbDisplayMode)->GetSelection());
            SETTINGS.video.displayMode.resizeable = !GetCtrl<ctrlCheck>(ID_cbLockWindowSize)->isChecked();
            SETTINGS.interface.mapScrollMode =
              static_cast<MapScrollMode>(GetCtrl<ctrlComboBox>(ID_cbMapScrollMode)->GetSelection().get());
            SETTINGS.video.fullscreenSize =
              supportedVideoModes[GetCtrl<ctrlComboBox>(ID_cbResolution)->GetSelection().get()];
            closeBehavior_ = CloseBehavior::Regular;

            const auto screenSize = SETTINGS.video.displayMode == DisplayMode::Fullscreen ?
                                      SETTINGS.video.fullscreenSize :
                                      SETTINGS.video.windowedSize;
            if(VIDEODRIVER.GetWindowSize() != screenSize || SETTINGS.video.displayMode != VIDEODRIVER.GetDisplayMode())
            {
                if(!VIDEODRIVER.ResizeScreen(screenSize, SETTINGS.video.displayMode))
                {
                    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                      _("Sorry!"), _("You need to restart your game to change the screen resolution!"), this,
                      MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
                }
            }
        }
        break;
        case ID_btAbort: Close(); break;
        default: RTTR_Assert(false);
    }
}

void iwSettings::Msg_ComboSelectItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    // Handle on apply and just disable closing the window without the buttons (apply/discard)
    closeBehavior_ = CloseBehavior::Custom;
}

void iwSettings::Msg_CheckboxChange(const unsigned /*ctrl_id*/, const bool /*checked*/)
{
    // Handle on apply and just disable closing the window without the buttons (apply/discard)
    closeBehavior_ = CloseBehavior::Custom;
}
