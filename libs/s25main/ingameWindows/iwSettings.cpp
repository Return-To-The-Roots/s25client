// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
    AddCheckBox(4, DrawPoint(200, 124), Extent(150, 26), TC_GREY, _("Statistics Scale"), NormalFont, false);
    GetCtrl<ctrlCheck>(4)->SetCheck(SETTINGS.ingame.scale_statistics);

    // "Vollbild"
    ctrlOptionGroup* optiongroup = AddOptionGroup(3, GroupSelectType::Check);
    optiongroup->AddTextButton(1, DrawPoint(200, 70), Extent(150, 22), TC_GREY, _("Fullscreen"), NormalFont);
    optiongroup->AddTextButton(2, DrawPoint(200, 95), Extent(150, 22), TC_GREY, _("Windowed"), NormalFont);

    // "Vollbild" setzen
    optiongroup = GetCtrl<ctrlOptionGroup>(3);
    optiongroup->SetSelection((SETTINGS.video.fullscreen ? 1 : 2)); //-V807
    VIDEODRIVER.ListVideoModes(video_modes);

    // "Auflösung"
    AddComboBox(0, DrawPoint(200, 35), Extent(150, 22), TC_GREY, NormalFont, 110);

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
