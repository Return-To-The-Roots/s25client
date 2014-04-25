// $Id: iwSettings.h
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "iwSettings.h"

#include "Settings.h"
#include "Loader.h"
#include "controls.h"
#include "GameClient.h"
#include "GlobalVars.h"
#include "dskGameInterface.h"

#include "VideoDriverWrapper.h"
#include "AudioDriverWrapper.h"

#include "iwTextfile.h"
#include "iwMsgbox.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwSettings.
 *
 *  @author NastX
 */
iwSettings::iwSettings(dskGameInterface* gameDesktop)
    : IngameWindow(CGI_SETTINGS, 0xFFFF, 0xFFFF, 370, 172, _("Settings"), LOADER.GetImageN("resource", 41)),
      gameDesktop(gameDesktop)
{
    AddText(  46,  15,  40, _("Fullscreen resolution:"), COLOR_YELLOW, 0, NormalFont);
    AddText(  47,  15,  85, _("Mode:"), COLOR_YELLOW, 0, NormalFont);
    AddCheckBox(4, 200, 124, 150, 26, TC_GREY, _("Statistics Scale"), NormalFont, false);
    GetCtrl<ctrlCheck>(4)->SetCheck(Settings::inst().ingame.scale_statistics);

    // "Vollbild"
    ctrlOptionGroup* optiongroup = AddOptionGroup(10, ctrlOptionGroup::CHECK, scale);
    optiongroup = AddOptionGroup(3, ctrlOptionGroup::CHECK, scale);
    optiongroup->AddTextButton(1, 200, 70, 150, 22, TC_GREY, _("Fullscreen"), NormalFont);
    optiongroup->AddTextButton(2, 200, 95, 150, 22, TC_GREY, _("Windowed"), NormalFont);

    // "Vollbild" setzen
    optiongroup = GetCtrl<ctrlOptionGroup>(3);
    optiongroup->SetSelection( (SETTINGS.video.fullscreen ? 1 : 2) );
    VideoDriverWrapper::inst().ListVideoModes(video_modes);

    // "Auflösung"
    AddComboBox(0, 200, 35, 150, 22, TC_GREY, NormalFont, 110);

    // Und zu der Combobox hinzufügen
    for(unsigned i = 0; i < video_modes.size(); ++i)
    {
        // >=800x600, alles andere macht keinen Sinn
        if(video_modes[i].width >= 800 && video_modes[i].height >= 600)
        {
            char str[64];
            sprintf(str, "%ux%u", video_modes[i].width, video_modes[i].height);

            GetCtrl<ctrlComboBox>(0)->AddString(str);

            // Ist das die aktuelle Auflösung? Dann selektieren
            if(video_modes[i].width == SETTINGS.video.fullscreen_width &&
                    video_modes[i].height == SETTINGS.video.fullscreen_height)
                GetCtrl<ctrlComboBox>(0)->SetSelection(i);
        }
        else
        {
            video_modes.erase(video_modes.begin() + i);
            --i;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author NastX
 */
iwSettings::~iwSettings()
{
    ctrlComboBox* SizeCombo = GetCtrl<ctrlComboBox>(0);
    SETTINGS.video.fullscreen_width = video_modes[SizeCombo->GetSelection()].width;
    SETTINGS.video.fullscreen_height = video_modes[SizeCombo->GetSelection()].height;

    // Auflösung/Vollbildmodus geändert?
#ifdef _WIN32
    if((SETTINGS.video.fullscreen_width != VideoDriverWrapper::inst().GetScreenWidth()
            ||
            SETTINGS.video.fullscreen_height != VideoDriverWrapper::inst().GetScreenHeight()
            || SETTINGS.video.fullscreen != VideoDriverWrapper::inst().IsFullscreen()))
    {
        if(!VideoDriverWrapper::inst().ResizeScreen(SETTINGS.video.fullscreen_width,
                SETTINGS.video.fullscreen_height,
                SETTINGS.video.fullscreen))
        {
            // WindowManager::inst().Show(new iwMsgbox(_("Sorry!"), _("You need to restart your game to change the screen resolution!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
        }
    }
#else
    if((SETTINGS.video.fullscreen &&
            (SETTINGS.video.fullscreen_width != VideoDriverWrapper::inst().GetScreenWidth()
             ||
             SETTINGS.video.fullscreen_height != VideoDriverWrapper::inst().GetScreenHeight())
       ) || SETTINGS.video.fullscreen != VideoDriverWrapper::inst().IsFullscreen())
    {
        if(!VideoDriverWrapper::inst().ResizeScreen(SETTINGS.video.fullscreen ? SETTINGS.video.fullscreen_width : SETTINGS.video.windowed_width,
                SETTINGS.video.fullscreen ? SETTINGS.video.fullscreen_height : SETTINGS.video.windowed_height,
                SETTINGS.video.fullscreen))
        {
            // WindowManager::inst().Show(new iwMsgbox(_("Sorry!"), _("You need to restart your game to change the screen resolution!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
        }
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author NastX
 */
void iwSettings::Msg_OptionGroupChange(const unsigned int ctrl_id, const unsigned short selection)
{
    switch(ctrl_id)
    {
        case 3:
            switch(selection)
            {
                case 1:
                    SETTINGS.video.fullscreen = true;
                    break;
                case 2:
                    SETTINGS.video.fullscreen = false;
                    break;
            }
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author NastX
 */
void iwSettings::Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked)
{
    switch(ctrl_id)
    {
        case 4:
        {
            SETTINGS.ingame.scale_statistics = checked;
            break;
        }
        case 5:
        {
            break;
        }
    }
}
