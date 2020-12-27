// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "dskOptions.h"
#include "GlobalGameSettings.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlProgress.h"
#include "driver/VideoDriver.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "dskMainMenu.h"
#include "helpers/containerUtils.h"
#include "helpers/mathFuncs.h"
#include "helpers/toString.h"
#include "ingameWindows/iwAddons.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "ingameWindows/iwTextfile.h"
#include "languages.h"
#include "ogl/FontStyle.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"
#include <mygettext/mygettext.h>
#include <sstream>

static VideoMode getAspectRatio(const VideoMode& vm)
{
    // First some a bit off values where the aspect ratio is defined by convention
    if(vm == VideoMode(1360, 1024))
        return VideoMode(4, 3);
    else if(vm == VideoMode(1360, 768) || vm == VideoMode(1366, 768))
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

dskOptions::dskOptions() : Desktop(LOADER.GetImageN("setup013", 0))
{
    // Zurück
    AddTextButton(0, DrawPoint(300, 550), Extent(200, 22), TC_RED1, _("Back"), NormalFont);

    // "Optionen"
    AddText(1, DrawPoint(400, 10), _("Options"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    ctrlOptionGroup* optiongroup = AddOptionGroup(10, ctrlOptionGroup::CHECK);

    AddTextButton(14, DrawPoint(520, 550), Extent(200, 22), TC_GREEN2, _("Addons"), NormalFont);

    // "Allgemein"
    optiongroup->AddTextButton(11, DrawPoint(80, 510), Extent(200, 22), TC_GREEN2, _("Common"), NormalFont);
    // "Grafik"
    optiongroup->AddTextButton(12, DrawPoint(300, 510), Extent(200, 22), TC_GREEN2, _("Graphics"), NormalFont);
    // "Sound"
    optiongroup->AddTextButton(13, DrawPoint(520, 510), Extent(200, 22), TC_GREEN2, _("Sound/Music"), NormalFont);

    ctrlGroup* groupAllgemein = AddGroup(21);
    ctrlGroup* groupGrafik = AddGroup(22);
    ctrlGroup* groupSound = AddGroup(23);
    ctrlComboBox* combo;

    // Allgemein
    // {

    // "Name"
    groupAllgemein->AddText(30, DrawPoint(80, 80), _("Name in Game:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* name = groupAllgemein->AddEdit(31, DrawPoint(280, 75), Extent(190, 22), TC_GREY, NormalFont, 15);
    name->SetText(SETTINGS.lobby.name);

    // "Sprache"
    groupAllgemein->AddText(32, DrawPoint(80, 110), _("Language:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupAllgemein->AddComboBox(33, DrawPoint(280, 105), Extent(190, 20), TC_GREY, NormalFont, 100);

    bool selected = false;
    for(unsigned i = 0; i < LANGUAGES.size(); ++i)
    {
        const Language& l = LANGUAGES.getLanguage(i);

        combo->AddString(_(l.name));
        if(SETTINGS.language.language == l.code)
        {
            combo->SetSelection(static_cast<unsigned short>(i));
            selected = true;
        }
    }
    if(!selected)
        combo->SetSelection(0);

    // Tastaturlayout
    groupAllgemein->AddText(34, DrawPoint(80, 150), _("Keyboard layout:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    groupAllgemein->AddTextButton(35, DrawPoint(280, 145), Extent(120, 22), TC_GREY, _("Readme"), NormalFont);

    groupAllgemein->AddText(40, DrawPoint(80, 190), _("Local Port:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* edtPort = groupAllgemein->AddEdit(41, DrawPoint(280, 185), Extent(190, 22), TC_GREY, NormalFont, 15);
    edtPort->SetNumberOnly(true);
    edtPort->SetText(SETTINGS.server.localPort);

    // IPv4/6
    groupAllgemein->AddText(300, DrawPoint(80, 230), _("Use IPv6:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    ctrlOptionGroup* ipv6 = groupAllgemein->AddOptionGroup(301, ctrlOptionGroup::CHECK);
    ipv6->AddTextButton(302, DrawPoint(480, 225), Extent(190, 22), TC_GREY, _("IPv6"), NormalFont);
    ipv6->AddTextButton(303, DrawPoint(280, 225), Extent(190, 22), TC_GREY, _("IPv4"), NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? 302 : 303));

    // ipv6-feld ggf (de-)aktivieren
    ipv6->GetCtrl<ctrlButton>(302)->SetEnabled(SETTINGS.proxy.type != ProxyType::Socks5); //-V807

    // Proxyserver
    groupAllgemein->AddText(36, DrawPoint(80, 280), _("Proxyserver:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* proxy = groupAllgemein->AddEdit(37, DrawPoint(280, 275), Extent(190, 22), TC_GREY, NormalFont);
    proxy->SetText(SETTINGS.proxy.hostname);
    proxy = groupAllgemein->AddEdit(371, DrawPoint(480, 275), Extent(50, 22), TC_GREY, NormalFont, 5);
    proxy->SetNumberOnly(true);
    proxy->SetText(SETTINGS.proxy.port);

    // Proxytyp
    groupAllgemein->AddText(38, DrawPoint(80, 310), _("Proxytyp:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupAllgemein->AddComboBox(39, DrawPoint(280, 305), Extent(390, 20), TC_GREY, NormalFont, 100);
    combo->AddString(_("No Proxy"));
    combo->AddString(_("Socks v4"));

    // TODO: not implemented
    // combo->AddString(_("Socks v5"));

    switch(SETTINGS.proxy.type)
    {
        default: combo->SetSelection(0); break;
        case ProxyType::Socks4: combo->SetSelection(1); break;
        case ProxyType::Socks5: combo->SetSelection(2); break;
    }

    // }

    groupAllgemein->AddText(70, DrawPoint(80, 360), _("Submit debug data:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupAllgemein->AddOptionGroup(71, ctrlOptionGroup::CHECK);
    optiongroup->AddTextButton(72, DrawPoint(480, 355), Extent(190, 22), TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(73, DrawPoint(280, 355), Extent(190, 22), TC_GREY, _("Off"), NormalFont);

    optiongroup->SetSelection(((SETTINGS.global.submit_debug_data == 1) ? 72 : 73)); //-V807

    // qx:upnp switch
    groupAllgemein->AddText(9999, DrawPoint(80, 390), _("Use UPnP"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlOptionGroup* upnp = groupAllgemein->AddOptionGroup(9998, ctrlOptionGroup::CHECK);
    upnp->AddTextButton(10002, DrawPoint(280, 385), Extent(190, 22), TC_GREY, _("Off"), NormalFont);
    upnp->AddTextButton(10001, DrawPoint(480, 385), Extent(190, 22), TC_GREY, _("On"), NormalFont);
    upnp->SetSelection((SETTINGS.global.use_upnp == 1) ? 10001 : 10002);

    groupAllgemein->AddText(10100, DrawPoint(80, 420), _("Smart Cursor"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlOptionGroup* smartCursor = groupAllgemein->AddOptionGroup(10101, ctrlOptionGroup::CHECK);
    smartCursor->AddTextButton(
      10103, DrawPoint(280, 415), Extent(190, 22), TC_GREY, _("Off"), NormalFont,
      _("Don't move cursor automatically\nUseful e.g. for split-screen / dual-mice multiplayer (see wiki)"));
    smartCursor->AddTextButton(10102, DrawPoint(480, 415), Extent(190, 22), TC_GREY, _("On"), NormalFont,
                               _("Place cursor on default button for new dialogs / action windows (default)"));
    smartCursor->SetSelection(SETTINGS.global.smartCursor ? 10102 : 10103);

    // "Auflösung"
    groupGrafik->AddText(40, DrawPoint(80, 80), _("Fullscreen resolution:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    groupGrafik->AddComboBox(41, DrawPoint(280, 75), Extent(190, 22), TC_GREY, NormalFont, 150);

    // "Vollbild"
    groupGrafik->AddText(46, DrawPoint(80, 130), _("Mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupGrafik->AddOptionGroup(47, ctrlOptionGroup::CHECK);
    optiongroup->AddTextButton(48, DrawPoint(480, 125), Extent(190, 22), TC_GREY, _("Fullscreen"), NormalFont);
    optiongroup->AddTextButton(49, DrawPoint(280, 125), Extent(190, 22), TC_GREY, _("Windowed"), NormalFont);

    // "VSync"
    groupGrafik->AddText(50, DrawPoint(80, 180), _("Limit Framerate:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    groupGrafik->AddComboBox(51, DrawPoint(280, 175), Extent(390, 22), TC_GREY, NormalFont, 150);

    // "VBO"
    groupGrafik->AddText(54, DrawPoint(80, 230), _("Vertex Buffer Objects:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupGrafik->AddOptionGroup(55, ctrlOptionGroup::CHECK);

    optiongroup->AddTextButton(56, DrawPoint(280, 225), Extent(190, 22), TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(57, DrawPoint(480, 225), Extent(190, 22), TC_GREY, _("Off"), NormalFont);

    // "Grafiktreiber"
    groupGrafik->AddText(58, DrawPoint(80, 275), _("Graphics Driver"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupGrafik->AddComboBox(59, DrawPoint(280, 275), Extent(390, 20), TC_GREY, NormalFont, 100);

    const auto video_drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Video);

    for(const auto& video_driver : video_drivers)
    {
        combo->AddString(video_driver.GetName());
        if(video_driver.GetName() == SETTINGS.driver.video)
            combo->SetSelection(combo->GetNumItems() - 1);
    }

    groupGrafik->AddText(74, DrawPoint(80, 320), _("Optimized Textures:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupGrafik->AddOptionGroup(75, ctrlOptionGroup::CHECK);

    optiongroup->AddTextButton(76, DrawPoint(280, 315), Extent(190, 22), TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(77, DrawPoint(480, 315), Extent(190, 22), TC_GREY, _("Off"), NormalFont);

    // "Audiotreiber"
    groupSound->AddText(60, DrawPoint(80, 230), _("Sounddriver"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupSound->AddComboBox(61, DrawPoint(280, 225), Extent(390, 20), TC_GREY, NormalFont, 100);

    const auto audio_drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Audio);

    for(const auto& audio_driver : audio_drivers)
    {
        combo->AddString(audio_driver.GetName());
        if(audio_driver.GetName() == SETTINGS.driver.audio)
            combo->SetSelection(combo->GetNumItems() - 1);
    }

    // Musik
    groupSound->AddText(62, DrawPoint(80, 80), _("Music"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupSound->AddOptionGroup(63, ctrlOptionGroup::CHECK);
    optiongroup->AddTextButton(64, DrawPoint(280, 75), Extent(90, 22), TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(65, DrawPoint(380, 75), Extent(90, 22), TC_GREY, _("Off"), NormalFont);

    ctrlProgress* Mvolume = groupSound->AddProgress(72, DrawPoint(480, 75), Extent(190, 22), TC_GREY, 139, 138, 100);
    Mvolume->SetPosition((SETTINGS.sound.musicVolume * 100) / 255); //-V807

    // Effekte
    groupSound->AddText(66, DrawPoint(80, 130), _("Effects"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupSound->AddOptionGroup(67, ctrlOptionGroup::CHECK);
    optiongroup->AddTextButton(68, DrawPoint(280, 125), Extent(90, 22), TC_GREY, _("On"), NormalFont);
    optiongroup->AddTextButton(69, DrawPoint(380, 125), Extent(90, 22), TC_GREY, _("Off"), NormalFont);

    ctrlProgress* FXvolume = groupSound->AddProgress(70, DrawPoint(480, 125), Extent(190, 22), TC_GREY, 139, 138, 100);
    FXvolume->SetPosition((SETTINGS.sound.effectsVolume * 100) / 255);

    // Musicplayer-Button
    groupSound->AddTextButton(71, DrawPoint(280, 175), Extent(190, 22), TC_GREY, _("Music player"), NormalFont);

    // "Allgemein" auswählen
    optiongroup = GetCtrl<ctrlOptionGroup>(10);
    optiongroup->SetSelection(11, true);

    // Grafik
    // {

    loadVideoModes();

    // Und zu der Combobox hinzufügen
    ctrlComboBox& cbVideoModes = *groupGrafik->GetCtrl<ctrlComboBox>(41);
    for(const auto& videoMode : video_modes)
    {
        VideoMode ratio = getAspectRatio(videoMode);
        s25util::ClassicImbuedStream<std::ostringstream> str;
        str << videoMode.width << "x" << videoMode.height;
        // Make the length always the same as 'iiiixiiii' to align the ratio
        int len = str.str().length();
        for(int i = len; i < 4 + 1 + 4; i++)
            str << " ";
        str << " (" << ratio.width << ":" << ratio.height << ")";

        cbVideoModes.AddString(str.str());

        // Ist das die aktuelle Auflösung? Dann selektieren
        if(videoMode == SETTINGS.video.fullscreenSize) //-V807
            cbVideoModes.SetSelection(cbVideoModes.GetNumItems() - 1);
    }

    // "Vollbild" setzen
    optiongroup = groupGrafik->GetCtrl<ctrlOptionGroup>(47);
    optiongroup->SetSelection((SETTINGS.video.fullscreen ? 48 : 49)); //-V807

    // "Limit Framerate" füllen
    auto* cbFrameRate = groupGrafik->GetCtrl<ctrlComboBox>(51);
    if(VIDEODRIVER.HasVSync())
        cbFrameRate->AddString(_("Dynamic (Limits to display refresh rate, works with most drivers)"));
    for(int framerate : Settings::SCREEN_REFRESH_RATES)
    {
        if(framerate == -1)
            cbFrameRate->AddString(_("Disabled"));
        else
            cbFrameRate->AddString(helpers::toString(framerate) + " FPS");
        if(SETTINGS.video.vsync == framerate)
            cbFrameRate->SetSelection(cbFrameRate->GetNumItems() - 1);
    }
    if(!cbFrameRate->GetSelection())
        cbFrameRate->SetSelection(0);

    // "VBO" setzen
    optiongroup = groupGrafik->GetCtrl<ctrlOptionGroup>(55);
    optiongroup->SetSelection((SETTINGS.video.vbo ? 56 : 57));

    optiongroup = groupGrafik->GetCtrl<ctrlOptionGroup>(75);
    optiongroup->SetSelection((SETTINGS.video.shared_textures ? 76 : 77));
    // }

    // Sound
    // {

    // "Musik" setzen
    optiongroup = groupSound->GetCtrl<ctrlOptionGroup>(63);
    optiongroup->SetSelection((SETTINGS.sound.musicEnabled ? 64 : 65));

    // "Effekte" setzen
    optiongroup = groupSound->GetCtrl<ctrlOptionGroup>(67);
    optiongroup->SetSelection((SETTINGS.sound.effectsEnabled ? 68 : 69));

    // }

    // Load game settings
    ggs.LoadSettings();
}

dskOptions::~dskOptions()
{
    // Save game settings
    ggs.SaveSettings();
}

void dskOptions::Msg_Group_ProgressChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                          const unsigned short position)
{
    switch(ctrl_id)
    {
        case 70:
        {
            SETTINGS.sound.effectsVolume = static_cast<uint8_t>((position * 255) / 100);
            AUDIODRIVER.SetMasterEffectVolume(SETTINGS.sound.effectsVolume);
        }
        break;
        case 72:
        {
            SETTINGS.sound.musicVolume = static_cast<uint8_t>((position * 255) / 100);
            AUDIODRIVER.SetMusicVolume(SETTINGS.sound.musicVolume);
        }
        break;
    }
}

void dskOptions::Msg_Group_ComboSelectItem(const unsigned group_id, const unsigned ctrl_id, const unsigned selection)
{
    auto* group = GetCtrl<ctrlGroup>(group_id);
    auto* combo = group->GetCtrl<ctrlComboBox>(ctrl_id);

    switch(ctrl_id)
    {
        case 33: // Sprache
        {
            // Language changed?
            std::string old_lang = SETTINGS.language.language; //-V807
            SETTINGS.language.language = LANGUAGES.setLanguage(selection);
            if(SETTINGS.language.language != old_lang)
                WINDOWMANAGER.Switch(std::make_unique<dskOptions>());
        }
        break;
        case 39: // Proxy
            switch(selection)
            {
                case 0: SETTINGS.proxy.type = ProxyType::None; break;
                case 1: SETTINGS.proxy.type = ProxyType::Socks4; break;
                case 2: SETTINGS.proxy.type = ProxyType::Socks5; break;
            }

            // ipv6 gleich sichtbar deaktivieren
            if(SETTINGS.proxy.type == ProxyType::Socks4 && SETTINGS.server.ipv6)
            {
                GetCtrl<ctrlGroup>(21)->GetCtrl<ctrlOptionGroup>(301)->SetSelection(303);
                GetCtrl<ctrlGroup>(21)->GetCtrl<ctrlOptionGroup>(301)->GetCtrl<ctrlButton>(302)->SetEnabled(false);
                SETTINGS.server.ipv6 = false;
            }

            if(SETTINGS.proxy.type != ProxyType::Socks4)
                GetCtrl<ctrlGroup>(21)->GetCtrl<ctrlOptionGroup>(301)->GetCtrl<ctrlButton>(302)->SetEnabled(true);
            break;
        case 41: // Auflösung
            SETTINGS.video.fullscreenSize = video_modes[selection];
            break;
        case 51: // Limit Framerate
            if(VIDEODRIVER.HasVSync())
            {
                if(selection == 0)
                    SETTINGS.video.vsync = 0;
                else
                    SETTINGS.video.vsync = Settings::SCREEN_REFRESH_RATES[selection - 1];
            } else
                SETTINGS.video.vsync = Settings::SCREEN_REFRESH_RATES[selection];

            VIDEODRIVER.setTargetFramerate(SETTINGS.video.vsync);
            break;
        case 59: // Videotreiber
            SETTINGS.driver.video = combo->GetText(selection);
            break;
        case 61: // Audiotreiber
            SETTINGS.driver.audio = combo->GetText(selection);
            break;
    }
}

void dskOptions::Msg_Group_OptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                             const unsigned selection)
{
    switch(ctrl_id)
    {
        case 301: // IPv6 Ja/Nein
        {
            switch(selection)
            {
                case 302: SETTINGS.server.ipv6 = true; break;
                case 303: SETTINGS.server.ipv6 = false; break;
            }
        }
        break;
        case 47: // Vollbild
        {
            switch(selection)
            {
                case 48: SETTINGS.video.fullscreen = true; break;
                case 49: SETTINGS.video.fullscreen = false; break;
            }
        }
        break;
        case 55: // VBO
        {
            switch(selection)
            {
                case 56: SETTINGS.video.vbo = true; break;
                case 57: SETTINGS.video.vbo = false; break;
            }
        }
        break;
        case 75:
        {
            switch(selection)
            {
                case 76: SETTINGS.video.shared_textures = true; break;
                case 77: SETTINGS.video.shared_textures = false; break;
            }
        }
        break;

        case 63: // Musik
        {
            switch(selection)
            {
                case 64: SETTINGS.sound.musicEnabled = true; break;
                case 65: SETTINGS.sound.musicEnabled = false; break;
            }
            if(SETTINGS.sound.musicEnabled)
                MUSICPLAYER.Play();
            else
                MUSICPLAYER.Stop();
        }
        break;
        case 67: // Soundeffekte
        {
            switch(selection)
            {
                case 68: SETTINGS.sound.effectsEnabled = true; break;
                case 69: SETTINGS.sound.effectsEnabled = false; break;
            }
        }
        break;
        case 71: // Submit debug data
        {
            switch(selection)
            {
                case 72: SETTINGS.global.submit_debug_data = 1; break;
                case 73: SETTINGS.global.submit_debug_data = 2; break;
            }
        }
        break;
        case 9998:
        {
            switch(selection)
            {
                case 10001: SETTINGS.global.use_upnp = 1; break;
                case 10002: SETTINGS.global.use_upnp = 0; break;
            }
        }
        break;
        case 10101:
        {
            switch(selection)
            {
                case 10102: SETTINGS.global.smartCursor = true; break;
                case 10103: SETTINGS.global.smartCursor = false; break;
            }
            VIDEODRIVER.SetMouseWarping(SETTINGS.global.smartCursor);
        }
        break;
    }
}

void dskOptions::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case 10: // Optionengruppen anzeigen
        {
            for(unsigned short i = 21; i < 24; ++i)
                GetCtrl<ctrlGroup>(i)->SetVisible(i == selection + 10);
        }
        break;
    }
}

/// Check that the port is valid and sets outPort to it. Shows an error otherwise
static bool validatePort(const std::string& sPort, uint16_t& outPort)
{
    boost::optional<uint16_t> port = validate::checkPort(sPort);
    if(port)
        outPort = *port;
    else
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
          _("Error"), _("Invalid port. The valid port-range is 1 to 65535!"), nullptr, MSB_OK, MSB_EXCLAMATIONRED, 1));
    }
    return static_cast<bool>(port);
}

void dskOptions::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // "Zurück"
        {
            auto* groupAllgemein = GetCtrl<ctrlGroup>(21);

            // Name abspeichern
            SETTINGS.lobby.name = groupAllgemein->GetCtrl<ctrlEdit>(31)->GetText();
            if(!validatePort(groupAllgemein->GetCtrl<ctrlEdit>(41)->GetText(), SETTINGS.server.localPort))
                return;

            SETTINGS.proxy.hostname = groupAllgemein->GetCtrl<ctrlEdit>(37)->GetText();
            if(!validatePort(groupAllgemein->GetCtrl<ctrlEdit>(371)->GetText(), SETTINGS.proxy.port))
                return;

            SETTINGS.Save();

            if((SETTINGS.video.fullscreen && SETTINGS.video.fullscreenSize != VIDEODRIVER.GetWindowSize()) //-V807
               || SETTINGS.video.fullscreen != VIDEODRIVER.IsFullscreen())
            {
                const auto screenSize =
                  SETTINGS.video.fullscreen ? SETTINGS.video.fullscreenSize : SETTINGS.video.windowedSize;
                if(!VIDEODRIVER.ResizeScreen(screenSize, SETTINGS.video.fullscreen))
                {
                    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                      _("Sorry!"), _("You need to restart your game to change the screen resolution!"), this, MSB_OK,
                      MSB_EXCLAMATIONGREEN, 1));
                    return;
                }
            }
            if(SETTINGS.driver.video != VIDEODRIVER.GetName() || SETTINGS.driver.audio != AUDIODRIVER.GetName())
            {
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                  _("Sorry!"), _("You need to restart your game to change the video or audio driver!"), this, MSB_OK,
                  MSB_EXCLAMATIONGREEN, 1));
                return;
            }

            WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
        }
        break;
        case 14: // Addons
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwAddons>(ggs));
            break;
    }
}

void dskOptions::Msg_Group_ButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;
        case 71: // "Music player"
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwMusicPlayer>());
        }
        break;
        case 35: // "Keyboard Readme"
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwTextfile>("keyboardlayout.txt", _("Keyboard layout")));
        }
        break;
    }
}

void dskOptions::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    switch(msgbox_id)
    {
        default: break;
        case 1: // "You need to restart your game ..."
        {
            WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
        }
        break;
    }
}

static bool cmpVideoModes(const VideoMode& left, const VideoMode& right)
{
    if(left == right)
        return false;
    VideoMode leftRatio = getAspectRatio(left);
    VideoMode rightRatio = getAspectRatio(right);
    // Cmp ratios descending (so 16:9 is above 4:3 as wider ones are more commonly used)
    if(leftRatio.width == rightRatio.width)
    {
        if(leftRatio.height == rightRatio.height)
        {
            // Same ratios -> cmp width/height
            if(left.width == right.width)
                return left.height < right.height;
            else
                return left.width < right.width;

        } else
            return leftRatio.height > rightRatio.height;
    } else
        return leftRatio.width > rightRatio.width;
}

void dskOptions::loadVideoModes()
{
    // Get available modes
    VIDEODRIVER.ListVideoModes(video_modes);
    // Remove everything below 800x600
    helpers::remove_if(video_modes, [](const auto& it) { return it.width < 800 && it.height < 600; });
    // Sort by aspect ratio
    std::sort(video_modes.begin(), video_modes.end(), cmpVideoModes);
}
