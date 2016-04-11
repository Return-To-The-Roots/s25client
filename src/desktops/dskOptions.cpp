// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "dskOptions.h"

#include "GlobalGameSettings.h"
#include "WindowManager.h"
#include "Loader.h"

#include "Settings.h"
#include "GlobalVars.h"

#include "dskMainMenu.h"

#include "languages.h"

#include "drivers/VideoDriverWrapper.h"
#include "drivers/AudioDriverWrapper.h"
#include "MusicPlayer.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "ingameWindows/iwAddons.h"
#include "ingameWindows/iwTextfile.h"
#include "ingameWindows/iwMsgbox.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlProgress.h"
#include "ogl/glArchivItem_Font.h"
#include "helpers/mathFuncs.h"
#include "helpers/converters.h"
#include "ExtensionList.h"
#include "libutil/src/colors.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

///////////////////////////////////////////////////////////////////////////////
/** @class dskOptions
 *
 *  Klasse des Optionen Desktops.
 *
 *  @author OLiver
 */

dskOptions::dskOptions() : Desktop(LOADER.GetImageN("setup013", 0))
{
    // Zurück
    AddTextButton(0, 300, 550, 200, 22,   TC_RED1, _("Back"), NormalFont);

    // "Optionen"
    AddText(1, 400, 10, _("Options"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

    ctrlOptionGroup* optiongroup = AddOptionGroup(10, ctrlOptionGroup::CHECK, scale_);

    AddTextButton(14, 520, 550, 200, 22, TC_GREEN2, _("Addons"), NormalFont);

    // "Allgemein"
    optiongroup->AddTextButton(11,  80, 510, 200, 22, TC_GREEN2, _("Common"), NormalFont);
    // "Grafik"
    optiongroup->AddTextButton(12, 300, 510, 200, 22, TC_GREEN2, _("Graphics"), NormalFont);
    // "Sound"
    optiongroup->AddTextButton(13, 520, 510, 200, 22, TC_GREEN2, _("Sound/Music"), NormalFont);

    ctrlGroup* groupAllgemein = AddGroup(21, scale_);
    ctrlGroup* groupGrafik = AddGroup(22, scale_);
    ctrlGroup* groupSound = AddGroup(23, scale_);
    ctrlComboBox* combo;

    // Allgemein
    // {

    // "Name"
    groupAllgemein->AddText(30, 80, 80, _("Name in Game:"), COLOR_YELLOW, 0, NormalFont);
    ctrlEdit* name = groupAllgemein->AddEdit(31, 280, 75, 190, 22, TC_GREY, NormalFont, 15);
    name->SetText(SETTINGS.lobby.name);

    // "Sprache"
    groupAllgemein->AddText(32, 80, 130, _("Language:"), COLOR_YELLOW, 0, NormalFont);
    combo = groupAllgemein->AddComboBox(33, 280, 125, 190, 20, TC_GREY, NormalFont, 100);

    bool selected = false;
    for(unsigned i = 0 ; i < LANGUAGES.getCount(); ++i)
    {
        const Languages::Language l = LANGUAGES.getLanguage(i);

        combo->AddString(_(l.name));
        if(SETTINGS.language.language == l.code )
        {
            combo->SetSelection(static_cast<unsigned short>(i));
            selected = true;
        }
    }
    if(!selected)
        combo->SetSelection(0);

    // Tastaturlayout
    groupAllgemein->AddText(34, 80, 180, _("Keyboard layout:"), COLOR_YELLOW, 0, NormalFont);
    groupAllgemein->AddTextButton(35, 280, 175, 120, 22, TC_GREY, _("Readme"), NormalFont);

    // IPv4/6
    groupAllgemein->AddText(300, 80, 230, _("Use IPv6:"), COLOR_YELLOW, 0, NormalFont);

    ctrlOptionGroup* ipv6 = groupAllgemein->AddOptionGroup(301, ctrlOptionGroup::CHECK, scale_);
    ipv6->AddTextButton(302, 480, 225, 190, 22, TC_GREY, _("IPv6"), NormalFont);
    ipv6->AddTextButton(303, 280, 225, 190, 22, TC_GREY, _("IPv4"), NormalFont);
    ipv6->SetSelection( (SETTINGS.server.ipv6 ? 302 : 303) );

    // ipv6-feld ggf (de-)aktivieren
    ipv6->GetCtrl<ctrlTextButton>(302)->Enable( (SETTINGS.proxy.typ != 4 && SETTINGS.proxy.typ != 40) ); //-V807

    // Proxyserver
    groupAllgemein->AddText(36, 80, 280, _("Proxyserver:"), COLOR_YELLOW, 0, NormalFont);
    ctrlEdit* proxy = groupAllgemein->AddEdit(37, 280, 275, 190, 22, TC_GREY, NormalFont);
    proxy->SetText(SETTINGS.proxy.proxy);
    proxy = groupAllgemein->AddEdit(371, 480, 275, 50, 22, TC_GREY, NormalFont, 5);
    proxy->SetText(SETTINGS.proxy.port);

    // Proxytyp
    groupAllgemein->AddText(38, 80, 310, _("Proxytyp:"), COLOR_YELLOW, 0, NormalFont);
    combo = groupAllgemein->AddComboBox(39, 280, 305, 390, 20, TC_GREY, NormalFont, 100);
    combo->AddString(_("No Proxy"));
    combo->AddString(_("Socks v4"));

    // TODO: not implemented
    //combo->AddString(_("Socks v5"));

    // und auswählen
    switch(SETTINGS.proxy.typ)
    {
        default:    {   combo->SetSelection(0); } break;
        case 4:     {   combo->SetSelection(1); } break;
        case 5:     {   combo->SetSelection(2); } break;
    }

    // }

    groupAllgemein->AddText(  70,  80, 360, _("Submit debug data:"), COLOR_YELLOW, 0, NormalFont);
    optiongroup = groupAllgemein->AddOptionGroup(71, ctrlOptionGroup::CHECK, scale_);
    optiongroup->AddTextButton(72, 480, 355, 190, 22, TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(73, 280, 355, 190, 22, TC_GREY, _("Off"), NormalFont);

    optiongroup->SetSelection( ((SETTINGS.global.submit_debug_data == 1) ? 72 : 73) );

    // qx:upnp switch
    groupAllgemein->AddText(9999, 80, 390, _("Use UPnP"), COLOR_YELLOW, 0, NormalFont);
    ctrlOptionGroup* upnp = groupAllgemein->AddOptionGroup(9998, ctrlOptionGroup::CHECK, scale_);
    upnp->AddTextButton(10002, 280, 385, 190, 22, TC_GREY, _("Off"), NormalFont);
    upnp->AddTextButton(10001, 480, 385, 190, 22, TC_GREY, _("On"), NormalFont);
    upnp->SetSelection( (SETTINGS.global.use_upnp == 1) ? 10001 : 10002 );

    groupAllgemein->AddText(10100, 80, 420, _("Smart Cursor"), COLOR_YELLOW, 0, NormalFont);
    ctrlOptionGroup* smartCursor = groupAllgemein->AddOptionGroup(10101, ctrlOptionGroup::CHECK, scale_);
    smartCursor->AddTextButton(10103, 280, 415, 190, 22, TC_GREY, _("Off"), NormalFont,
        _("Don't move cursor automatically\nUseful e.g. for split-screen / dual-mice multiplayer (see wiki)"));
    smartCursor->AddTextButton(10102, 480, 415, 190, 22, TC_GREY, _("On"), NormalFont,
        _("Place cursor on default button for new dialogs / action windows (default)"));
    smartCursor->SetSelection(SETTINGS.global.smartCursor ? 10102 : 10103);

    if(!GLOBALVARS.ext_vbo) // VBO unterstützt?
        optiongroup->AddText(  56, 280, 230, _("not supported"), COLOR_YELLOW, 0, NormalFont);
    else
        optiongroup->AddTextButton(56, 280, 225, 190, 22, TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(57, 480, 225, 190, 22, TC_GREY, _("Off"), NormalFont);

    // "Auflösung"
    groupGrafik->AddText(  40,  80, 80, _("Fullscreen resolution:"), COLOR_YELLOW, 0, NormalFont);
    groupGrafik->AddComboBox(41, 280, 75, 190, 22, TC_GREY, NormalFont, 150);

    // "Vollbild"
    groupGrafik->AddText(  46,  80, 130, _("Mode:"), COLOR_YELLOW, 0, NormalFont);
    optiongroup = groupGrafik->AddOptionGroup(47, ctrlOptionGroup::CHECK, scale_);
    optiongroup->AddTextButton(48, 480, 125, 190, 22, TC_GREY, _("Fullscreen"), NormalFont);
    optiongroup->AddTextButton(49, 280, 125, 190, 22, TC_GREY, _("Windowed"), NormalFont);

    // "VSync"
    groupGrafik->AddText(  50,  80, 180, _("Limit Framerate:"), COLOR_YELLOW, 0, NormalFont);
    groupGrafik->AddComboBox(51, 280, 175, 390, 22, TC_GREY, NormalFont, 150);

    // "VBO"
    groupGrafik->AddText(  54,  80, 230, _("Vertex Buffer Objects:"), COLOR_YELLOW, 0, NormalFont);
    optiongroup = groupGrafik->AddOptionGroup(55, ctrlOptionGroup::CHECK, scale_);

    if(!GLOBALVARS.ext_vbo) // VBO unterstützt?
        optiongroup->AddText(  56, 280, 230, _("not supported"), COLOR_YELLOW, 0, NormalFont);
    else
        optiongroup->AddTextButton(56, 280, 225, 190, 22, TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(57, 480, 225, 190, 22, TC_GREY, _("Off"), NormalFont);

    // "Grafiktreiber"
    groupGrafik->AddText(58, 80, 275, _("Graphics Driver"), COLOR_YELLOW, 0, NormalFont);
    combo = groupGrafik->AddComboBox(59, 280, 275, 390, 20, TC_GREY, NormalFont, 100);

    std::vector<DriverWrapper::DriverItem> video_drivers = DriverWrapper::LoadDriverList(DriverWrapper::DT_VIDEO);

    for(std::vector<DriverWrapper::DriverItem>::iterator it = video_drivers.begin(); it != video_drivers.end(); ++it)
    {
        combo->AddString(it->GetName());
        if(it->GetName() == SETTINGS.driver.video)
            combo->SetSelection(combo->GetCount() - 1);
    }

    groupGrafik->AddText(  74,  80, 320, _("Optimized Textures:"), COLOR_YELLOW, 0, NormalFont);
    optiongroup = groupGrafik->AddOptionGroup(75, ctrlOptionGroup::CHECK, scale_);

    optiongroup->AddTextButton(76, 280, 315, 190, 22, TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(77, 480, 315, 190, 22, TC_GREY, _("Off"), NormalFont);


    // "Audiotreiber"
    groupSound->AddText(60,  80, 230, _("Sounddriver"), COLOR_YELLOW, 0, NormalFont);
    combo = groupSound->AddComboBox(61, 280, 225, 390, 20, TC_GREY, NormalFont, 100);

    std::vector<DriverWrapper::DriverItem> audio_drivers = DriverWrapper::LoadDriverList(DriverWrapper::DT_AUDIO);

    for(std::vector<DriverWrapper::DriverItem>::iterator it = audio_drivers.begin(); it != audio_drivers.end(); ++it)
    {
        combo->AddString(it->GetName());
        if(it->GetName() == SETTINGS.driver.audio)
            combo->SetSelection(combo->GetCount() - 1);
    }

    // Musik
    groupSound->AddText(  62,  80, 80, _("Music"), COLOR_YELLOW, 0, NormalFont);
    optiongroup = groupSound->AddOptionGroup(63, ctrlOptionGroup::CHECK, scale_);
    optiongroup->AddTextButton(64, 280, 75, 90, 22, TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(65, 380, 75, 90, 22, TC_GREY, _("Off"), NormalFont);

    ctrlProgress* Mvolume = groupSound->AddProgress(72, 480, 75, 190, 22, TC_GREY, 139, 138, 10);
    Mvolume->SetPosition(SETTINGS.sound.musik_volume * 10 / 255); //-V807

    // Effekte
    groupSound->AddText(  66,  80, 130, _("Effects"), COLOR_YELLOW, 0, NormalFont);
    optiongroup = groupSound->AddOptionGroup(67, ctrlOptionGroup::CHECK, scale_);
    optiongroup->AddTextButton(68, 280, 125, 90, 22, TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(69, 380, 125, 90, 22, TC_GREY, _("Off"), NormalFont);

    ctrlProgress* FXvolume = groupSound->AddProgress(70, 480, 125, 190, 22, TC_GREY, 139, 138, 10);
    FXvolume->SetPosition(SETTINGS.sound.effekte_volume * 10 / 255);

    // Musicplayer-Button
    groupSound->AddTextButton(71, 280, 175, 190, 22, TC_GREY, _("Music player"), NormalFont);

    // "Allgemein" auswählen
    optiongroup = GetCtrl<ctrlOptionGroup>(10);
    optiongroup->SetSelection(11, true);


    // Grafik
    // {

    loadVideoModes();

    // Und zu der Combobox hinzufügen
    ctrlComboBox& cbVideoModes = *groupGrafik->GetCtrl<ctrlComboBox>(41);
    for(std::vector<VideoMode>::const_iterator it = video_modes.begin(); it != video_modes.end(); ++it)
    {
        VideoMode ratio = getAspectRatio(*it);
        std::stringstream str;
        str << it->width << "x" << it->height;
        // Make the length always the same as 'iiiixiiii' to align the ratio
        int len = str.str().length();
        for(int i = len; i < 4+1+4; i++)
            str << " ";
        str << " (" << ratio.width << ":" << ratio.height << ")";

        cbVideoModes.AddString(str.str());

        // Ist das die aktuelle Auflösung? Dann selektieren
        if(*it == VideoMode(SETTINGS.video.fullscreen_width, SETTINGS.video.fullscreen_height)) //-V807
            cbVideoModes.SetSelection(cbVideoModes.GetCount() - 1);
    }

    // "Vollbild" setzen
    optiongroup = groupGrafik->GetCtrl<ctrlOptionGroup>(47);
    optiongroup->SetSelection( (SETTINGS.video.fullscreen ? 48 : 49) ); //-V807

    // "Limit Framerate" füllen
    ctrlComboBox* cbFrameRate = groupGrafik->GetCtrl<ctrlComboBox>(51);
    for(unsigned char i = 0; i < Settings::SCREEN_REFRESH_RATES_COUNT; ++i)
    {
        switch(Settings::SCREEN_REFRESH_RATES[i])
        {
            case 0:
            {
                cbFrameRate->AddString(_("Disabled"));
                cbFrameRate->SetSelection(0);
            } break;
            case 1:
            {
                if(GLOBALVARS.ext_swapcontrol)
                    cbFrameRate->AddString(_("Dynamic (Limits to display refresh rate, works with most drivers)"));
                if(SETTINGS.video.vsync == 1)
                    cbFrameRate->SetSelection(1);
            } break;
            default:
            {
                // frameratebegrenzungen mit Bildabstand kleiner 13ms
                // wird unter windows nicht mehr aufgelöst
#ifdef _WIN32
                if(960 / Settings::SCREEN_REFRESH_RATES[i] > 13)
#endif // _WIN32
                {
                    cbFrameRate->AddString(helpers::toString(Settings::SCREEN_REFRESH_RATES[i]) + " FPS");
                }

                if(SETTINGS.video.vsync == Settings::SCREEN_REFRESH_RATES[i])
                    cbFrameRate->SetSelection(i - (GLOBALVARS.ext_swapcontrol ? 0 : 1));
            } break;
        }
    }

    // "VBO" setzen
    optiongroup = groupGrafik->GetCtrl<ctrlOptionGroup>(55);
    if(GLOBALVARS.ext_vbo)
        optiongroup->SetSelection( (SETTINGS.video.vbo ? 56 : 57) );
    else
        optiongroup->SetSelection(57);

    optiongroup = groupGrafik->GetCtrl<ctrlOptionGroup>(75);
    optiongroup->SetSelection( (SETTINGS.video.shared_textures ? 76 : 77) );
    // }

    // Sound
    // {

    // "Musik" setzen
    optiongroup = groupSound->GetCtrl<ctrlOptionGroup>(63);
    optiongroup->SetSelection( (SETTINGS.sound.musik ? 64 : 65) );

    // "Effekte" setzen
    optiongroup = groupSound->GetCtrl<ctrlOptionGroup>(67);
    optiongroup->SetSelection( (SETTINGS.sound.effekte ? 68 : 69) );

    // }
}

dskOptions::~dskOptions()
{
    // Save settings
    ggs.SaveSettings();
}

void dskOptions::Msg_Group_ProgressChange(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const unsigned short position)
{
    switch(ctrl_id)
    {
        case 70:
        {
            SETTINGS.sound.effekte_volume = (unsigned char)position * 255 / 10 + (position < 10 ? 1 : 0);
            AUDIODRIVER.SetMasterEffectVolume(SETTINGS.sound.effekte_volume);
        } break;
        case 72:
        {
            SETTINGS.sound.musik_volume = (unsigned char)position * 255 / 10 + (position < 10 ? 1 : 0);
            AUDIODRIVER.SetMasterMusicVolume(SETTINGS.sound.musik_volume);
        } break;
    }
}

void dskOptions::Msg_Group_ComboSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const int selection)
{
    ctrlGroup* group = GetCtrl<ctrlGroup>(group_id);
    ctrlComboBox* combo = group->GetCtrl<ctrlComboBox>(ctrl_id);

    switch(ctrl_id)
    {
        case 33: // Sprache
        {
            // Language changed?
            std::string old_lang = SETTINGS.language.language; //-V807
            SETTINGS.language.language = LANGUAGES.setLanguage(selection);
            if(SETTINGS.language.language != old_lang)
                WINDOWMANAGER.Switch(new dskOptions);
        } break;
        case 39: // Proxy
        {
            switch(selection)
            {
                case 0: SETTINGS.proxy.typ = 0; break;
                case 1: SETTINGS.proxy.typ = 4; break;
                case 2: SETTINGS.proxy.typ = 5; break;
            }

            // ipv6 gleich sichtbar deaktivieren
            if(SETTINGS.proxy.typ == 4 && SETTINGS.server.ipv6)
            {
                GetCtrl<ctrlGroup>(21)->GetCtrl<ctrlOptionGroup>(301)->SetSelection(303);
                GetCtrl<ctrlGroup>(21)->GetCtrl<ctrlOptionGroup>(301)->GetCtrl<ctrlTextButton>(302)->Enable(false);
                SETTINGS.server.ipv6 = false;
            }

            if(SETTINGS.proxy.typ != 4)
                GetCtrl<ctrlGroup>(21)->GetCtrl<ctrlOptionGroup>(301)->GetCtrl<ctrlTextButton>(302)->Enable(true);
        } break;
        case 41: // Auflösung
        {
            SETTINGS.video.fullscreen_width = video_modes[selection].width;
            SETTINGS.video.fullscreen_height = video_modes[selection].height;
        } break;
        case 51: // Limit Framerate
        {
            // 0: aus
            // 1: vsync, wenn verfügbar, ansonsten schon eine Framerate
            // 2: Framerates
            switch(selection)
            {
                case 0:
                {
                    SETTINGS.video.vsync = 0;
                } break;
                case 1:
                {
                    SETTINGS.video.vsync = (GLOBALVARS.ext_swapcontrol ? 1 : Settings::SCREEN_REFRESH_RATES[2]);
                }   break;
                default:
                {
                    SETTINGS.video.vsync = Settings::SCREEN_REFRESH_RATES[selection + (GLOBALVARS.ext_swapcontrol ? 0 : 1)];
                }   break;
            }

            if(GLOBALVARS.ext_swapcontrol)
                wglSwapIntervalEXT((SETTINGS.video.vsync == 1));
        } break;
        case 59: // Videotreiber
        {
            SETTINGS.driver.video = combo->GetText(selection);
        } break;
        case 61: // Audiotreiber
        {
            SETTINGS.driver.audio =  combo->GetText(selection);
        } break;
    }
}

void dskOptions::Msg_Group_OptionGroupChange(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        case 301: // IPv6 Ja/Nein
        {
            switch(selection)
            {
                case 302: SETTINGS.server.ipv6 = true;  break;
                case 303: SETTINGS.server.ipv6 = false; break;
            }
        } break;
        case 47: // Vollbild
        {
            switch(selection)
            {
                case 48: SETTINGS.video.fullscreen = true;  break;
                case 49: SETTINGS.video.fullscreen = false; break;
            }
        } break;
        case 55: // VBO
        {
            switch(selection)
            {
                case 56: SETTINGS.video.vbo = true;  break;
                case 57: SETTINGS.video.vbo = false; break;
            }
        } break;
        case 75:
        {
            switch(selection)
            {
                case 76: SETTINGS.video.shared_textures = true;  break;
                case 77: SETTINGS.video.shared_textures = false; break;
            }
        } break;

        case 63: // Musik
        {
            switch(selection)
            {
                case 64: SETTINGS.sound.musik = true;  break;
                case 65: SETTINGS.sound.musik = false; break;
            }
            if(SETTINGS.sound.musik)
                MUSICPLAYER.Play();
            else
                MUSICPLAYER.Stop();
        } break;
        case 67: // Soundeffekte
        {
            switch(selection)
            {
                case 68: SETTINGS.sound.effekte = true;  break;
                case 69: SETTINGS.sound.effekte = false; break;
            }
        } break;
        case 71: // Submit debug data
        {
            switch(selection)
            {
                case 72: SETTINGS.global.submit_debug_data = 1; break;
                case 73: SETTINGS.global.submit_debug_data = 2; break;
            }
        } break;
        case 9998:
        {
            switch(selection)
            {
                case 10001: SETTINGS.global.use_upnp = 1; break;
                case 10002: SETTINGS.global.use_upnp = 0; break;
            }
        } break;
        case 10101:
        {
            switch(selection)
            {
                case 10102: SETTINGS.global.smartCursor = true; break;
                case 10103: SETTINGS.global.smartCursor = false; break;
            }
        } break;
    }
}

void dskOptions::Msg_OptionGroupChange(const unsigned int ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        case 10: // Optionengruppen anzeigen
        {
            for(unsigned short i = 21; i < 24; ++i)
                GetCtrl<ctrlGroup>(i)->SetVisible(i == selection + 10);
        } break;
    }
}

void dskOptions::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // "Zurück"
        {
            ctrlGroup* groupAllgemein = GetCtrl<ctrlGroup>(21);

            // Name abspeichern
            SETTINGS.lobby.name = groupAllgemein->GetCtrl<ctrlEdit>(31)->GetText();
            // Proxy abspeichern, überprüfung der einstellung übernimmt SETTINGS.Save()
            SETTINGS.proxy.proxy = groupAllgemein->GetCtrl<ctrlEdit>(37)->GetText();
            SETTINGS.proxy.port = atoi(groupAllgemein->GetCtrl<ctrlEdit>(371)->GetText().c_str());

            SETTINGS.Save();

            // Auflösung/Vollbildmodus geändert?
#ifdef _WIN32
            if(SETTINGS.video.fullscreen_width  != VIDEODRIVER.GetScreenWidth() ||  //-V807
               SETTINGS.video.fullscreen_height != VIDEODRIVER.GetScreenHeight() || 
               SETTINGS.video.fullscreen != VIDEODRIVER.IsFullscreen())
            {
                if(!VIDEODRIVER.ResizeScreen(SETTINGS.video.fullscreen_width,
                        SETTINGS.video.fullscreen_height,
                        SETTINGS.video.fullscreen))
                {
                    WINDOWMANAGER.Show(new iwMsgbox(_("Sorry!"), _("You need to restart your game to change the screen resolution!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
                    return;
                }
            }
#else
            if((SETTINGS.video.fullscreen &&
                    (SETTINGS.video.fullscreen_width  != VIDEODRIVER.GetScreenWidth() ||
                     SETTINGS.video.fullscreen_height != VIDEODRIVER.GetScreenHeight())
               ) || SETTINGS.video.fullscreen != VIDEODRIVER.IsFullscreen())
            {
                if(!VIDEODRIVER.ResizeScreen(SETTINGS.video.fullscreen ? SETTINGS.video.fullscreen_width : SETTINGS.video.windowed_width,
                        SETTINGS.video.fullscreen ? SETTINGS.video.fullscreen_height : SETTINGS.video.windowed_height,
                        SETTINGS.video.fullscreen))
                {
                    WINDOWMANAGER.Show(new iwMsgbox(_("Sorry!"), _("You need to restart your game to change the screen resolution!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
                    return;
                }
            }
#endif
            if(SETTINGS.driver.video != VIDEODRIVER.GetName() || SETTINGS.driver.audio != AUDIODRIVER.GetName())
            {
                WINDOWMANAGER.Show(new iwMsgbox(_("Sorry!"), _("You need to restart your game to change the video or audio driver!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
                return;
            }

            WINDOWMANAGER.Switch(new dskMainMenu);
        } break;
        case 14: // Addons
        {
            ggs.LoadSettings();
            WINDOWMANAGER.Show(new iwAddons(ggs));

        } break;
    }
}

void dskOptions::Msg_Group_ButtonClick(const unsigned int  /*group_id*/, const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        default:
            break;
        case 71: // "Music player"
        {
            WINDOWMANAGER.Show(new iwMusicPlayer);
        } break;
        case 35: // "Keyboard Readme"
        {
            WINDOWMANAGER.Show(new iwTextfile("keyboardlayout.txt", _("Keyboard layout") ) );
        } break;
    }
}

void dskOptions::Msg_MsgBoxResult(const unsigned int msgbox_id, const MsgboxResult  /*mbr*/)
{
    switch(msgbox_id)
    {
        default:
            break;
        case 1: // "You need to restart your game ..."
        {
            WINDOWMANAGER.Switch(new dskMainMenu);
        } break;
    }
}

bool dskOptions::cmpVideoModes(const VideoMode& left, const VideoMode& right)
{
    if(left == right)
        return false;
    VideoMode leftRatio = getAspectRatio(left);
    VideoMode rightRatio = getAspectRatio(right);
    // Cmp ratios descending (so 16:9 is above 4:3 as wider ones are more commonly used)
    if(leftRatio.width > rightRatio.width)
        return true;
    else if(leftRatio.width < rightRatio.width)
        return false;
    else if(leftRatio.height > rightRatio.height)
        return true;
    else if(leftRatio.height < rightRatio.height)
        return false;
    // Same ratios -> cmp width/height
    if(left.width < right.width)
        return true;
    else if(left.width > right.width)
        return false;
    else
        return left.height < right.height;
}

void dskOptions::loadVideoModes()
{
    // Get available modes
    VIDEODRIVER.ListVideoModes(video_modes);
    // Remove everything below 800x600
    for(std::vector<VideoMode>::iterator it = video_modes.begin(); it != video_modes.end();)
    {
        if(it->width < 800 && it->height < 600)
            it = video_modes.erase(it);
        else
            ++it;
    }
    // Sort by aspect ratio
    std::sort(video_modes.begin(), video_modes.end(), cmpVideoModes);
}

VideoMode dskOptions::getAspectRatio(const VideoMode& vm)
{
    // First some a bit off values where the aspect ratio is defined by convention
    if(vm == VideoMode(1360, 1024))
        return VideoMode(4, 3);
    else if(vm == VideoMode(1360, 768))
        return VideoMode(16, 9);
    else if(vm == VideoMode(1366, 768))
        return VideoMode(16, 9);

    // Normally Aspect ration is simply width/height as integer numbers (e.g. 4:3)
    int divisor = helpers::gcd(vm.width, vm.height);
    VideoMode ratio(vm.width / divisor, vm.height / divisor);
    // But there are some special cases:
    if(ratio == VideoMode(8, 5))
        return VideoMode(16, 10);
    else if(ratio == VideoMode(5, 3))
        return VideoMode(15, 9);
    else
        return ratio;
}
