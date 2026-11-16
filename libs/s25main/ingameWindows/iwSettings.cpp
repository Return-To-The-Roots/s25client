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
    ID_txtDisplayMode,
    ID_cbDisplayMode,
    ID_cbLockWindowSize,
    ID_grpResolution,
    ID_txtResolution,
    ID_cbResolution,
    ID_grpWindowSize,
    ID_txtWindowSize,
    ID_cbWindowSize,
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
                   LOADER.getImageN("resource", 41), false)
{
    // Controls are in 2 columns, the left might be the label for the control on the right
    constexpr auto rowWidth = 350;
    constexpr auto leftColOffset = 17u;                           // X-position of the left column
    constexpr DrawPoint rightColOffset(170u - leftColOffset, -1); // Offset from left column to right column
    constexpr Extent ctrlSize(rowWidth - rightColOffset.x - leftColOffset, 22);
    constexpr Extent cbSize = Extent(rowWidth - leftColOffset, 26);
    // 2 buttons evenly spaced
    constexpr Extent btSize = Extent(rowWidth / 3, 22);
    constexpr auto btSpacing = btSize.x / 3;

    DrawPoint curPos(leftColOffset, 30);
    AddText(ID_txtDisplayMode, curPos, _("Mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* cbDisplayMode =
      AddComboBox(ID_cbDisplayMode, curPos + rightColOffset, ctrlSize, TextureColor::Grey, NormalFont, 100);
    cbDisplayMode->AddItem(_("Windowed"));
    cbDisplayMode->AddItem(_("Fullscreen"));
    cbDisplayMode->AddItem(_("Borderless window"));
    cbDisplayMode->setSelection(rttr::enum_cast(SETTINGS.video.displayMode.type));
    curPos.y += ctrlSize.y + 5;

    auto* grpResolution = AddGroup(ID_grpResolution);
    grpResolution->AddText(ID_txtResolution, curPos, _("Fullscreen resolution:"), COLOR_YELLOW, FontStyle{},
                           NormalFont);
    auto* cbResolution = grpResolution->AddComboBox(ID_cbResolution, curPos + rightColOffset, ctrlSize,
                                                    TextureColor::Grey, NormalFont, 110);
    auto* grpWindowSize = AddGroup(ID_grpWindowSize);
    grpWindowSize->AddText(ID_txtWindowSize, curPos, _("Window size:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    auto& cbWindowSize = *grpWindowSize->AddComboBox(ID_cbWindowSize, curPos + rightColOffset, ctrlSize,
                                                     TextureColor::Grey, NormalFont, 150);
    curPos.y += ctrlSize.y + 1;
    grpWindowSize
      ->AddCheckBox(ID_cbLockWindowSize, curPos, cbSize, TextureColor::Grey, _("Lock window size:"), NormalFont, false)
      ->setChecked(!SETTINGS.video.displayMode.resizeable);

    const auto supportedResolutions = VIDEODRIVER.ListVideoModes();
    supportedResolutions_.assign(supportedResolutions.begin(), supportedResolutions.end());
    for(const auto& videoMode : supportedResolutions_)
    {
        cbResolution->AddItem(helpers::format("%ux%u", videoMode.width, videoMode.height));
        if(videoMode == SETTINGS.video.fullscreenSize)
            cbResolution->setSelection(cbResolution->getNumItems() - 1);
    }
    const auto windowSizes = VIDEODRIVER.getDefaultWindowSizes();
    windowSizes_.assign(windowSizes.begin(), windowSizes.end());
    cbWindowSize.AddItem(""); // Placeholder for current window size
    for(const auto& size : windowSizes_)
        cbWindowSize.AddItem(helpers::format("%ux%u", size.width, size.height));

    curPos.y += cbSize.y + 15;

    AddText(ID_txtMapScrollMode, curPos, _("Map scroll mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* cbMapScrollMode =
      AddComboBox(ID_cbMapScrollMode, curPos + rightColOffset, ctrlSize, TextureColor::Grey, NormalFont, 100);
    cbMapScrollMode->AddItem(
      _("Scroll same (Map moves in the same direction the mouse is moved when scrolling/panning.)"));
    cbMapScrollMode->AddItem(
      _("Scroll opposite (Map moves in the opposite direction the mouse is moved when scrolling/panning.)"));
    cbMapScrollMode->AddItem(_("Grab and drag (Map moves with your cursor when scrolling/panning.)"));
    cbMapScrollMode->setSelection(static_cast<int>(SETTINGS.interface.mapScrollMode));

    curPos.y += ctrlSize.y + 10;
    AddCheckBox(ID_cbSmartCursor, curPos, cbSize, TextureColor::Grey, _("Smart Cursor"), NormalFont, false)
      ->setChecked(SETTINGS.global.smartCursor)
      ->setTooltip(_("Place cursor on default button for new dialogs / action windows (default)"));
    curPos.y += cbSize.y + 3;
    AddCheckBox(ID_cbStatisticScale, curPos, cbSize, TextureColor::Grey, _("Statistics Scale"), NormalFont, false)
      ->setChecked(SETTINGS.ingame.scaleStatistics);

    curPos = DrawPoint(leftColOffset + btSpacing, getSize().y - 40);
    AddTextButton(ID_btApply, curPos, btSize, TextureColor::Green2, _("Apply"), NormalFont, _("Apply Changes"));
    curPos.x += btSize.x + btSpacing;
    AddTextButton(ID_btAbort, curPos, btSize, TextureColor::Red1, _("Abort"), NormalFont, _("Close Without Saving"));

    updateWindowSizeComboBox();
    updateResolutionGroups();
}

void iwSettings::msgButtonClick(unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btApply:
        {
            auto& grpWindowSize = *getCtrl<ctrlGroup>(ID_grpWindowSize);
            SETTINGS.global.smartCursor = getCtrl<ctrlCheck>(ID_cbSmartCursor)->isChecked();
            VIDEODRIVER.setMouseWarping(SETTINGS.global.smartCursor);
            SETTINGS.ingame.scaleStatistics = getCtrl<ctrlCheck>(ID_cbStatisticScale)->isChecked();
            SETTINGS.video.displayMode = DisplayMode(getCtrl<ctrlComboBox>(ID_cbDisplayMode)->getSelection().value());
            SETTINGS.video.displayMode.resizeable = !grpWindowSize.getCtrl<ctrlCheck>(ID_cbLockWindowSize)->isChecked();
            SETTINGS.interface.mapScrollMode =
              static_cast<MapScrollMode>(getCtrl<ctrlComboBox>(ID_cbMapScrollMode)->getSelection().value());
            SETTINGS.video.fullscreenSize = supportedResolutions_
              [getCtrl<ctrlGroup>(ID_grpResolution)->getCtrl<ctrlComboBox>(ID_cbResolution)->getSelection().value()];
            const unsigned windowSizeSelection =
              grpWindowSize.getCtrl<ctrlComboBox>(ID_cbWindowSize)->getSelection().value();
            if(windowSizeSelection > 0)
                SETTINGS.video.windowedSize = windowSizes_[windowSizeSelection - 1];
            else if(VIDEODRIVER.getDisplayMode() == DisplayMode::Windowed)
                SETTINGS.video.windowedSize = VIDEODRIVER.getWindowSize();
            closeBehavior_ = CloseBehavior::Regular;

            const auto screenSize = SETTINGS.video.displayMode == DisplayMode::Fullscreen ?
                                      SETTINGS.video.fullscreenSize :
                                      SETTINGS.video.windowedSize;
            if((SETTINGS.video.displayMode != DisplayMode::BorderlessWindow
                && VIDEODRIVER.getWindowSize() != screenSize)
               || SETTINGS.video.displayMode != VIDEODRIVER.getDisplayMode())
            {
                if(!VIDEODRIVER.ResizeScreen(screenSize, SETTINGS.video.displayMode))
                {
                    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                      _("Sorry!"), _("You need to restart your game to change the screen resolution!"), this,
                      MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
                }
            }
            SETTINGS.Save();
        }
        break;
        case ID_btAbort: Close(); break;
        default: RTTR_Assert(false);
    }
}

void iwSettings::msgComboSelectItem(const unsigned ctrl_id, const unsigned /*selection*/)
{
    // Handle on apply and just disable closing the window without the buttons (apply/discard)
    closeBehavior_ = CloseBehavior::Custom;
    if(ctrl_id == ID_cbDisplayMode)
        updateResolutionGroups();
}

void iwSettings::msgCheckboxChange(const unsigned /*ctrl_id*/, const bool /*checked*/)
{
    // Handle on apply and just disable closing the window without the buttons (apply/discard)
    closeBehavior_ = CloseBehavior::Custom;
}

void iwSettings::msgGroupComboSelectItem(unsigned /*group_id*/, unsigned ctrl_id, unsigned selection)
{
    if(ctrl_id == ID_cbWindowSize && selection > 0)
    {
        const DisplayMode currentDisplayMode = getDisplayModeSelection();
        // Directly apply new window size
        if(currentDisplayMode == DisplayMode::Windowed && VIDEODRIVER.getDisplayMode() == DisplayMode::Windowed)
        {
            VIDEODRIVER.ResizeScreen(windowSizes_[selection - 1], DisplayMode::Windowed);
        } else
            closeBehavior_ = CloseBehavior::Custom;
    } else
        closeBehavior_ = CloseBehavior::Custom;
}

void iwSettings::msgScreenResize(const ScreenResizeEvent& sr)
{
    IngameWindow::msgScreenResize(sr);
    updateWindowSizeComboBox();
}

void iwSettings::updateWindowSizeComboBox()
{
    auto* cbWindowSize = getCtrl<ctrlGroup>(ID_grpWindowSize)->getCtrl<ctrlComboBox>(ID_cbWindowSize);

    const VideoMode currentSize =
      VIDEODRIVER.getDisplayMode() == DisplayMode::Windowed ? VIDEODRIVER.getWindowSize() : SETTINGS.video.windowedSize;
    cbWindowSize->setText(0, helpers::format("%ux%u", currentSize.width, currentSize.height));
    // Not found == -1 -> First entry selected
    cbWindowSize->setSelection(static_cast<unsigned>(helpers::indexOf(windowSizes_, currentSize) + 1));
}

void iwSettings::updateResolutionGroups()
{
    const DisplayMode currentDisplayMode = getDisplayModeSelection();
    getCtrl<ctrlGroup>(ID_grpResolution)->setVisible(currentDisplayMode == DisplayMode::Fullscreen);
    getCtrl<ctrlGroup>(ID_grpWindowSize)->setVisible(currentDisplayMode == DisplayMode::Windowed);
}

DisplayMode iwSettings::getDisplayModeSelection() const
{
    return static_cast<DisplayMode>(getCtrl<ctrlComboBox>(ID_cbDisplayMode)->getSelection().value());
}
