// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

iwSettings::iwSettings()
    : IngameWindow(CGI_SETTINGS, IngameWindow::posLastOrCenter, Extent(370, 172), _("Settings"),
                   LOADER.GetImageN("resource", 41))
{
    AddText(46, DrawPoint(15, 40), _("Fullscreen resolution:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddText(47, DrawPoint(15, 85), _("Mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddCheckBox(4, DrawPoint(200, 124), Extent(150, 26), TextureColor::Grey, _("Statistics Scale"), NormalFont, false);
    GetCtrl<ctrlCheck>(4)->SetCheck(SETTINGS.ingame.scale_statistics);

    // "Vollbild"
    ctrlOptionGroup* optiongroup = AddOptionGroup(3, GroupSelectType::Check);
    optiongroup->AddTextButton(1, DrawPoint(200, 70), Extent(150, 22), TextureColor::Grey, _("Fullscreen"), NormalFont);
    optiongroup->AddTextButton(2, DrawPoint(200, 95), Extent(150, 22), TextureColor::Grey, _("Windowed"), NormalFont);

    // "Vollbild" setzen
    optiongroup = GetCtrl<ctrlOptionGroup>(3);
    optiongroup->SetSelection((SETTINGS.video.fullscreen ? 1 : 2)); //-V807
    VIDEODRIVER.ListVideoModes(video_modes);

    // "Auflösung"
    AddComboBox(0, DrawPoint(200, 35), Extent(150, 22), TextureColor::Grey, NormalFont, 110);

    // Und zu der Combobox hinzufügen
    for(unsigned i = 0; i < video_modes.size(); ++i)
    {
        // >=800x600, alles andere macht keinen Sinn
        if(video_modes[i].width >= 800 && video_modes[i].height >= 600)
        {
            GetCtrl<ctrlComboBox>(0)->AddString(helpers::format("%ux%u", video_modes[i].width, video_modes[i].height));

            // Ist das die aktuelle Auflösung? Dann selektieren
            if(video_modes[i] == SETTINGS.video.fullscreenSize)
                GetCtrl<ctrlComboBox>(0)->SetSelection(i);
        } else
        {
            video_modes.erase(video_modes.begin() + i);
            --i;
        }
    }
}

iwSettings::~iwSettings()
{
    try
    {
        auto* SizeCombo = GetCtrl<ctrlComboBox>(0);
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
        case 3: SETTINGS.video.fullscreen = (selection == 1); break;
    }
}

void iwSettings::Msg_CheckboxChange(const unsigned ctrl_id, const bool checked)
{
    switch(ctrl_id)
    {
        case 4:
        {
            SETTINGS.ingame.scale_statistics = checked;
            break;
        }
    }
}
