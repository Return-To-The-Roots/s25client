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
    ID_BACK_BUTTON = 0,
    ID_OPTIONS_LABEL = 1,
    ID_OPTIONS_GROUP = 10,
    ID_COMMON_BUTTON = 11,
    ID_GRAPHICS_BUTTON = 12,
    ID_SOUND_MUSIC_BUTTON = 13,
    ID_ADDONS_BUTTON = 14,
    ID_GROUP_COMMON = 21,
    ID_GROUP_GRAPHICS = 22,
    ID_GROUP_SOUND = 23,

    ID_COMMON_NAME_LABEL = 30,
    ID_COMMON_NAME_EDIT = 31,
    ID_COMMON_LANGUAGE_LABEL = 32,
    ID_COMMON_LANGUAGE_COMBO = 33,
    ID_COMMON_KEY_LAYOUT_LABEL = 34,
    ID_COMMON_KEY_LAYOUT_BUTTON = 35,
    ID_COMMON_PROXY_LABEL = 36,
    ID_COMMON_PROXY_EDIT_ADDRESS = 37,
    ID_COMMON_PROXY_EDIT_PORT = 371,
    ID_COMMON_PROXY_TYPE_LABEL = 38,
    ID_COMMON_PROXY_TYPE_COMBO = 39,
    ID_COMMON_LOCAL_PORT_LABEL = 40,
    ID_COMMON_LOCAL_PORT_EDIT = 41,
    ID_COMMON_DEBUG_DATA_LABEL = 70,
    ID_COMMON_DEBUG_DATA_GROUP = 71,
    ID_COMMON_DEBUG_DATA_ON = 72,
    ID_COMMON_DEBUG_DATA_OFF = 73,
    ID_COMMON_PORTRAIT_LABEL = 78,
    ID_COMMON_PORTRAIT_BUTTON = 79,
    ID_COMMON_PORTRAIT_COMBO = 81,
    ID_COMMON_IPV6_LABEL = 300,
    ID_COMMON_IPV6_GROUP = 301,
    ID_COMMON_IPV6_BUTTON_ON = 302,
    ID_COMMON_IPV6_BUTTON_OFF = 303,
    ID_COMMON_UPNP_LABEL = 9999,
    ID_COMMON_UPNP_GROUP = 9998,
    ID_COMMON_UPNP_ON = 10001,
    ID_COMMON_UPNP_OFF = 10002,
    ID_COMMON_SMART_CURSOR_LABEL = 10100,
    ID_COMMON_SMART_CURSOR_GROUP = 10101,
    ID_COMMON_SMART_CURSOR_ON = 10102,
    ID_COMMON_SMART_CURSOR_OFF = 10103,

    ID_GRAPHICS_RESOLUTION_LABEL = 40,
    ID_GRAPHICS_RESOLUTION_COMBO = 41,
    ID_GRAPHICS_MODE_LABEL = 46,
    ID_GRAPHICS_MODE_GROUP = 47,
    ID_GRAPHICS_MODE_FULLSCREEN = 48,
    ID_GRAPHICS_MODE_WINDOWED = 49,
    ID_GRAPHICS_VSYNC_LABEL = 50,
    ID_GRAPHICS_VSYNC_COMBO = 51,
    ID_GRAPHICS_VBO_LABEL = 54,
    ID_GRAPHICS_VBO_GROUP = 55,
    ID_GRAPHICS_VBO_ON = 56,
    ID_GRAPHICS_VBO_OFF = 57,
    ID_GRAPHICS_DRIVER_LABEL = 58,
    ID_GRAPHICS_DRIVER_COMBO = 59,
    ID_GRAPHICS_OPTIMIZED_TEXTURES_LABEL = 74,
    ID_GRAPHICS_OPTIMIZED_TEXTURES_GROUP = 75,
    ID_GRAPHICS_OPTIMIZED_TEXTURES_ON = 76,
    ID_GRAPHICS_OPTIMIZED_TEXTURES_OFF = 77,

    ID_SOUND_DRIVER_LABEL = 60,
    ID_SOUND_DRIVER_COMBO = 61,
    ID_SOUND_MUSIC_LABEL = 62,
    ID_SOUND_MUSIC_GROUP = 63,
    ID_SOUND_MUSIC_ON = 64,
    ID_SOUND_MUSIC_OFF = 65,
    ID_SOUND_MUSIC_VOLUME = 72,
    ID_SOUND_EFFECTS_LABEL = 66,
    ID_SOUND_EFFECTS_GROUP = 67,
    ID_SOUND_EFFECTS_ON = 68,
    ID_SOUND_EFFECTS_OFF = 69,
    ID_SOUND_EFFECTS_VOLUME = 70,
    ID_SOUND_MUSIC_PLAYER = 71,
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
    // Back
    AddTextButton(ID_BACK_BUTTON, DrawPoint(300, 550), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);

    // "Options"
    AddText(ID_OPTIONS_LABEL, DrawPoint(400, 10), _("Options"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    ctrlOptionGroup* optiongroup = AddOptionGroup(ID_OPTIONS_GROUP, GroupSelectType::Check);

    AddTextButton(ID_ADDONS_BUTTON, DrawPoint(520, 550), Extent(200, 22), TextureColor::Green2, _("Addons"),
                  NormalFont);

    // "Common"
    optiongroup->AddTextButton(ID_COMMON_BUTTON, DrawPoint(80, 510), Extent(200, 22), TextureColor::Green2, _("Common"),
                               NormalFont);
    // "Graphics"
    optiongroup->AddTextButton(ID_GRAPHICS_BUTTON, DrawPoint(300, 510), Extent(200, 22), TextureColor::Green2,
                               _("Graphics"), NormalFont);
    // "Sound"
    optiongroup->AddTextButton(ID_SOUND_MUSIC_BUTTON, DrawPoint(520, 510), Extent(200, 22), TextureColor::Green2,
                               _("Sound/Music"), NormalFont);

    ctrlGroup* groupCommon = AddGroup(ID_GROUP_COMMON);
    ctrlGroup* groupGraphics = AddGroup(ID_GROUP_GRAPHICS);
    ctrlGroup* groupSound = AddGroup(ID_GROUP_SOUND);
    ctrlComboBox* combo;

    // Common
    // {

    // "Name"
    int offsY = 80;
    groupCommon->AddText(ID_COMMON_NAME_LABEL, DrawPoint(80, offsY), _("Name in Game:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    ctrlEdit* name = groupCommon->AddEdit(ID_COMMON_NAME_EDIT, DrawPoint(280, offsY - 5), Extent(190, 22),
                                          TextureColor::Grey, NormalFont, 15);
    name->SetText(SETTINGS.lobby.name);

    // Portrait display next to name
    const auto& currentPortrait = Portraits[SETTINGS.lobby.portraitIndex];
    groupCommon->AddImageButton(ID_COMMON_PORTRAIT_BUTTON, DrawPoint(500, offsY - 5), Extent(40, 54),
                                TextureColor::Grey,
                                LOADER.GetImageN(currentPortrait.resourceId, currentPortrait.resourceIndex));

    // Portrait dropdown
    offsY += 30;
    groupCommon->AddText(ID_COMMON_PORTRAIT_LABEL, DrawPoint(80, offsY), _("Portrait:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    combo = groupCommon->AddComboBox(ID_COMMON_PORTRAIT_COMBO, DrawPoint(280, offsY - 5), Extent(190, 20),
                                     TextureColor::Grey, NormalFont, 100);

    for(unsigned i = 0; i < Portraits.size(); ++i)
    {
        combo->AddString(_(Portraits[i].name));
        if(SETTINGS.lobby.portraitIndex == i)
        {
            combo->SetSelection(i);
        }
    }

    // "Language"
    offsY += 40;
    groupCommon->AddText(ID_COMMON_LANGUAGE_LABEL, DrawPoint(80, offsY), _("Language:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    combo = groupCommon->AddComboBox(ID_COMMON_LANGUAGE_COMBO, DrawPoint(280, offsY - 5), Extent(190, 20),
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

    // Keyboard layout
    offsY += 40;
    groupCommon->AddText(ID_COMMON_KEY_LAYOUT_LABEL, DrawPoint(80, offsY), _("Keyboard layout:"), COLOR_YELLOW,
                         FontStyle{}, NormalFont);
    groupCommon->AddTextButton(ID_COMMON_KEY_LAYOUT_BUTTON, DrawPoint(280, offsY - 5), Extent(120, 22),
                               TextureColor::Grey, _("Readme"), NormalFont);

    offsY += 40;
    groupCommon->AddText(ID_COMMON_LOCAL_PORT_LABEL, DrawPoint(80, offsY), _("Local Port:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    ctrlEdit* edtPort = groupCommon->AddEdit(ID_COMMON_LOCAL_PORT_EDIT, DrawPoint(280, offsY - 5), Extent(190, 22),
                                             TextureColor::Grey, NormalFont, 15);
    edtPort->SetNumberOnly(true);
    edtPort->SetText(SETTINGS.server.localPort);

    // IPv4/6
    offsY += 40;
    groupCommon->AddText(ID_COMMON_IPV6_LABEL, DrawPoint(80, offsY), _("Use IPv6:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);

    ctrlOptionGroup* ipv6 = groupCommon->AddOptionGroup(ID_COMMON_IPV6_GROUP, GroupSelectType::Check);
    ipv6->AddTextButton(ID_COMMON_IPV6_BUTTON_ON, DrawPoint(480, offsY - 5), Extent(190, 22), TextureColor::Grey,
                        _("IPv6"), NormalFont);
    ipv6->AddTextButton(ID_COMMON_IPV6_BUTTON_OFF, DrawPoint(280, offsY - 5), Extent(190, 22), TextureColor::Grey,
                        _("IPv4"), NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? ID_COMMON_IPV6_BUTTON_ON : ID_COMMON_IPV6_BUTTON_OFF));

    // ipv6-feld ggf (de-)aktivieren
    ipv6->GetCtrl<ctrlButton>(ID_COMMON_IPV6_BUTTON_ON)->SetEnabled(SETTINGS.proxy.type != ProxyType::Socks5); //-V807

    // Proxyserver
    offsY += 50;
    groupCommon->AddText(ID_COMMON_PROXY_LABEL, DrawPoint(80, offsY), _("Proxyserver:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    ctrlEdit* proxy = groupCommon->AddEdit(ID_COMMON_PROXY_EDIT_ADDRESS, DrawPoint(280, offsY - 5), Extent(190, 22),
                                           TextureColor::Grey, NormalFont);
    proxy->SetText(SETTINGS.proxy.hostname);
    proxy = groupCommon->AddEdit(ID_COMMON_PROXY_EDIT_PORT, DrawPoint(480, offsY - 5), Extent(50, 22),
                                 TextureColor::Grey, NormalFont, 5);
    proxy->SetNumberOnly(true);
    proxy->SetText(SETTINGS.proxy.port);

    // Proxytyp
    offsY += 30;
    groupCommon->AddText(ID_COMMON_PROXY_TYPE_LABEL, DrawPoint(80, offsY), _("Proxytyp:"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    combo = groupCommon->AddComboBox(ID_COMMON_PROXY_TYPE_COMBO, DrawPoint(280, offsY - 5), Extent(390, 20),
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
    groupCommon->AddText(ID_COMMON_DEBUG_DATA_LABEL, DrawPoint(80, offsY), _("Submit debug data:"), COLOR_YELLOW,
                         FontStyle{}, NormalFont);
    optiongroup = groupCommon->AddOptionGroup(ID_COMMON_DEBUG_DATA_GROUP, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_COMMON_DEBUG_DATA_ON, DrawPoint(480, offsY - 5), Extent(190, 22), TextureColor::Grey,
                               _("On"), NormalFont);
    optiongroup->AddTextButton(ID_COMMON_DEBUG_DATA_OFF, DrawPoint(280, offsY - 5), Extent(190, 22), TextureColor::Grey,
                               _("Off"), NormalFont);

    optiongroup->SetSelection(
      ((SETTINGS.global.submit_debug_data == 1) ? ID_COMMON_DEBUG_DATA_ON : ID_COMMON_DEBUG_DATA_OFF)); //-V807

    // qx:upnp switch
    offsY += 30;
    groupCommon->AddText(ID_COMMON_UPNP_LABEL, DrawPoint(80, offsY), _("Use UPnP"), COLOR_YELLOW, FontStyle{},
                         NormalFont);
    ctrlOptionGroup* upnp = groupCommon->AddOptionGroup(ID_COMMON_UPNP_GROUP, GroupSelectType::Check);
    upnp->AddTextButton(ID_COMMON_UPNP_OFF, DrawPoint(280, offsY - 5), Extent(190, 22), TextureColor::Grey, _("Off"),
                        NormalFont);
    upnp->AddTextButton(ID_COMMON_UPNP_ON, DrawPoint(480, offsY - 5), Extent(190, 22), TextureColor::Grey, _("On"),
                        NormalFont);
    upnp->SetSelection((SETTINGS.global.use_upnp == 1) ? ID_COMMON_UPNP_ON : ID_COMMON_UPNP_OFF);

    offsY += 30;
    groupCommon->AddText(ID_COMMON_SMART_CURSOR_LABEL, DrawPoint(80, offsY), _("Smart Cursor"), COLOR_YELLOW,
                         FontStyle{}, NormalFont);
    ctrlOptionGroup* smartCursor = groupCommon->AddOptionGroup(ID_COMMON_SMART_CURSOR_GROUP, GroupSelectType::Check);
    smartCursor->AddTextButton(
      ID_COMMON_SMART_CURSOR_OFF, DrawPoint(280, offsY - 5), Extent(190, 22), TextureColor::Grey, _("Off"), NormalFont,
      _("Don't move cursor automatically\nUseful e.g. for split-screen / dual-mice multiplayer (see wiki)"));
    smartCursor->AddTextButton(ID_COMMON_SMART_CURSOR_ON, DrawPoint(480, offsY - 5), Extent(190, 22),
                               TextureColor::Grey, _("On"), NormalFont,
                               _("Place cursor on default button for new dialogs / action windows (default)"));
    smartCursor->SetSelection(SETTINGS.global.smartCursor ? ID_COMMON_SMART_CURSOR_ON : ID_COMMON_SMART_CURSOR_OFF);

    // }

    // "Fullscreen resolution"
    groupGraphics->AddText(ID_GRAPHICS_RESOLUTION_LABEL, DrawPoint(80, 80), _("Fullscreen resolution:"), COLOR_YELLOW,
                           FontStyle{}, NormalFont);
    groupGraphics->AddComboBox(ID_GRAPHICS_RESOLUTION_COMBO, DrawPoint(280, 75), Extent(190, 22), TextureColor::Grey,
                               NormalFont, 150);

    // "Mode"
    groupGraphics->AddText(ID_GRAPHICS_MODE_LABEL, DrawPoint(80, 130), _("Mode:"), COLOR_YELLOW, FontStyle{},
                           NormalFont);
    optiongroup = groupGraphics->AddOptionGroup(ID_GRAPHICS_MODE_GROUP, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_GRAPHICS_MODE_FULLSCREEN, DrawPoint(480, 125), Extent(190, 22), TextureColor::Grey,
                               _("Fullscreen"), NormalFont);
    optiongroup->AddTextButton(ID_GRAPHICS_MODE_WINDOWED, DrawPoint(280, 125), Extent(190, 22), TextureColor::Grey,
                               _("Windowed"), NormalFont);

    // "VSync"
    groupGraphics->AddText(ID_GRAPHICS_VSYNC_LABEL, DrawPoint(80, 180), _("Limit Framerate:"), COLOR_YELLOW,
                           FontStyle{}, NormalFont);
    groupGraphics->AddComboBox(ID_GRAPHICS_VSYNC_COMBO, DrawPoint(280, 175), Extent(390, 22), TextureColor::Grey,
                               NormalFont, 150);

    // "VBO"
    groupGraphics->AddText(ID_GRAPHICS_VBO_LABEL, DrawPoint(80, 230), _("Vertex Buffer Objects:"), COLOR_YELLOW,
                           FontStyle{}, NormalFont);
    optiongroup = groupGraphics->AddOptionGroup(ID_GRAPHICS_VBO_GROUP, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_GRAPHICS_VBO_ON, DrawPoint(280, 225), Extent(190, 22), TextureColor::Grey, _("On"),
                               NormalFont);
    optiongroup->AddTextButton(ID_GRAPHICS_VBO_OFF, DrawPoint(480, 225), Extent(190, 22), TextureColor::Grey, _("Off"),
                               NormalFont);

    // "Graphics Driver"
    groupGraphics->AddText(ID_GRAPHICS_DRIVER_LABEL, DrawPoint(80, 275), _("Graphics Driver"), COLOR_YELLOW,
                           FontStyle{}, NormalFont);
    combo = groupGraphics->AddComboBox(ID_GRAPHICS_DRIVER_COMBO, DrawPoint(280, 275), Extent(390, 20),
                                       TextureColor::Grey, NormalFont, 100);

    const auto video_drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Video);

    for(const auto& video_driver : video_drivers)
    {
        combo->AddString(video_driver.GetName());
        if(video_driver.GetName() == SETTINGS.driver.video)
            combo->SetSelection(combo->GetNumItems() - 1);
    }

    groupGraphics->AddText(ID_GRAPHICS_OPTIMIZED_TEXTURES_LABEL, DrawPoint(80, 320), _("Optimized Textures:"),
                           COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupGraphics->AddOptionGroup(ID_GRAPHICS_OPTIMIZED_TEXTURES_GROUP, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_GRAPHICS_OPTIMIZED_TEXTURES_ON, DrawPoint(280, 315), Extent(190, 22),
                               TextureColor::Grey, _("On"), NormalFont);
    optiongroup->AddTextButton(ID_GRAPHICS_OPTIMIZED_TEXTURES_OFF, DrawPoint(480, 315), Extent(190, 22),
                               TextureColor::Grey, _("Off"), NormalFont);

    // "Sounddriver"
    groupSound->AddText(ID_SOUND_DRIVER_LABEL, DrawPoint(80, 230), _("Sounddriver"), COLOR_YELLOW, FontStyle{},
                        NormalFont);
    combo = groupSound->AddComboBox(ID_SOUND_DRIVER_COMBO, DrawPoint(280, 225), Extent(390, 20), TextureColor::Grey,
                                    NormalFont, 100);

    const auto audio_drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Audio);

    for(const auto& audio_driver : audio_drivers)
    {
        combo->AddString(audio_driver.GetName());
        if(audio_driver.GetName() == SETTINGS.driver.audio)
            combo->SetSelection(combo->GetNumItems() - 1);
    }

    // Music
    groupSound->AddText(ID_SOUND_MUSIC_LABEL, DrawPoint(80, 80), _("Music"), COLOR_YELLOW, FontStyle{}, NormalFont);
    optiongroup = groupSound->AddOptionGroup(ID_SOUND_MUSIC_GROUP, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_SOUND_MUSIC_ON, DrawPoint(280, 75), Extent(90, 22), TextureColor::Grey, _("On"),
                               NormalFont);
    optiongroup->AddTextButton(ID_SOUND_MUSIC_OFF, DrawPoint(380, 75), Extent(90, 22), TextureColor::Grey, _("Off"),
                               NormalFont);

    ctrlProgress* Mvolume = groupSound->AddProgress(ID_SOUND_MUSIC_VOLUME, DrawPoint(480, 75), Extent(190, 22),
                                                    TextureColor::Grey, 139, 138, 100);
    Mvolume->SetPosition((SETTINGS.sound.musicVolume * 100) / 255); //-V807

    // Effects
    groupSound->AddText(ID_SOUND_EFFECTS_LABEL, DrawPoint(80, 130), _("Effects"), COLOR_YELLOW, FontStyle{},
                        NormalFont);
    optiongroup = groupSound->AddOptionGroup(ID_SOUND_EFFECTS_GROUP, GroupSelectType::Check);
    optiongroup->AddTextButton(ID_SOUND_EFFECTS_ON, DrawPoint(280, 125), Extent(90, 22), TextureColor::Grey, _("On"),
                               NormalFont);
    optiongroup->AddTextButton(ID_SOUND_EFFECTS_OFF, DrawPoint(380, 125), Extent(90, 22), TextureColor::Grey, _("Off"),
                               NormalFont);

    ctrlProgress* FXvolume = groupSound->AddProgress(ID_SOUND_EFFECTS_VOLUME, DrawPoint(480, 125), Extent(190, 22),
                                                     TextureColor::Grey, 139, 138, 100);
    FXvolume->SetPosition((SETTINGS.sound.effectsVolume * 100) / 255);

    // Music player
    groupSound->AddTextButton(ID_SOUND_MUSIC_PLAYER, DrawPoint(280, 175), Extent(190, 22), TextureColor::Grey,
                              _("Music player"), NormalFont);

    // Activate "Common"
    optiongroup = GetCtrl<ctrlOptionGroup>(ID_OPTIONS_GROUP);
    optiongroup->SetSelection(ID_COMMON_BUTTON, true);

    // Graphics
    // {

    loadVideoModes();

    // Und zu der Combobox hinzufügen
    ctrlComboBox& cbVideoModes = *groupGraphics->GetCtrl<ctrlComboBox>(ID_GRAPHICS_RESOLUTION_COMBO);
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
    optiongroup = groupGraphics->GetCtrl<ctrlOptionGroup>(ID_GRAPHICS_MODE_GROUP);
    optiongroup->SetSelection(
      (SETTINGS.video.fullscreen ? ID_GRAPHICS_MODE_FULLSCREEN : ID_GRAPHICS_MODE_WINDOWED)); //-V807

    // "Limit Framerate" füllen
    auto* cbFrameRate = groupGraphics->GetCtrl<ctrlComboBox>(ID_GRAPHICS_VSYNC_COMBO);
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
    optiongroup = groupGraphics->GetCtrl<ctrlOptionGroup>(ID_GRAPHICS_VBO_GROUP);
    optiongroup->SetSelection((SETTINGS.video.vbo ? ID_GRAPHICS_VBO_ON : ID_GRAPHICS_VBO_OFF));

    optiongroup = groupGraphics->GetCtrl<ctrlOptionGroup>(ID_GRAPHICS_OPTIMIZED_TEXTURES_GROUP);
    optiongroup->SetSelection(
      (SETTINGS.video.shared_textures ? ID_GRAPHICS_OPTIMIZED_TEXTURES_ON : ID_GRAPHICS_OPTIMIZED_TEXTURES_OFF));
    // }

    // Sound
    // {

    // "Music" setting
    optiongroup = groupSound->GetCtrl<ctrlOptionGroup>(ID_SOUND_MUSIC_GROUP);
    optiongroup->SetSelection((SETTINGS.sound.musicEnabled ? ID_SOUND_MUSIC_ON : ID_SOUND_MUSIC_OFF));

    // "Effects" setting
    optiongroup = groupSound->GetCtrl<ctrlOptionGroup>(ID_SOUND_EFFECTS_GROUP);
    optiongroup->SetSelection((SETTINGS.sound.effectsEnabled ? ID_SOUND_EFFECTS_ON : ID_SOUND_EFFECTS_OFF));

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
        case ID_SOUND_EFFECTS_VOLUME:
        {
            SETTINGS.sound.effectsVolume = static_cast<uint8_t>((position * 255) / 100);
            AUDIODRIVER.SetMasterEffectVolume(SETTINGS.sound.effectsVolume);
        }
        break;
        case ID_SOUND_MUSIC_PLAYER:
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
        case ID_COMMON_PORTRAIT_COMBO:
            SETTINGS.lobby.portraitIndex = selection;
            updatePortraitControls();
            break;
        case ID_COMMON_LANGUAGE_COMBO: // Language
        {
            // Language changed?
            std::string old_lang = SETTINGS.language.language; //-V807
            SETTINGS.language.language = LANGUAGES.setLanguage(selection);
            if(SETTINGS.language.language != old_lang)
                WINDOWMANAGER.Switch(std::make_unique<dskOptions>());
        }
        break;
        case ID_COMMON_PROXY_TYPE_COMBO: // Proxy
            switch(selection)
            {
                case 0: SETTINGS.proxy.type = ProxyType::None; break;
                case 1: SETTINGS.proxy.type = ProxyType::Socks4; break;
                case 2: SETTINGS.proxy.type = ProxyType::Socks5; break;
            }

            // ipv6 gleich sichtbar deaktivieren
            if(SETTINGS.proxy.type == ProxyType::Socks4 && SETTINGS.server.ipv6)
            {
                GetCtrl<ctrlGroup>(ID_GROUP_COMMON)
                  ->GetCtrl<ctrlOptionGroup>(ID_COMMON_IPV6_GROUP)
                  ->SetSelection(ID_COMMON_IPV6_BUTTON_OFF);
                GetCtrl<ctrlGroup>(ID_GROUP_COMMON)
                  ->GetCtrl<ctrlOptionGroup>(ID_COMMON_IPV6_GROUP)
                  ->GetCtrl<ctrlButton>(ID_COMMON_IPV6_BUTTON_ON)
                  ->SetEnabled(false);
                SETTINGS.server.ipv6 = false;
            }

            if(SETTINGS.proxy.type != ProxyType::Socks4)
                GetCtrl<ctrlGroup>(ID_GROUP_COMMON)
                  ->GetCtrl<ctrlOptionGroup>(ID_COMMON_IPV6_GROUP)
                  ->GetCtrl<ctrlButton>(ID_COMMON_IPV6_BUTTON_ON)
                  ->SetEnabled(true);
            break;
        case ID_COMMON_LOCAL_PORT_EDIT: // Fullscreen resolution
            SETTINGS.video.fullscreenSize = video_modes[selection];
            break;
        case ID_GRAPHICS_VSYNC_COMBO: // Limit Framerate
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
        case ID_GRAPHICS_DRIVER_COMBO: // Videotreiber
            SETTINGS.driver.video = combo->GetText(selection);
            break;
        case ID_SOUND_DRIVER_COMBO: // Audiotreiber
            SETTINGS.driver.audio = combo->GetText(selection);
            break;
    }
}

void dskOptions::Msg_Group_OptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                             const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_COMMON_IPV6_GROUP: // IPv6 Ja/Nein
        {
            switch(selection)
            {
                case ID_COMMON_IPV6_BUTTON_ON: SETTINGS.server.ipv6 = true; break;
                case ID_COMMON_IPV6_BUTTON_OFF: SETTINGS.server.ipv6 = false; break;
            }
        }
        break;
        case ID_GRAPHICS_MODE_GROUP: // Vollbild
        {
            switch(selection)
            {
                case ID_GRAPHICS_MODE_FULLSCREEN: SETTINGS.video.fullscreen = true; break;
                case ID_GRAPHICS_MODE_WINDOWED: SETTINGS.video.fullscreen = false; break;
            }
        }
        break;
        case ID_GRAPHICS_VBO_GROUP: // VBO
        {
            switch(selection)
            {
                case ID_GRAPHICS_VBO_ON: SETTINGS.video.vbo = true; break;
                case ID_GRAPHICS_VBO_OFF: SETTINGS.video.vbo = false; break;
            }
        }
        break;
        case ID_GRAPHICS_OPTIMIZED_TEXTURES_GROUP:
        {
            switch(selection)
            {
                case ID_GRAPHICS_OPTIMIZED_TEXTURES_ON: SETTINGS.video.shared_textures = true; break;
                case ID_GRAPHICS_OPTIMIZED_TEXTURES_OFF: SETTINGS.video.shared_textures = false; break;
            }
        }
        break;

        case ID_SOUND_MUSIC_GROUP: // Music
        {
            switch(selection)
            {
                case ID_SOUND_MUSIC_ON: SETTINGS.sound.musicEnabled = true; break;
                case ID_SOUND_MUSIC_OFF: SETTINGS.sound.musicEnabled = false; break;
            }
            if(SETTINGS.sound.musicEnabled)
                MUSICPLAYER.Play();
            else
                MUSICPLAYER.Stop();
        }
        break;
        case ID_SOUND_EFFECTS_GROUP: // Soundeffekte
        {
            switch(selection)
            {
                case ID_SOUND_EFFECTS_ON: SETTINGS.sound.effectsEnabled = true; break;
                case ID_SOUND_EFFECTS_OFF: SETTINGS.sound.effectsEnabled = false; break;
            }
        }
        break;
        case ID_COMMON_DEBUG_DATA_GROUP: // Submit debug data
        {
            switch(selection)
            {
                case ID_COMMON_DEBUG_DATA_ON: SETTINGS.global.submit_debug_data = 1; break;
                case ID_COMMON_DEBUG_DATA_OFF: SETTINGS.global.submit_debug_data = 2; break;
            }
        }
        break;
        case ID_COMMON_UPNP_GROUP:
        {
            switch(selection)
            {
                case ID_COMMON_UPNP_ON: SETTINGS.global.use_upnp = 1; break;
                case ID_COMMON_UPNP_OFF: SETTINGS.global.use_upnp = 0; break;
            }
        }
        break;
        case ID_COMMON_SMART_CURSOR_GROUP:
        {
            switch(selection)
            {
                case ID_COMMON_SMART_CURSOR_ON: SETTINGS.global.smartCursor = true; break;
                case ID_COMMON_SMART_CURSOR_OFF: SETTINGS.global.smartCursor = false; break;
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
        case ID_OPTIONS_GROUP: // Optionengruppen anzeigen
        {
            static const std::array<unsigned int, 3> buttons{ID_COMMON_BUTTON, ID_GRAPHICS_BUTTON,
                                                             ID_SOUND_MUSIC_BUTTON};
            static const std::array<unsigned int, 3> groups{ID_GROUP_COMMON, ID_GROUP_GRAPHICS, ID_GROUP_SOUND};
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
        case ID_BACK_BUTTON: // "Back"
        {
            auto* groupCommon = GetCtrl<ctrlGroup>(ID_GROUP_COMMON);

            // Name abspeichern
            SETTINGS.lobby.name = groupCommon->GetCtrl<ctrlEdit>(ID_COMMON_NAME_EDIT)->GetText();
            if(!validatePort(groupCommon->GetCtrl<ctrlEdit>(ID_COMMON_LOCAL_PORT_EDIT)->GetText(),
                             SETTINGS.server.localPort))
                return;

            SETTINGS.proxy.hostname = groupCommon->GetCtrl<ctrlEdit>(ID_COMMON_PROXY_EDIT_ADDRESS)->GetText();
            if(!validatePort(groupCommon->GetCtrl<ctrlEdit>(ID_COMMON_PROXY_EDIT_PORT)->GetText(), SETTINGS.proxy.port))
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
        case ID_ADDONS_BUTTON: // Addons
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwAddons>(ggs));
            break;
    }
}

void dskOptions::Msg_Group_ButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;
        case ID_COMMON_PORTRAIT_BUTTON: // Click on portrait
            SETTINGS.lobby.portraitIndex = (SETTINGS.lobby.portraitIndex + 1) % Portraits.size();
            updatePortraitControls();
            break;
        case ID_SOUND_MUSIC_PLAYER: // "Music player"
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwMusicPlayer>());
            break;
        case ID_COMMON_KEY_LAYOUT_BUTTON: // "Keyboard Readme"
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
    auto* groupCommon = GetCtrl<ctrlGroup>(ID_GROUP_COMMON);

    auto* portraitButton = groupCommon->GetCtrl<ctrlImageButton>(ID_COMMON_PORTRAIT_BUTTON);
    auto* newPortraitTexture = LOADER.GetTextureN(newPortrait.resourceId, newPortrait.resourceIndex);
    portraitButton->SetImage(newPortraitTexture);

    auto* portraitCombo = groupCommon->GetCtrl<ctrlComboBox>(ID_COMMON_PORTRAIT_COMBO);
    portraitCombo->SetSelection(SETTINGS.lobby.portraitIndex);
}
