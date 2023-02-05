// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "controls/ctrlImageButton.h"
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
#include "gameData/PortraitConsts.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"
#include <mygettext/mygettext.h>
#include <sstream>

enum
{
    ID_btBack = 0,
    ID_txtOptions = 1,
    ID_optOptions = 10,
    ID_btCommon = 11,
    ID_btGraphics = 12,
    ID_btSoundMusic = 13,
    ID_btAddons = 14,
    ID_groupCommon = 21,
    ID_groupGraphics = 22,
    ID_groupSound = 23,

    ID_txtCommonName = 30,
    ID_edtCommonName = 31,
    ID_txtCommonLanguage = 32,
    ID_cbCommonLanguage = 33,
    ID_txtCommonKeyLayout = 34,
    ID_btCommonKeyLayout = 35,
    ID_txtCommonProxy = 36,
    ID_edtCommonProxyAddress = 37,
    ID_edtCommonProxyPort = 371,
    ID_txtCommonProxyType = 38,
    ID_cbCommonProxyType = 39,
    ID_txtCommonLocalPort = 40,
    ID_edtCommonLocalPort = 41,
    ID_txtCommonDebugData = 70,
    ID_optCommonDebugData = 71,
    ID_btCommonDebugDataOn = 72,
    ID_btCommonDebugDataOff = 73,
    ID_txtCommonPortrait = 78,
    ID_btCommonPortrait = 79,
    ID_cbCommonPortrait = 81,
    ID_txtCommonIpv6 = 300,
    ID_optCommonIpv6 = 301,
    ID_btCommonIpv6On = 302,
    ID_btCommonIpv6Off = 303,
    ID_txtCommonUpnp = 9999,
    ID_optCommonUpnp = 9998,
    ID_btCommonUpnpOn = 10001,
    ID_btCommonUpnpOff = 10002,
    ID_txtCommonSmartCursor = 10100,
    ID_optCommonSmartCursor = 10101,
    ID_btCommonSmartCursorOn = 10102,
    ID_btCommonSmartCursorOff = 10103,

    ID_txtGraphicsResolution = 40,
    ID_cbGraphicsResolution = 41,
    ID_txtGraphicsMode = 46,
    ID_optGraphicsMode = 47,
    ID_btGraphicsModeFullscreen = 48,
    ID_btGraphicsModeWindowed = 49,
    ID_txtGraphicsVsync = 50,
    ID_cbGraphicsVsync = 51,
    ID_txtGraphicsVbo = 54,
    ID_optGraphicsVbo = 55,
    ID_btGraphicsVboOn = 56,
    ID_btGraphicsVboOff = 57,
    ID_txtGraphicsDriver = 58,
    ID_cbGraphicsDriver = 59,
    ID_txtGraphicsOptimizedTextures = 74,
    ID_optGraphicsOptimizedTextures = 75,
    ID_btGraphicsOptimizedTexturesOn = 76,
    ID_btGraphicsOptimizedTexturesOff = 77,

    ID_txtSoundDriver = 60,
    ID_cbSoundDriver = 61,
    ID_txtSoundMusic = 62,
    ID_optSoundMusic = 63,
    ID_btSoundMusicOn = 64,
    ID_btSoundMusicOff = 65,
    ID_pgSoundMusicVolume = 72,
    ID_txtSoundEffects = 66,
    ID_optSoundEffects = 67,
    ID_btSoundEffectsOn = 68,
    ID_SoundEffectsOff = 69,
    ID_pgSoundEffectsVolume = 70,
    ID_btSoundMusicPlayer = 71,
};

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
    AddTextButton(ID_btBack, DrawPoint(300, 550), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);

    AddText(ID_txtOptions, DrawPoint(400, 10), _("Options"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    ctrlOptionGroup* optiongroup = AddOptionGroup(ID_optOptions, GroupSelectType::Check);

    AddTextButton(ID_btAddons, DrawPoint(520, 550), Extent(200, 22), TextureColor::Green2, _("Addons"), NormalFont);

    optiongroup->AddTextButton(ID_btCommon, DrawPoint(80, 510), Extent(200, 22), TextureColor::Green2, _("Common"),
                               NormalFont);
    optiongroup->AddTextButton(ID_btGraphics, DrawPoint(300, 510), Extent(200, 22), TextureColor::Green2, _("Graphics"),
                               NormalFont);
    optiongroup->AddTextButton(ID_btSoundMusic, DrawPoint(520, 510), Extent(200, 22), TextureColor::Green2,
                               _("Sound/Music"), NormalFont);

    ctrlGroup* groupCommon = AddGroup(ID_groupCommon);
    ctrlGroup* groupGraphics = AddGroup(ID_groupGraphics);
    ctrlGroup* groupSound = AddGroup(ID_groupSound);
    ctrlComboBox* combo;

    // Common
    // {

    int offsY = 80;
    groupCommon->AddText(ID_txtCommonName, DrawPoint(80, offsY), _("Name in Game:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    ctrlEdit* name = groupCommon->AddEdit(ID_edtCommonName, DrawPoint(280, offsY - 5), Extent(190, 22),
                                          TextureColor::Grey, NormalFont, 15);
    name->SetText(SETTINGS.lobby.name);

    const auto& currentPortrait = Portraits[SETTINGS.lobby.portraitIndex];
    groupCommon->AddImageButton(ID_btCommonPortrait, DrawPoint(500, offsY - 5), Extent(40, 54), TextureColor::Grey,
                                LOADER.GetImageN(currentPortrait.resourceId, currentPortrait.resourceIndex));
    offsY += 30;
    groupCommon->AddText(ID_txtCommonPortrait, DrawPoint(80, offsY), _("Portrait:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    combo = groupCommon->AddComboBox(ID_cbCommonPortrait, DrawPoint(280, offsY - 5), Extent(190, 20),
                                     TextureColor::Grey, NormalFont, 100);

    for(unsigned i = 0; i < Portraits.size(); ++i)
    {
        combo->AddString(_(Portraits[i].name));
        if(SETTINGS.lobby.portraitIndex == i)
        {
            combo->SetSelection(i);
        }
    }

    offsY += 40;
    groupCommon->AddText(ID_txtCommonLanguage, DrawPoint(80, offsY), _("Language:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    combo = groupCommon->AddComboBox(ID_cbCommonLanguage, DrawPoint(280, offsY - 5), Extent(190, 20),
                                     TextureColor::Grey, NormalFont, 100);

    bool isAnySelected = false;
    for(unsigned i = 0; i < LANGUAGES.size(); ++i)
    {
        const Language& l = LANGUAGES.getLanguage(i);

        combo->AddString(_(l.name));
        if(SETTINGS.language.language == l.code)
        {
            combo->SetSelection(static_cast<unsigned short>(i));
            isAnySelected = true;
        }
    }
    if(!isAnySelected)
        combo->SetSelection(0);

    offsY += 40;
    groupCommon->AddText(ID_txtCommonKeyLayout, DrawPoint(80, offsY), _("Keyboard layout:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    groupCommon->AddTextButton(ID_btCommonKeyLayout, DrawPoint(280, offsY - 5), Extent(120, 22), TextureColor::Grey,
                               _("Readme"), NormalFont);

    offsY += 40;
    groupCommon->AddText(ID_txtCommonLocalPort, DrawPoint(80, offsY), _("Local Port:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    ctrlEdit* edtPort = groupCommon->AddEdit(ID_edtCommonLocalPort, DrawPoint(280, offsY - 5), Extent(190, 22),
                                             TextureColor::Grey, NormalFont, 15);
    edtPort->SetNumberOnly(true);
    edtPort->SetText(SETTINGS.server.localPort);

    offsY += 40;
    groupCommon->AddText(ID_txtCommonIpv6, DrawPoint(80, offsY), _("Use IPv6:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    ctrlOptionGroup* ipv6 = groupCommon->AddOptionGroup(ID_optCommonIpv6, GroupSelectType::Check);
    ipv6->AddTextButton(ID_btCommonIpv6On, DrawPoint(480, offsY - 5), Extent(190, 22), TextureColor::Grey, _("IPv6"),
                        NormalFont);
    ipv6->AddTextButton(ID_btCommonIpv6Off, DrawPoint(280, offsY - 5), Extent(190, 22), TextureColor::Grey, _("IPv4"),
                        NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? ID_btCommonIpv6On : ID_btCommonIpv6Off));

    ipv6->GetCtrl<ctrlButton>(ID_btCommonIpv6On)->SetEnabled(SETTINGS.proxy.type != ProxyType::Socks5); //-V807

    offsY += 50;
    groupCommon->AddText(ID_txtCommonProxy, DrawPoint(80, offsY), _("Proxyserver:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    ctrlEdit* proxy = groupCommon->AddEdit(ID_edtCommonProxyAddress, DrawPoint(280, offsY - 5), Extent(190, 22),
                                           TextureColor::Grey, NormalFont);
    proxy->SetText(SETTINGS.proxy.hostname);
    proxy = groupCommon->AddEdit(ID_edtCommonProxyPort, DrawPoint(480, offsY - 5), Extent(50, 22), TextureColor::Grey,
                                 NormalFont, 5);
    proxy->SetNumberOnly(true);
    proxy->SetText(SETTINGS.proxy.port);

    offsY += 30;
    groupCommon->AddText(ID_txtCommonProxyType, DrawPoint(80, offsY), _("Proxytyp:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    combo = groupCommon->AddComboBox(ID_cbCommonProxyType, DrawPoint(280, offsY - 5), Extent(390, 20),
                                     TextureColor::Grey, NormalFont, 100);
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

    offsY += 50;
    groupCommon->AddText(ID_txtCommonDebugData, DrawPoint(80, offsY), _("Submit debug data:"), COLOR_YELLOW,
                         FontStyle{}, NormalFont);
    optiongroup = groupCommon->AddOptionGroup(ID_optCommonDebugData, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_btCommonDebugDataOn, DrawPoint(480, offsY - 5), Extent(190, 22), TextureColor::Grey,
                               _("On"), NormalFont);
    optiongroup->AddTextButton(ID_btCommonDebugDataOff, DrawPoint(280, offsY - 5), Extent(190, 22), TextureColor::Grey,
                               _("Off"), NormalFont);

    optiongroup->SetSelection(
      ((SETTINGS.global.submit_debug_data == 1) ? ID_btCommonDebugDataOn : ID_btCommonDebugDataOff)); //-V807

    offsY += 30;
    groupCommon->AddText(ID_txtCommonUpnp, DrawPoint(80, offsY), _("Use UPnP"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlOptionGroup* upnp = groupCommon->AddOptionGroup(ID_optCommonUpnp, GroupSelectType::Check);
    upnp->AddTextButton(ID_btCommonUpnpOff, DrawPoint(280, offsY - 5), Extent(190, 22), TextureColor::Grey, _("Off"),
                        NormalFont);
    upnp->AddTextButton(ID_btCommonUpnpOn, DrawPoint(480, offsY - 5), Extent(190, 22), TextureColor::Grey, _("On"),
                        NormalFont);
    upnp->SetSelection((SETTINGS.global.use_upnp == 1) ? ID_btCommonUpnpOn : ID_btCommonUpnpOff);

    offsY += 30;
    groupCommon->AddText(ID_txtCommonSmartCursor, DrawPoint(80, offsY), _("Smart Cursor"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    ctrlOptionGroup* smartCursor = groupCommon->AddOptionGroup(ID_optCommonSmartCursor, GroupSelectType::Check);
    smartCursor->AddTextButton(
      ID_btCommonSmartCursorOff, DrawPoint(280, offsY - 5), Extent(190, 22), TextureColor::Grey, _("Off"), NormalFont,
      _("Don't move cursor automatically\nUseful e.g. for split-screen / dual-mice multiplayer (see wiki)"));
    smartCursor->AddTextButton(ID_btCommonSmartCursorOn, DrawPoint(480, offsY - 5), Extent(190, 22), TextureColor::Grey,
                               _("On"), NormalFont,
                               _("Place cursor on default button for new dialogs / action windows (default)"));
    smartCursor->SetSelection(SETTINGS.global.smartCursor ? ID_btCommonSmartCursorOn : ID_btCommonSmartCursorOff);

    // }

    groupGraphics->AddText(ID_txtGraphicsResolution, DrawPoint(80, 80), _("Fullscreen resolution:"), COLOR_YELLOW,
                           FontStyle{}, NormalFont);
    groupGraphics->AddComboBox(ID_cbGraphicsResolution, DrawPoint(280, 75), Extent(190, 22), TextureColor::Grey,
                               NormalFont, 150);

    groupGraphics->AddText(ID_txtGraphicsMode, DrawPoint(80, 130), _("Mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupGraphics->AddOptionGroup(ID_optGraphicsMode, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_btGraphicsModeFullscreen, DrawPoint(480, 125), Extent(190, 22), TextureColor::Grey,
                               _("Fullscreen"), NormalFont);
    optiongroup->AddTextButton(ID_btGraphicsModeWindowed, DrawPoint(280, 125), Extent(190, 22), TextureColor::Grey,
                               _("Windowed"), NormalFont);

    groupGraphics->AddText(ID_txtGraphicsVsync, DrawPoint(80, 180), _("Limit Framerate:"), COLOR_YELLOW, FontStyle{},
                           NormalFont);
    groupGraphics->AddComboBox(ID_cbGraphicsVsync, DrawPoint(280, 175), Extent(390, 22), TextureColor::Grey, NormalFont,
                               150);

    groupGraphics->AddText(ID_txtGraphicsVbo, DrawPoint(80, 230), _("Vertex Buffer Objects:"), COLOR_YELLOW,
                           FontStyle{}, NormalFont);
    optiongroup = groupGraphics->AddOptionGroup(ID_optGraphicsVbo, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_btGraphicsVboOn, DrawPoint(280, 225), Extent(190, 22), TextureColor::Grey, _("On"),
                               NormalFont);
    optiongroup->AddTextButton(ID_btGraphicsVboOff, DrawPoint(480, 225), Extent(190, 22), TextureColor::Grey, _("Off"),
                               NormalFont);

    groupGraphics->AddText(ID_txtGraphicsDriver, DrawPoint(80, 275), _("Graphics Driver"), COLOR_YELLOW, FontStyle{},
                           NormalFont);
    combo = groupGraphics->AddComboBox(ID_cbGraphicsDriver, DrawPoint(280, 275), Extent(390, 20), TextureColor::Grey,
                                       NormalFont, 100);

    const auto video_drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Video);

    for(const auto& video_driver : video_drivers)
    {
        combo->AddString(video_driver.GetName());
        if(video_driver.GetName() == SETTINGS.driver.video)
            combo->SetSelection(combo->GetNumItems() - 1);
    }

    groupGraphics->AddText(ID_txtGraphicsOptimizedTextures, DrawPoint(80, 320), _("Optimized Textures:"), COLOR_YELLOW,
                           FontStyle{}, NormalFont);
    optiongroup = groupGraphics->AddOptionGroup(ID_optGraphicsOptimizedTextures, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_btGraphicsOptimizedTexturesOn, DrawPoint(280, 315), Extent(190, 22),
                               TextureColor::Grey, _("On"), NormalFont);
    optiongroup->AddTextButton(ID_btGraphicsOptimizedTexturesOff, DrawPoint(480, 315), Extent(190, 22),
                               TextureColor::Grey, _("Off"), NormalFont);

    groupSound->AddText(ID_txtSoundDriver, DrawPoint(80, 230), _("Sounddriver"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupSound->AddComboBox(ID_cbSoundDriver, DrawPoint(280, 225), Extent(390, 20), TextureColor::Grey,
                                    NormalFont, 100);

    const auto audio_drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Audio);

    for(const auto& audio_driver : audio_drivers)
    {
        combo->AddString(audio_driver.GetName());
        if(audio_driver.GetName() == SETTINGS.driver.audio)
            combo->SetSelection(combo->GetNumItems() - 1);
    }

    groupSound->AddText(ID_txtSoundMusic, DrawPoint(80, 80), _("Music"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupSound->AddOptionGroup(ID_optSoundMusic, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_btSoundMusicOn, DrawPoint(280, 75), Extent(90, 22), TextureColor::Grey, _("On"),
                               NormalFont);
    optiongroup->AddTextButton(ID_btSoundMusicOff, DrawPoint(380, 75), Extent(90, 22), TextureColor::Grey, _("Off"),
                               NormalFont);

    ctrlProgress* Mvolume = groupSound->AddProgress(ID_pgSoundMusicVolume, DrawPoint(480, 75), Extent(190, 22),
                                                    TextureColor::Grey, 139, 138, 100);
    Mvolume->SetPosition((SETTINGS.sound.musicVolume * 100) / 255); //-V807

    groupSound->AddText(ID_txtSoundEffects, DrawPoint(80, 130), _("Effects"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupSound->AddOptionGroup(ID_optSoundEffects, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_btSoundEffectsOn, DrawPoint(280, 125), Extent(90, 22), TextureColor::Grey, _("On"),
                               NormalFont);
    optiongroup->AddTextButton(ID_SoundEffectsOff, DrawPoint(380, 125), Extent(90, 22), TextureColor::Grey, _("Off"),
                               NormalFont);

    ctrlProgress* FXvolume = groupSound->AddProgress(ID_pgSoundEffectsVolume, DrawPoint(480, 125), Extent(190, 22),
                                                     TextureColor::Grey, 139, 138, 100);
    FXvolume->SetPosition((SETTINGS.sound.effectsVolume * 100) / 255);

    groupSound->AddTextButton(ID_btSoundMusicPlayer, DrawPoint(280, 175), Extent(190, 22), TextureColor::Grey,
                              _("Music player"), NormalFont);

    optiongroup = GetCtrl<ctrlOptionGroup>(ID_optOptions);
    optiongroup->SetSelection(ID_btCommon, true);

    // Graphics
    // {

    loadVideoModes();

    // Und zu der Combobox hinzufügen
    ctrlComboBox& cbVideoModes = *groupGraphics->GetCtrl<ctrlComboBox>(ID_cbGraphicsResolution);
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

        // Ist das die aktuelle Fullscreen resolution? Dann selektieren
        if(videoMode == SETTINGS.video.fullscreenSize) //-V807
            cbVideoModes.SetSelection(cbVideoModes.GetNumItems() - 1);
    }

    // "Vollbild" setting
    optiongroup = groupGraphics->GetCtrl<ctrlOptionGroup>(ID_optGraphicsMode);
    optiongroup->SetSelection(
      (SETTINGS.video.fullscreen ? ID_btGraphicsModeFullscreen : ID_btGraphicsModeWindowed)); //-V807

    // "Limit Framerate" füllen
    auto* cbFrameRate = groupGraphics->GetCtrl<ctrlComboBox>(ID_cbGraphicsVsync);
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

    // "VBO" setting
    optiongroup = groupGraphics->GetCtrl<ctrlOptionGroup>(ID_optGraphicsVbo);
    optiongroup->SetSelection((SETTINGS.video.vbo ? ID_btGraphicsVboOn : ID_btGraphicsVboOff));

    optiongroup = groupGraphics->GetCtrl<ctrlOptionGroup>(ID_optGraphicsOptimizedTextures);
    optiongroup->SetSelection(
      (SETTINGS.video.shared_textures ? ID_btGraphicsOptimizedTexturesOn : ID_btGraphicsOptimizedTexturesOff));
    // }

    // Sound
    // {

    // "Music" setting
    optiongroup = groupSound->GetCtrl<ctrlOptionGroup>(ID_optSoundMusic);
    optiongroup->SetSelection((SETTINGS.sound.musicEnabled ? ID_btSoundMusicOn : ID_btSoundMusicOff));

    // "Effects" setting
    optiongroup = groupSound->GetCtrl<ctrlOptionGroup>(ID_optSoundEffects);
    optiongroup->SetSelection((SETTINGS.sound.effectsEnabled ? ID_btSoundEffectsOn : ID_SoundEffectsOff));

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
        case ID_pgSoundEffectsVolume:
        {
            SETTINGS.sound.effectsVolume = static_cast<uint8_t>((position * 255) / 100);
            AUDIODRIVER.SetMasterEffectVolume(SETTINGS.sound.effectsVolume);
        }
        break;
        case ID_btSoundMusicPlayer:
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
        case ID_cbCommonPortrait:
            SETTINGS.lobby.portraitIndex = selection;
            updatePortraitControls();
            break;
        case ID_cbCommonLanguage:
        {
            // Language changed?
            std::string old_lang = SETTINGS.language.language; //-V807
            SETTINGS.language.language = LANGUAGES.setLanguage(selection);
            if(SETTINGS.language.language != old_lang)
                WINDOWMANAGER.Switch(std::make_unique<dskOptions>());
        }
        break;
        case ID_cbCommonProxyType:
            switch(selection)
            {
                case 0: SETTINGS.proxy.type = ProxyType::None; break;
                case 1: SETTINGS.proxy.type = ProxyType::Socks4; break;
                case 2: SETTINGS.proxy.type = ProxyType::Socks5; break;
            }

            // ipv6 gleich sichtbar deaktivieren
            if(SETTINGS.proxy.type == ProxyType::Socks4 && SETTINGS.server.ipv6)
            {
                GetCtrl<ctrlGroup>(ID_groupCommon)
                  ->GetCtrl<ctrlOptionGroup>(ID_optCommonIpv6)
                  ->SetSelection(ID_btCommonIpv6Off);
                GetCtrl<ctrlGroup>(ID_groupCommon)
                  ->GetCtrl<ctrlOptionGroup>(ID_optCommonIpv6)
                  ->GetCtrl<ctrlButton>(ID_btCommonIpv6On)
                  ->SetEnabled(false);
                SETTINGS.server.ipv6 = false;
            }

            if(SETTINGS.proxy.type != ProxyType::Socks4)
                GetCtrl<ctrlGroup>(ID_groupCommon)
                  ->GetCtrl<ctrlOptionGroup>(ID_optCommonIpv6)
                  ->GetCtrl<ctrlButton>(ID_btCommonIpv6On)
                  ->SetEnabled(true);
            break;
        case ID_edtCommonLocalPort: SETTINGS.video.fullscreenSize = video_modes[selection]; break;
        case ID_cbGraphicsVsync:
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
        case ID_cbGraphicsDriver: SETTINGS.driver.video = combo->GetText(selection); break;
        case ID_cbSoundDriver: SETTINGS.driver.audio = combo->GetText(selection); break;
    }
}

void dskOptions::Msg_Group_OptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                             const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_optCommonIpv6:
        {
            switch(selection)
            {
                case ID_btCommonIpv6On: SETTINGS.server.ipv6 = true; break;
                case ID_btCommonIpv6Off: SETTINGS.server.ipv6 = false; break;
            }
        }
        break;
        case ID_optGraphicsMode:
        {
            switch(selection)
            {
                case ID_btGraphicsModeFullscreen: SETTINGS.video.fullscreen = true; break;
                case ID_btGraphicsModeWindowed: SETTINGS.video.fullscreen = false; break;
            }
        }
        break;
        case ID_optGraphicsVbo:
        {
            switch(selection)
            {
                case ID_btGraphicsVboOn: SETTINGS.video.vbo = true; break;
                case ID_btGraphicsVboOff: SETTINGS.video.vbo = false; break;
            }
        }
        break;
        case ID_optGraphicsOptimizedTextures:
        {
            switch(selection)
            {
                case ID_btGraphicsOptimizedTexturesOn: SETTINGS.video.shared_textures = true; break;
                case ID_btGraphicsOptimizedTexturesOff: SETTINGS.video.shared_textures = false; break;
            }
        }
        break;

        case ID_optSoundMusic:
        {
            switch(selection)
            {
                case ID_btSoundMusicOn: SETTINGS.sound.musicEnabled = true; break;
                case ID_btSoundMusicOff: SETTINGS.sound.musicEnabled = false; break;
            }
            if(SETTINGS.sound.musicEnabled)
                MUSICPLAYER.Play();
            else
                MUSICPLAYER.Stop();
        }
        break;
        case ID_optSoundEffects:
        {
            switch(selection)
            {
                case ID_btSoundEffectsOn: SETTINGS.sound.effectsEnabled = true; break;
                case ID_SoundEffectsOff: SETTINGS.sound.effectsEnabled = false; break;
            }
        }
        break;
        case ID_optCommonDebugData:
        {
            switch(selection)
            {
                case ID_btCommonDebugDataOn: SETTINGS.global.submit_debug_data = 1; break;
                case ID_btCommonDebugDataOff: SETTINGS.global.submit_debug_data = 2; break;
            }
        }
        break;
        case ID_optCommonUpnp:
        {
            switch(selection)
            {
                case ID_btCommonUpnpOn: SETTINGS.global.use_upnp = 1; break;
                case ID_btCommonUpnpOff: SETTINGS.global.use_upnp = 0; break;
            }
        }
        break;
        case ID_optCommonSmartCursor:
        {
            switch(selection)
            {
                case ID_btCommonSmartCursorOn: SETTINGS.global.smartCursor = true; break;
                case ID_btCommonSmartCursorOff: SETTINGS.global.smartCursor = false; break;
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
        case ID_optOptions: // Optionengruppen anzeigen
        {
            static const std::array<unsigned int, 3> buttons{ID_btCommon, ID_btGraphics, ID_btSoundMusic};
            static const std::array<unsigned int, 3> groups{ID_groupCommon, ID_groupGraphics, ID_groupSound};
            for(unsigned short i = 0; i < groups.size(); ++i)
                GetCtrl<ctrlGroup>(groups[i])->SetVisible(selection == buttons[i]);
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
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"),
                                                      _("Invalid port. The valid port-range is 1 to 65535!"), nullptr,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
    }
    return static_cast<bool>(port);
}

void dskOptions::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack:
        {
            auto* groupCommon = GetCtrl<ctrlGroup>(ID_groupCommon);

            // Name abspeichern
            SETTINGS.lobby.name = groupCommon->GetCtrl<ctrlEdit>(ID_edtCommonName)->GetText();
            if(!validatePort(groupCommon->GetCtrl<ctrlEdit>(ID_edtCommonLocalPort)->GetText(),
                             SETTINGS.server.localPort))
                return;

            SETTINGS.proxy.hostname = groupCommon->GetCtrl<ctrlEdit>(ID_edtCommonProxyAddress)->GetText();
            if(!validatePort(groupCommon->GetCtrl<ctrlEdit>(ID_edtCommonProxyPort)->GetText(), SETTINGS.proxy.port))
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
                      _("Sorry!"), _("You need to restart your game to change the screen resolution!"), this,
                      MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
                    return;
                }
            }
            if(SETTINGS.driver.video != VIDEODRIVER.GetName() || SETTINGS.driver.audio != AUDIODRIVER.GetName())
            {
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                  _("Sorry!"), _("You need to restart your game to change the video or audio driver!"), this,
                  MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
                return;
            }

            WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
        }
        break;
        case ID_btAddons: WINDOWMANAGER.ToggleWindow(std::make_unique<iwAddons>(ggs)); break;
    }
}

void dskOptions::Msg_Group_ButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;
        case ID_btCommonPortrait:
            SETTINGS.lobby.portraitIndex = (SETTINGS.lobby.portraitIndex + 1) % Portraits.size();
            updatePortraitControls();
            break;
        case ID_btSoundMusicPlayer: WINDOWMANAGER.ToggleWindow(std::make_unique<iwMusicPlayer>()); break;
        case ID_btCommonKeyLayout:
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwTextfile>("keyboardlayout.txt", _("Keyboard layout")));
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
    helpers::erase_if(video_modes, [](const auto& it) { return it.width < 800 && it.height < 600; });
    // Sort by aspect ratio
    std::sort(video_modes.begin(), video_modes.end(), cmpVideoModes);
}

void dskOptions::updatePortraitControls()
{
    const auto& newPortrait = Portraits[SETTINGS.lobby.portraitIndex];
    auto* groupCommon = GetCtrl<ctrlGroup>(ID_groupCommon);

    auto* portraitButton = groupCommon->GetCtrl<ctrlImageButton>(ID_btCommonPortrait);
    auto* newPortraitTexture = LOADER.GetTextureN(newPortrait.resourceId, newPortrait.resourceIndex);
    portraitButton->SetImage(newPortraitTexture);

    auto* portraitCombo = groupCommon->GetCtrl<ctrlComboBox>(ID_cbCommonPortrait);
    portraitCombo->SetSelection(SETTINGS.lobby.portraitIndex);
}
